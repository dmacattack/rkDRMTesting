// This will works on Embedded GPU that implements .gem_prime_mmap like Rockchip ones.
// This will fail on most DRM drivers for GPU with dedicated memory as they tend to NOT implement .gem_prime_mmap.
#include <stdio.h>
#include <libdrm/drm.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <stdlib.h> // rand
#include <QDebug>
#include <QFile>
#include <QTextStream>

// constants
const char* DRM_CARD_PATH = "/dev/dri/card0";

// member vars
int drm_fd = 0;
drmModeModeInfo    * __restrict chosen_resolution = NULL;
drmModeConnector   * __restrict valid_connector   = NULL;
drmModeEncoder     * __restrict screen_encoder    = NULL;
drmModeCrtc        * __restrict crtc_to_restore   = NULL;
struct drm_mode_create_dumb create_request;
uint32_t frame_buffer_id = 0;
uint8_t* primed_framebuffer = NULL;

// function prototypes
void could_not_map_or_export_buffer();
void could_not_retrieve_current_ctrc();
void could_not_add_or_alloc_frame_buffer();
void could_not_add_buffer(int err);
void could_not_alloc_buffer(int err);
void could_not_retreive_encoder();
void no_valid_resolution();
void no_valid_connector();
void cleanup_all();


// Works on Rockchip systems but fail with ENOSYS on AMDGPU
int main(int argc, char *argv[])
{
    qDebug() << "v01.02.2021";
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    /* DRM is based on the fact that you can connect multiple screens,
     * on multiple different connectors which have, of course, multiple
     * encoders that transform CRTC (The screen final buffer where all
     * the framebuffers are blended together) represented in XRGB8888 (or
     * similar) into something the selected screen comprehend.
     * (Think XRGB8888 to DVI-D format for example)
     *
     * The default selection system is simple :
     * - We try to find the first connected screen and choose its
     *   preferred resolution.
     */
    drmModeRes         * __restrict drm_resources;

    int ret = 0;

    /* Open the DRM device node and get a File Descriptor */
    drm_fd = open(DRM_CARD_PATH, O_RDWR | O_CLOEXEC);

    if (drm_fd >= 0)
    {
        /* Let's see what we can use through this drm node */
        drm_resources = drmModeGetResources(drm_fd);

        /* Get a valid connector. A valid connector is one that's connected */
        for (int_fast32_t c = 0; c < drm_resources->count_connectors; c++)
        {
            valid_connector = drmModeGetConnector(drm_fd, drm_resources->connectors[c]);

            if (valid_connector->connection == DRM_MODE_CONNECTED)
            {
                break;
            }

            drmModeFreeConnector(valid_connector);
            valid_connector = NULL;
        }

        /* Bail out if nothing was connected */
        if (!valid_connector)
        {
            no_valid_connector();
        }
        else
        {
            /* Get the preferred resolution */
            for (int_fast32_t m = 0; m < valid_connector->count_modes; m++)
            {
                drmModeModeInfo * __restrict tested_resolution = &valid_connector->modes[m];
                if (tested_resolution->type & DRM_MODE_TYPE_PREFERRED)
                {
                    chosen_resolution = tested_resolution;
                    break;
                }
            }

            /* Bail out if there's no such thing as a "preferred resolution" */
            if (!chosen_resolution)
            {
                no_valid_resolution();
            }
            else
            {
                /* Get an encoder that will transform our CRTC data into something
                 * the screen comprehend natively, through the chosen connector */
                screen_encoder = drmModeGetEncoder(drm_fd, valid_connector->encoder_id);

                /* If there's no screen encoder through the chosen connector, bail
                 * out quickly. */
                if (!screen_encoder)
                {
                    could_not_retreive_encoder();
                }
                else
                {
                    // get the current crtc id
                    uint32_t current_crtc_id = screen_encoder->crtc_id;
                    qDebug() << "current crtc id = " << current_crtc_id;

                    drmModeCrtcPtr pcrtc = drmModeGetCrtc(drm_fd, current_crtc_id);

                    if (pcrtc == NULL)
                    {
                        qDebug() << "ctrc is null";
                    }
                    else
                    {

                        uint32_t bufId = pcrtc->buffer_id;
                        qDebug() << "crtc is not null, buffer id = " << bufId;

// ============================================================================================
                        // get the planes
// ============================================================================================
                        drmModePlaneResPtr plane_res = drmModeGetPlaneResources(drm_fd);

                        if (plane_res == NULL)
                        {
                            return EXIT_FAILURE;
                        }

                        qDebug() << "there are " << plane_res->count_planes << "planes";
                        drmModePlanePtr plane = NULL;
                        for (uint32_t i = 0 ; i < plane_res->count_planes; i++)
                        {

                            plane = drmModeGetPlane(drm_fd, plane_res->planes[i]);
                            if (!plane)
                            {
                                qDebug("Failed to get plane %d: %s.",
                                       plane_res->planes[i],
                                       strerror(errno));
                                plane = NULL;
                                continue;
                            }

                            qDebug( "Plane_id=%u CRTC_ID=%u FB_ID=%u. crtc x,y = %u,%u",
                                   plane->plane_id,
                                    plane->crtc_id,
                                    plane->fb_id,
                                    plane->crtc_x,
                                    plane->crtc_y
                                    );
                        }

                        // TODO error check on the plane
                        qDebug() << "which fbid should we use ? ";
                        QTextStream qtin(stdin);
                        QString input = qtin.readLine().trimmed();
                        int fbid = 0;
                        if (input.isEmpty())
                        {
                            qDebug() << "using the framebuffer ID: " << bufId;
                            fbid = static_cast<int>(bufId);
                        }
                        else
                        {
                            fbid = qtin.readLine().trimmed().toInt();
                        }

// ============================================================================================
                        // get the framebuffer from the given id
// ============================================================================================


#if 1
                        qDebug() << "get the framebuffer from the fbid " << fbid;
                        drmModeFBPtr pFb = drmModeGetFB(drm_fd, fbid);
#else
                        qDebug() << "get the framebuffer from the plane";
                        drmModeFBPtr pFb = drmModeGetFB(drm_fd, plane->fb_id);
#endif
                        if (pFb == NULL)
                        {
                            qFatal("fb is null");
                        }

                        qDebug("framebuffer wxh @bpp = %ux%u @%u.pitch = %u. depth = %u. handle = %u",
                               pFb->width,
                               pFb->height,
                               pFb->bpp,
                               pFb->pitch,
                               pFb->depth,
                               pFb->handle
                               );

                        // get the memory
                        struct drm_mode_map_dumb mreq;
                        uint8_t *map;

                        memset(&mreq, 0, sizeof(mreq));
                        mreq.handle = pFb->handle;
                        ret = drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
                        if (ret)
                        {
                            qFatal("error %s", strerror(errno));
                        }

                        size_t size = pFb->height * pFb->pitch;
                        qDebug() << "frame buffer size = " << size;

                        map = (uint8_t*) mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mreq.offset);
                        if (map == MAP_FAILED)
                        {
                            qFatal("map failed");
                        }
                        else if(map == NULL)
                        {
                            qFatal("map is null");
                        }

#if 0
                        // paint on the screen

                        qDebug() << "paint on the screen";

                        uint32_t const bytes_per_pixel = 4;
                        uint8_t r,g,b;
                        uint32_t ht = 100; //pFb->height;
                        uint32_t wd = 100; //pFb->width;
                        r = 0x00;
                        g = 0xff;
                        b = 0x00;
                        uint32_t color = (r << 16) | (g << 8) | b;

                        /* Pitch is the stride in bytes.
                         * However, for our purpose we'd like to know the stride in pixels.
                         * So we'll divide the pitch (in bytes) by the number of bytes
                         * composing a pixel to get that information.
                         */
                        uint32_t const stride_pixel = pFb->pitch / bytes_per_pixel;
                        uint32_t const width_pixel  = pFb->width;

                        qDebug() << "stride_pixel = " << stride_pixel;
                        qDebug() << "width_pixel = " << width_pixel;

                        /*
                        for (uint32_t j = 0 ; j < ht; j++)
                        {
                            for (uint32_t i = 0 ; i < wd; i++)
                            {
                                ((uint32_t *) map)[off] = color;
                            }
                        }
                        */

#endif
#if 1
                        // copy it to a file
                        QFile file("/mnt/userdata/test.data");
                        if (!file.open(QIODevice::WriteOnly))
                        {
                            qCritical() << "couldnt open test file";
                        }
                        else
                        {
                            qDebug() << "saving to a file";
                            // pointer to the data

                            // write in the data

                            file.write((char*)map, size);


                            // close the file
                            file.close();
                        }
#endif


                        qFatal("stop");

#if 0
                        // request a prime handle
                        struct drm_prime_handle prime_request =
                        {
                            .handle = pFb->handle,
                            .flags  = DRM_CLOEXEC | DRM_RDWR,
                            .fd     = -1
                        };

                        ret = ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_request);
                        qFatal("primehandle = %d %d ", ret, prime_request.handle );



                        int dma_buf_fd = -1;
                        int resp = drmPrimeHandleToFD(drm_fd, prime_request.handle, O_RDONLY, &dma_buf_fd);

                        qDebug() << "convert handle to fd resp" << resp << "dma_fd = " << dma_buf_fd;

                        qFatal("exit");

                        /* If we could not export the buffer, bail out since that's the
                         * purpose of our test */
                        if (ret || dma_buf_fd < 0)
                        {

                            qDebug("Could not export buffer : %s (%d) - FD : %d\n",
                                   strerror(ret), ret,
                                   dma_buf_fd );

                            could_not_map_or_export_buffer();
                        }
                        else
                        {
                            // map the buffer
                            size_t len = pFb->width * pFb->height * 4;
                            qDebug() << "buffer size = " << len;
                            void *pMap = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, dma_buf_fd, 0);
                            char* pData = static_cast<char*>(pMap);

                            if (pData == NULL)
                            {
                                qDebug() << "mmap is NULL";
                            }
                            else if (pData == MAP_FAILED)
                            {
                                qDebug() << "mmap is FAILED" << strerror(errno);
                            }
                            else
                            {

                                qDebug() << "mmap is not null !!!!";

                                // change some pixels
                                memset(pData, 0xff00ff00, len/2);

#if 0
                                for (int i = 0; i < 100; i ++)
                                {
                                    qDebug("%d) 0x%08X", i, pData[i]);
                                }
#endif

#if 0
                                QFile file("/mnt/userdata/test.data");
                                if (!file.open(QIODevice::WriteOnly))
                                {
                                    qCritical() << "couldnt open test file";
                                }
                                else
                                {
                                    qDebug() << "saving to a file";
                                    // pointer to the data

                                    // write in the data

                                    file.write(pData, len);


                                    // close the file
                                    file.close();
                                }

#endif
                                munmap(pMap, len);
                            }
                        }
#endif
                    }





                    //drmModeFBPtr pBuf = drmModeGetFB(drm_fd, bufId);


                    qDebug("\n\n\n");
                    // cleanup
                    drmModeFreeEncoder(screen_encoder);
                    drmModeFreeModeInfo(chosen_resolution);
                    drmModeFreeConnector(valid_connector);
                    close(drm_fd);



#if 0
                    return 0;

                    /* We're almost done with KMS. We'll now allocate a "dumb" buffer on
                     * the GPU, and use it as a "frame buffer", that is something that
                     * will be read and displayed on screen (the CRTC to be exact) */

                    /* Request a dumb buffer */
                    memset(&create_request, 0, sizeof(create_request) );
                    create_request.width  = chosen_resolution->hdisplay;
                    create_request.height = chosen_resolution->vdisplay;
                    create_request.bpp    = 32;

                    ret = ioctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_request);

                    /* Bail out if we could not allocate a dumb buffer */
                    if (ret)
                    {
                        could_not_alloc_buffer(ret);
                    }
                    else
                    {
                        /* Create a framebuffer, using the old method.
                         *
                         * A new method exist, drmModeAddFB2 : Return of the Revengeance !
                         *
                         * Jokes aside, this other method takes well defined color formats
                         * as arguments instead of specifying the depth and BPP manually.
                         */
                        ret = drmModeAddFB
                        (
                            drm_fd,                        // file descriptor
                            chosen_resolution->hdisplay,   // width
                            chosen_resolution->vdisplay,   // height
                            24,                            // depth
                            32,                            // bits per pixel
                            create_request.pitch,          // pitch
                            create_request.handle,         // handle
                            &frame_buffer_id               // fb id populated by this call
                        );


                        /* Without framebuffer, we won't do anything so bail out ! */
                        if (ret)
                        {
                            could_not_add_buffer(ret);
                        }
                        else
                        {
                            /* We assume that the currently chosen encoder CRTC ID is the current
                             * one.
                             */
                            uint32_t current_crtc_id = screen_encoder->crtc_id;

                            if (!current_crtc_id)
                            {
                                qDebug("The current encoder has no CRTC attached... ?");
                                could_not_retrieve_current_ctrc();
                            }
                            else
                            {
                                /* Backup the informations of the CRTC to restore when we're done.
                                 * The most important piece seems to currently be the buffer ID.
                                 */
                                crtc_to_restore = drmModeGetCrtc(drm_fd, current_crtc_id);

                                if (!crtc_to_restore)
                                {
                                    qDebug("Could not retrieve the current CRTC with a valid ID !\n");
                                    could_not_retrieve_current_ctrc();
                                }
                                else
                                {
                                    /* Set the CRTC so that it uses our new framebuffer */
                                    ret = drmModeSetCrtc ( drm_fd,
                                                           current_crtc_id,
                                                           frame_buffer_id,
                                                           0,
                                                           0,
                                                           &valid_connector->connector_id,
                                                           1,
                                                           chosen_resolution );

                                    /* For this test only : Export our dumb buffer using PRIME */
                                    /* This will provide us a PRIME File Descriptor that we'll use to
                                     * map the represented buffer. This could be also be used to reimport
                                     * the GEM buffer into another GPU */
                                    struct drm_prime_handle prime_request =
                                    {
                                        .handle = create_request.handle,
                                        .flags  = DRM_CLOEXEC | DRM_RDWR,
                                        .fd     = -1
                                    };

                                    ret = ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_request);
                                    int const dma_buf_fd = prime_request.fd;

                                    /* If we could not export the buffer, bail out since that's the
                                     * purpose of our test */
                                    if (ret || dma_buf_fd < 0)
                                    {

                                        qDebug("Could not export buffer : %s (%d) - FD : %d\n",
                                               strerror(ret), ret,
                                               dma_buf_fd );

                                        could_not_map_or_export_buffer();
                                    }
                                    else
                                    {
                                        /* Map the exported buffer, using the PRIME File descriptor */
                                        /* That ONLY works if the DRM driver implements gem_prime_mmap.
                                         * This function is not implemented in most of the DRM drivers for
                                         * GPU with discrete memory. Meaning that it will surely fail with
                                         * Radeon, AMDGPU and Nouveau drivers for desktop cards ! */
                                        void *pMap = mmap(0, create_request.size, PROT_READ | PROT_WRITE, MAP_SHARED, dma_buf_fd, 0);
                                        const char* pData = static_cast<char*>(pMap); = static_cast<uint8_t*>(pMap);

                                        ret = errno;

                                        /* Bail out if we could not map the framebuffer using this method */
                                        if (primed_framebuffer == NULL || primed_framebuffer == MAP_FAILED)
                                        {
                                            qDebug( "%s %d Could not map buffer exported through PRIME : %s (%d). Buffer : %p\n",
                                                __FUNCTION__,
                                                __LINE__,
                                                strerror(ret),
                                                ret,
                                                primed_framebuffer );

                                            could_not_map_or_export_buffer();
                                        }
                                        else
                                        {
                                            qDebug("%s %d Buffer mapped !\n", __FUNCTION__, __LINE__);

                                            /* The fun begins ! At last !
                                             * We'll do something simple :
                                             * We'll lit a row of pixel, on the screen, starting from the top,
                                             * down to the bottom of screen, using either Red, Blue or Green
                                             * randomly, each time we press Enter.
                                             * If we press 'q' and then Enter, the process will stop.
                                             * The process will also stop once we've reached the bottom of the
                                             * screen.
                                             */
                                            uint32_t const bytes_per_pixel = 4;
                                            uint_fast64_t pixel = 0;
                                            uint_fast64_t size = create_request.size;

                                            /* Cleanup the framebuffer */
                                            memset(primed_framebuffer, 0xffffff00, size);

                                            /* The colors table */
                                            uint32_t const red   = (0xff<<16);
                                            uint32_t const green = (0xff<<8);
                                            uint32_t const blue  = (0xff);
                                            uint32_t const colors[] = {red, green, blue};

                                            /* Pitch is the stride in bytes.
                                             * However, for our purpose we'd like to know the stride in pixels.
                                             * So we'll divide the pitch (in bytes) by the number of bytes
                                             * composing a pixel to get that information.
                                             */
                                            uint32_t const stride_pixel = create_request.pitch / bytes_per_pixel;
                                            uint32_t const width_pixel  = create_request.width;

                                            /* The width is padded so that each row starts with a specific
                                             * alignment. That means that we have useless pixels that we could
                                             * avoid dealing with in the first place.
                                             * Now, it might be faster to just lit these useless pixels and get
                                             * done with it. */
                                            uint32_t const diff_between_width_and_stride = stride_pixel - width_pixel;
                                            uint_fast64_t const size_in_pixels =create_request.height * stride_pixel;

                                            qDebug() << "enter a key to start drawing pixels. enter will draw a line, 'q' will exit";
                                            getc(stdin);

                                            /* While we didn't get a 'q' + Enter or reached the bottom of the
                                             * screen... */
                                            while (getc(stdin) != 'q' && pixel < size_in_pixels)
                                            {
                                                /* Choose a random color. 3 being the size of the colors table. */
                                                uint32_t current_color = colors[rand()%3];

                                                /* Color every pixel of the row.
                                                 * Now, the framebuffer is linear. Meaning that the first pixel of
                                                 * the first row should be at index 0, but the first pixel of the
                                                 * second row should be at index (stride+0) and the first pixel of
                                                 * the n-th row should be at (n*stride+0).
                                                 *
                                                 * Instead of computing the value, we'll just increment the "pixel"
                                                 * index and accumulate the padding once done with the current row,
                                                 * in order to be ready to start for the next row.
                                                 */
                                                for (uint_fast32_t p = 0; p < width_pixel; p++)
                                                {
                                                    ((uint32_t *) primed_framebuffer)[pixel++] = current_color;
                                                }
                                                pixel += diff_between_width_and_stride;
                                                //LOG("pixel : %lu, size : %lu\n", pixel, size_in_pixels);
                                            }
                                            cleanup_all();
                                        } // ~ successfully mapped buffer
                                    } // ~ successfully export buffer
                                } // ~ crtc_to_restore is valid
                            } // ~ successfully retrieved ctrc encoder
                        } // ~successfully added a framebuffer
                    } // ~ buffer successfully allocated
