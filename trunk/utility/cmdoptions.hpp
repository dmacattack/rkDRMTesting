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
    static int     imageRotation()  { return mRotation;       }

private:
    // class is intended to be statically called
    CmdOptions() {}

    static int parseCmdLineInt    (const QCommandLineParser &parser, const QCommandLineOption &opt, int defaultVal);
    static QString parseCmdLineStr(const QCommandLineParser &parser, const QCommandLineOption &opt, QString defaultVal);

private:
    // static members for cmd line data
    static QString mConfigFilePath;
    static QString mOutputFmt;
    static int mRotation;
};

#endif // CMDOPTIONS_H
