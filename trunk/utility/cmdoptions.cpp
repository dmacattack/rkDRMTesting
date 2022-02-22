#include "cmdoptions.hpp"
#include <QDebug>

namespace
{
    // value defaults
    const QString DEFAULT_CONFIG_PATH = "/mnt/app/bin/mirrorSettings.json";
    const QString DEFAULT_OUTPUT_FMT  = "raw";
    const QString DEFAULT_IP          = "127.0.0.1:7001";
    const int DEFAULT_ROTATION        = 0;
    const int DEFAULT_FRAMERATE       = 10;
    const QString DEFAULT_FILENAME   = "/mnt/userdata/screenshot.data";
}

// static variable defaults
QString CmdOptions::mConfigFilePath  = DEFAULT_CONFIG_PATH;
QString CmdOptions::mOutputFmt       = DEFAULT_OUTPUT_FMT;
QString CmdOptions::mOutputIP        = DEFAULT_IP;
QString CmdOptions::mOutputFileName  = DEFAULT_FILENAME;
int CmdOptions::mRotation            = DEFAULT_ROTATION;
int CmdOptions::mFPS                 = DEFAULT_FRAMERATE;

// process the cmd line options & set the static variables
void CmdOptions::setCmdOptions(QCoreApplication &coreApp)
{
    QCommandLineParser parser;
    parser.addHelpOption();

    // create a list of options
    // -t to provide the settings file
    // --fmt to provide the output format
    // --ip to set the streamable ip
    // -r/--rotation to provide the output rotation
    // --fps to set the framerate
    // --location to set the image filename
    QCommandLineOption configFilePathOpt(QStringList() << "t" << "config",
                                         QCoreApplication::translate("main", "Set the path to the config file"),
                                         QCoreApplication::translate("main", "config"));

    QCommandLineOption setOutputFmtOpt(QStringList() << "fmt",
                                       QCoreApplication::translate("main", "set the output format.\n"
                                                                           "raw - raw image will be saved \n"
                                                                           "jpeg - image will be converted to jpeg and saved \n"
                                                                           "png - image will be converted to png and saved \n"
                                                                           "adb - video will be streamed over adb (experimental) \n"
                                                                           "udp - video will be streamed over udp \n"),
                                       QCoreApplication::translate("main", "raw,jpeg,png")
                                       );

    QCommandLineOption setOutputIPOpt(QStringList() << "ip",
                                      QCoreApplication::translate("main", "set the stream IP. "),
                                      QCoreApplication::translate("main", "<ip>:<port>")
                                      );

    QCommandLineOption setOutputRotOpt(QStringList() << "r" << "rotation",
                                       QCoreApplication::translate("main", "set the output image rotation"),
                                       QCoreApplication::translate("main", "0,90,180,270")
                                       );

    QCommandLineOption setOutputFpsOpt(QStringList() << "fps",
                                       QCoreApplication::translate("main", "set the output framerate (experimental)"),
                                       QCoreApplication::translate("main", "5 .. 30")
                                       );

    QCommandLineOption setOutputFileNameOpt(QStringList() << "location",
                                            QCoreApplication::translate("main", "set the image filename"),
                                            QCoreApplication::translate("main", "/path/to/image.data")
                                            );

    // add the options
    parser.addOption(configFilePathOpt);
    parser.addOption(setOutputFmtOpt);
    parser.addOption(setOutputRotOpt);
    parser.addOption(setOutputFpsOpt);

    // process the options given to the application
    parser.process(coreApp);

    // populate values
    mConfigFilePath = parseCmdLineStr(parser, configFilePathOpt,  DEFAULT_CONFIG_PATH);
    mOutputFmt      = parseCmdLineStr(parser, setOutputFmtOpt,    DEFAULT_OUTPUT_FMT);
    mOutputIP       = parseCmdLineStr(parser, setOutputIPOpt,     DEFAULT_IP);
    mRotation       = parseCmdLineInt(parser, setOutputRotOpt,    DEFAULT_ROTATION);
    mFPS            = parseCmdLineInt(parser, setOutputFpsOpt,    DEFAULT_FRAMERATE);
    mOutputFileName = parseCmdLineStr(parser, setOutputFileNameOpt, DEFAULT_FILENAME);

    // range limit
    mFPS = (mFPS < 5 ? 5 :
           (mFPS > 30 ? 30 : mFPS));

    if (mRotation != 0 || mRotation != 90 || mRotation != 180 || mRotation != 270)
    {
        mRotation = 0;
    }
}

// parse an integer from the cmd line option
int CmdOptions::parseCmdLineInt(const QCommandLineParser &parser, const QCommandLineOption &opt, int defaultVal)
{
    int cmdlineInt = defaultVal;
    bool isInt = false;

    if ( parser.isSet(opt) )
    {
        cmdlineInt = parser.value(opt).toInt(&isInt);

        if (!isInt)
        {
            // integer conversion failed, return the default value
            cmdlineInt = defaultVal;
        }
    }

    return cmdlineInt;
}

// parse a string from the cmd line option
QString CmdOptions::parseCmdLineStr(const QCommandLineParser &parser, const QCommandLineOption &opt, QString defaultVal)
{
    QString cmdlineStr = defaultVal;

    if ( parser.isSet(opt) )
    {
        cmdlineStr = parser.value(opt);
    }

    return cmdlineStr;
}