#endif
                } // ~valid screen_encoder
            } // ~valid_resolution
        } // ~valid_connector
    } // ~valid_fd
    else
    {
        qDebug() << __FUNCTION__ << __LINE__ << "Could not open /dev/dri/card0 : %m\n";
    }

    return 0;
}


/**
 * @brief could_not_map_or_export_buffer - cleanup when the buffer could not be mapped or exported
 */
void could_not_map_or_export_buffer()
{
    struct drm_mode_destroy_dumb destroy_request;
    memset(&destroy_request, 0 , sizeof(destroy_request));
    destroy_request.handle = create_request.handle;
    ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_request);
    drmModeSetCrtc
    (
        drm_fd,
        crtc_to_restore->crtc_id, crtc_to_restore->buffer_id,
        0, 0, &valid_connector->connector_id, 1, &crtc_to_restore->mode
    );
    drmModeRmFB(drm_fd, frame_buffer_id);
    drmModeFreeEncoder(screen_encoder);
    drmModeFreeModeInfo(chosen_resolution);
    drmModeFreeConnector(valid_connector);
    close(drm_fd);
}

/**
 * @brief could_not_retrieve_current_ctrc - cleanup when the curr crtc could not be found
 */
void could_not_retrieve_current_ctrc()
{

    drmModeRmFB(drm_fd, frame_buffer_id);
    drmModeFreeEncoder(screen_encoder);
    drmModeFreeModeInfo(chosen_resolution);
    drmModeFreeConnector(valid_connector);
    close(drm_fd);
}

