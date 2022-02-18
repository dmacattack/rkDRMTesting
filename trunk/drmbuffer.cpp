#include "drmbuffer.hpp"
#include <sys/mman.h>
#include <QDebug>

/**
 * @brief DRMBuffer::DRMBuffer - ctor
 */
DRMBuffer::DRMBuffer()
: mBufferSize(0)
, mpBuffer(NULL)
{

}

/**
 * @brief DRMBuffer::DRMBuffer - overloaded ctor to set the buffer directly
 * @param bufSize - buffer size
 * @param fd - file descriptor
 * @param offset - byte offset
 */
DRMBuffer::DRMBuffer(size_t bufSize, int fd, __off64_t offset)
: DRMBuffer()
{
    set(bufSize, fd, offset);
}

/**
 * @brief DRMBuffer::~DRMBuffer - dtor
 */
DRMBuffer::~DRMBuffer()
{
    if ((mBufferSize > 0) && (mpBuffer != NULL))
    {
        munmap(mpBuffer, mBufferSize);
    }
}

/**
 * @brief DRMBuffer::set - set & map the buffer
 * @param bufSize - buffer size
 * @param fd- file descriptor
 * @param offset - byte offset
 */
void DRMBuffer::set(size_t bufSize, int fd, __off64_t offset)
{
    if (bufSize <= 0)
    {
        qWarning() << "invalid buffer size";
        return;
    }

    // set the buffer size
    mBufferSize = bufSize;

    // do the mmap
    mpBuffer = (uint8_t*) mmap(0, bufSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);

    if (mpBuffer == MAP_FAILED)
    {
        qWarning("map failed");
    }
    else if(mpBuffer == NULL)
    {
        qWarning("map is null");
    }
}

/**
 * @brief DRMBuffer::isValid - returns if the buffer is valid
 */
bool DRMBuffer::isValid()
{
    return ((mBufferSize > 0) && (mpBuffer != NULL));
}
