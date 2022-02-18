#ifndef DRMCAPTURE_HPP
#define DRMCAPTURE_HPP

// This will works on Embedded GPU that implements .gem_prime_mmap like Rockchip ones.
// This will fail on most DRM drivers for GPU with dedicated memory as they tend to NOT implement .gem_prime_mmap.

#include <QObject>
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
#include <QTextStream>

class DRMCapture
{
public:
    DRMCapture();
    void capture();

private:
    void could_not_map_or_export_buffer();
    void could_not_retrieve_current_ctrc();
    void could_not_add_or_alloc_frame_buffer();
    void could_not_add_buffer(int err);
    void could_not_alloc_buffer(int err);
    void could_not_retreive_encoder();
    void no_valid_resolution();
    void no_valid_connector();
    void cleanup_all();
    void writeToFile(const QString &filePath, uint8_t *pBuf, size_t bufSize);


private:
    int drm_fd;
    drmModeModeInfo    * __restrict chosen_resolution;
    drmModeConnector   * __restrict valid_connector;
    drmModeEncoder     * __restrict screen_encoder ;
    drmModeCrtc        * __restrict crtc_to_restore;
    struct drm_mode_create_dumb create_request;
    uint32_t frame_buffer_id;
    uint8_t* primed_framebuffer;

};

#endif // DRMCAPTURE_HPP
