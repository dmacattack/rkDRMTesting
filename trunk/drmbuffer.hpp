#ifndef DRMBUFFER_HPP
#define DRMBUFFER_HPP

#include <QString>

/**
 * @brief The DRMBuffer class - object to represent a drm buffer
 */
class DRMBuffer
{
public:
    DRMBuffer();
    DRMBuffer(size_t bufSize, int fd, __off64_t offset);
    ~DRMBuffer();

    void set(size_t bufSize, int fd, __off64_t offset);

    bool isValid();
    size_t size()     { return mBufferSize; }
    uint8_t *buffer() { return mpBuffer;    }


private:
    size_t mBufferSize;
    uint8_t *mpBuffer;
};

#endif // DRMBUFFER_HPP
