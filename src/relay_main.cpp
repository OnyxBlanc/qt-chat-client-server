#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTextStream>
#include "relayserver.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("QtChatRelay");

    QCommandLineParser parser;
    parser.setApplicationDescription("Serveur relais mixte TCP + UDP pour QtChatApp et clients SFML JSON.");
    parser.addHelpOption();
    QCommandLineOption portOption({"p", "port"}, "Port TCP et UDP (defaut : 5000).", "port", "5000");
    QCommandLineOption tcpOption("tcp-port", "Port TCP (remplace --port).", "port");
    QCommandLineOption udpOption("udp-port", "Port UDP (remplace --port).", "port");
    parser.addOption(portOption);
    parser.addOption(tcpOption);
    parser.addOption(udpOption);
    parser.process(app);

    bool ok = false;
    const int defaultPort = parser.value(portOption).toInt(&ok);
    if (!ok || defaultPort < 1 || defaultPort > 65535) {
        QTextStream(stderr) << "Port invalide." << Qt::endl;
        return 1;
    }
    const int tcpPort = parser.isSet(tcpOption) ? parser.value(tcpOption).toInt(&ok) : defaultPort;
    if (!ok || tcpPort < 1 || tcpPort > 65535) return 1;
    const int udpPort = parser.isSet(udpOption) ? parser.value(udpOption).toInt(&ok) : defaultPort;
    if (!ok || udpPort < 1 || udpPort > 65535) return 1;

    RelayServer relay;
    QObject::connect(&relay, &RelayServer::logMessage, [](const QString &text) {
        QTextStream(stdout) << text << Qt::endl;
    });
    if (!relay.start(static_cast<quint16>(tcpPort), static_cast<quint16>(udpPort))) return 1;
    return app.exec();
}
