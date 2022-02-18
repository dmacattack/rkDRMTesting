#include "drmcapture.hpp"
#include <QDebug>
#include <sys/mman.h>
#include "filehandler.hpp"

#define DBG_BLOCK 0

// anonymous namespace
namespace
{
    const char* DRM_CARD_PATH = "/dev/dri/card0";
}

/**
 * @brief DRMCapture::DRMCapture - ctor
 */
DRMCapture::DRMCapture()
: drm_fd            (0)
, chosen_resolution (NULL)
, valid_connector   (NULL)
, screen_encoder    (NULL)
, crtc_to_restore   (NULL)
, create_request    ()
, frame_buffer_id   (0)
, primed_framebuffer(NULL)
{

}

/**
 * @brief DRMCapture::capture - execute the capture process
 * @returns DRMBuffer object
 */
DRMBuffer *DRMCapture::capture()
{
    DRMBuffer *pBuffer = NULL;
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
    drmModeRes* __restrict drm_resources;
    int ret = 0;

    // Open the DRM device node and get a File Descriptor
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
#if DBG_BLOCK
                    qDebug() << "current crtc id = " << current_crtc_id;
#endif

                    drmModeCrtcPtr pcrtc = drmModeGetCrtc(drm_fd, current_crtc_id);

                    if (pcrtc == NULL)
                    {
                        qDebug() << "ctrc is null";
                    }
                    else
                    {

                        uint32_t bufId = pcrtc->buffer_id;
#if DBG_BLOCK
                        qDebug() << "crtc is not null, buffer id = " << bufId;
#endif

// ============================================================================================
                        // get the planes
// ============================================================================================
                        drmModePlaneResPtr plane_res = drmModeGetPlaneResources(drm_fd);

                        if (plane_res == NULL)
                        {
                            qFatal("plane resources is null ");
                        }

#if DBG_BLOCK
                        // print out the available planes

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
#endif

#if DBG_BLOCK
                        // uncomment this to allow manual user selection of the plane.

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

                        qDebug() << "get the framebuffer from the fbid " << fbid;
#else
                        // use the framebuffer id
                        int fbid = static_cast<int>(bufId);
#endif

// ============================================================================================
                        // get the framebuffer from the given id
// ============================================================================================


                        drmModeFBPtr pFb = drmModeGetFB(drm_fd, fbid);

                        if (pFb == NULL)
                        {
                            qFatal("fb is null");
                        }

#if DBG_BLOCK
                        qDebug("framebuffer wxh @bpp = %ux%u @%u.pitch = %u. depth = %u. handle = %u",
                               pFb->width,
                               pFb->height,
                               pFb->bpp,
                               pFb->pitch,
                               pFb->depth,
                               pFb->handle
                               );
#endif

                        // get the memory
                        struct drm_mode_map_dumb mreq;
                        //uint8_t *map = NULL;

                        memset(&mreq, 0, sizeof(mreq));
                        mreq.handle = pFb->handle;
                        ret = drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
                        if (ret)
                        {
                            qFatal("error %s", strerror(errno));
                        }

                        size_t size = pFb->height * pFb->pitch;
#if DBG_BLOCK
                        qDebug() << "frame buffer size = " << size;
#endif
                        // create a buffer
                        pBuffer = new DRMBuffer(size, drm_fd, mreq.offset);
                    } // ~pcrtc
                } // ~screen_encoder
            } // ~chosen_resolution

            // cleanup
            drmModeFreeEncoder(screen_encoder);
            drmModeFreeModeInfo(chosen_resolution);
            // drmModeFreeConnector(valid_connector);  // throws a double free or corruption error. TODO figure out why
            close(drm_fd);
        } // ~valid_connector
    } // ~valid_fd
    else
    {
        qDebug() << __FUNCTION__ << __LINE__ << "Could not open /dev/dri/card0 : %m\n";
    }

    return pBuffer;
}

/**
 * @brief could_not_map_or_export_buffer - cleanup when the buffer could not be mapped or exported
 */
void DRMCapture::could_not_map_or_export_buffer()
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
void DRMCapture::could_not_retrieve_current_ctrc()
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
void DRMCapture::could_not_add_or_alloc_frame_buffer()
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
void DRMCapture::could_not_add_buffer(int err)
{
    qDebug("Could not add a framebuffer using drmModeAddFB : %s\n", strerror(err) );
    could_not_add_or_alloc_frame_buffer();
}

/**
 * @brief could_not_alloc_buffer - cleanup when the dumb buffer could not be allocated
 * @param err - errno code
 */
void DRMCapture::could_not_alloc_buffer(int err)
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
void DRMCapture::could_not_retreive_encoder()
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
void DRMCapture::no_valid_resolution()
{
    qDebug() << "No preferred resolution on the selected connector " << valid_connector->connector_id;
    drmModeFreeConnector(valid_connector);
    close(drm_fd);
}

/**
 * @brief no_valid_connector - valid connector not found
 */
void DRMCapture::no_valid_connector()
{
    qDebug() << "No connectors or no connected connectors found...\n";
    close(drm_fd);
}

/**
 * @brief cleanup_all - cleanup everything on a successful transaction
 */
void DRMCapture::cleanup_all()
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

