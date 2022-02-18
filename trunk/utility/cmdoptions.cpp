#include "cmdoptions.hpp"
#include <QDebug>

namespace
{
    // value defaults
    const QString DEFAULT_CONFIG_PATH = "/mnt/app/bin/mirrorSettings.json";
    const QString DEFAULT_OUTPUT_FMT  = "raw";
    const int DEFAULT_ROTATION        = 0;
}

// static variable defaults
QString CmdOptions::mConfigFilePath  = DEFAULT_CONFIG_PATH;
QString CmdOptions::mOutputFmt       = DEFAULT_OUTPUT_FMT;
int CmdOptions::mRotation            = 0;

// process the cmd line options & set the static variables
void CmdOptions::setCmdOptions(QCoreApplication &coreApp)
{
    QCommandLineParser parser;
    parser.addHelpOption();

    // create a list of options
    // -t to provide the settings file
    // -f to provide the output format
    // -r to provide the output rotation
    QCommandLineOption configFilePathOpt(QStringList() << "t" << "config",
                                         QCoreApplication::translate("main", "Set the path to the config file"),
                                         QCoreApplication::translate("main", "config"));

    QCommandLineOption setOutputFmtOpt(QStringList() << "f" << "fmt",
                                       QCoreApplication::translate("main", "set the output format"),
                                       QCoreApplication::translate("main", "raw,jpeg,png")
                                       );

    QCommandLineOption setOutputRotOpt(QStringList() << "r" << "rotation",
                                       QCoreApplication::translate("main", "set the output image rotation"),
                                       QCoreApplication::translate("main", "0,90,180,270")
                                       );

    // add the options
    parser.addOption(configFilePathOpt);
    parser.addOption(setOutputFmtOpt);
    parser.addOption(setOutputRotOpt);

    // process the options given to the application
    parser.process(coreApp);

    // populate values
    mConfigFilePath = parseCmdLineStr(parser, configFilePathOpt,  DEFAULT_CONFIG_PATH);
    mOutputFmt      = parseCmdLineStr(parser, setOutputFmtOpt,    DEFAULT_OUTPUT_FMT);
    mRotation       = parseCmdLineInt(parser, setOutputRotOpt,    DEFAULT_ROTATION);
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