/**
 * @brief could_not_add_or_alloc_frame_buffer - cleanup when the framebuffer could not be added or allocated
 */
void could_not_add_or_alloc_frame_buffer()
{
    drmModeFreeEncoder(screen_encoder);
    drmModeFreeModeInfo(chosen_resolution);
    drmModeFreeConnector(valid_connector);
    close(drm_fd);
}

/**
 * @brief could_not_add_buffer- cleanup when the dumb buffer could not be added
 * @param err - errno code
 */
void could_not_add_buffer(int err)
{
    qDebug("Could not add a framebuffer using drmModeAddFB : %s\n",
           strerror(err) );
    could_not_add_or_alloc_frame_buffer();
}

/**
 * @brief could_not_alloc_buffer - cleanup when the dumb buffer could not be allocated
 * @param err - errno code
 */
void could_not_alloc_buffer(int err)
{
    qDebug("Dumb Buffer Object Allocation request of %ux%u@%u failed : %s\n",
           create_request.width, create_request.height,
           create_request.bpp,
           strerror(err)
           );
    could_not_add_or_alloc_frame_buffer();
}

/**
 * @brief could_not_retreive_encoder - cleanup when the encoder could not be retreived
 */
void could_not_retreive_encoder()
{
    qDebug("Could not retrieve the encoder for mode %s, on connector %u",
           chosen_resolution->name,
           valid_connector->connector_id);

    drmModeFreeModeInfo(chosen_resolution);
    drmModeFreeConnector(valid_connector);
    close(drm_fd);
}

