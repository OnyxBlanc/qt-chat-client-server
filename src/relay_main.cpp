#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTextStream>
#include "relayserver.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("QtChatRelay");

    QCommandLineParser parser;
    parser.setApplicationDescription("Serveur relais pour QtChatApp : diffuse les messages a tous les clients connectes.");
    parser.addHelpOption();
    QCommandLineOption portOption(QStringList() << "p" << "port", "Port TCP d'ecoute (defaut : 5000).", "port", "5000");
    parser.addOption(portOption);
    parser.process(app);

    bool ok = false;
    const int port = parser.value(portOption).toInt(&ok);
    if (!ok || port < 1 || port > 65535) {
        QTextStream(stderr) << "Port invalide." << Qt::endl;
        return 1;
    }

    RelayServer relay;
    QObject::connect(&relay, &RelayServer::logMessage, [](const QString &text) {
        QTextStream(stdout) << text << Qt::endl;
    });

    if (!relay.start(static_cast<quint16>(port))) {
        return 1;
    }

    return app.exec();
}
