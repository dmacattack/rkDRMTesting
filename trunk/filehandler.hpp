#ifndef FILEHANDLER_HPP
#define FILEHANDLER_HPP

#include <QString>
#include "drmbuffer.hpp"

/**
 * @brief The FileHandler class - object to handle file trasactions such as saving a buffer to a file
 */
class FileHandler
{
public:
    static void writeToFile(const QString &filePath, uint8_t *pBuf, size_t bufSize);
    static void writeToFile(const QString &filePath, DRMBuffer *pBuffer);

private:
    // static class
    FileHandler() {}
};

#endif // FILEHANDLER_HPP