/**
 * @brief no_valid_resolution - cleanup when the resolution is invalid
 */
void no_valid_resolution()
{
    qDebug() << "No preferred resolution on the selected connector " << valid_connector->connector_id;
    drmModeFreeConnector(valid_connector);
    close(drm_fd);
}

/**
 * @brief no_valid_connector - valid connector not found
 */
void no_valid_connector()
{
    qDebug() << "No connectors or no connected connectors found...\n";
    close(drm_fd);
}

/**
 * @brief cleanup_all - cleanup everything on a successful transaction
 */
void cleanup_all()
{
    struct drm_mode_destroy_dumb destroy_request;
    memset(&destroy_request, 0 , sizeof(destroy_request));
    destroy_request.handle = create_request.handle;
    ioctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_request);
    drmModeSetCrtc
    (
        drm_fd,
        crtc_to_restore->crtc_id, crtc_to_restore->buffer_id,
        0, 0, &valid_connector->connector_id, 1, &crtc_to_restore->mode
    );
    munmap(primed_framebuffer, create_request.size);
    drmModeRmFB(drm_fd, frame_buffer_id);
    drmModeFreeEncoder(screen_encoder);
    drmModeFreeModeInfo(chosen_resolution);
    //drmModeFreeConnector(valid_connector); // double free error for some reason
    close(drm_fd);
}
