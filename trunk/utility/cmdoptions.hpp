#ifndef CMDOPTIONS_H
#define CMDOPTIONS_H

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>

class CmdOptions
{
public:
    static void setCmdOptions(QCoreApplication &coreApp);

    // getters
    static QString configFilePath() { return mConfigFilePath; }
    static QString outputFormat()   { return mOutputFmt;      }
    static QString outputIPAddr()   { return mOutputIP;       }
    static int     imageRotation()  { return mRotation;       }
    static int     frameRate()      { return mFPS;            }
    static QString outputFilename() { return mOutputFileName; }

private:
    // class is intended to be statically called
    CmdOptions() {}

    static int parseCmdLineInt    (const QCommandLineParser &parser, const QCommandLineOption &opt, int defaultVal);
    static QString parseCmdLineStr(const QCommandLineParser &parser, const QCommandLineOption &opt, QString defaultVal);

private:
    // static members for cmd line data
    static QString mConfigFilePath;
    static QString mOutputFmt;
    static QString mOutputIP;
    static int mRotation;
    static int mFPS;
    static QString mOutputFileName;
};

#endif // CMDOPTIONS_H
