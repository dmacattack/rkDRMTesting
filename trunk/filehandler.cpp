#include "filehandler.hpp"
#include <QFile>
#include <QDebug>

#define DBG_BLOCK 1

/**
 * @brief FileHandler::writeToFile - write a buffer to a file
 * @param filePath - file path to write
 * @param pBuf - pointer to buffer
 * @param bufSize - buffer size in bytes
 */
void FileHandler::writeToFile(const QString &filePath, uint8_t *pBuf, size_t bufSize)
{
    // copy it to a file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        qCritical() << "couldnt open test file";
    }
    else
    {
#if DBG_BLOCK
        qDebug() << "saving buffer to file: " << filePath;
#endif
        // write in the data
        file.write((char*)pBuf, bufSize);

        // close the file
        file.close();
    }
}

/**
 * @brief FileHandler::writeToFile - write a buffer to a file
 * @param filePath - file path to write
 * @param pBuffer - pointer to drm buffer object
 */
void FileHandler::writeToFile(const QString &filePath, DRMBuffer *pBuffer)
{
    writeToFile(filePath, pBuffer->buffer(), pBuffer->size());
}
