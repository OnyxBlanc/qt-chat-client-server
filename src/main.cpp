#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("QtChatApp");
    QApplication::setOrganizationName("OnyxBlanc");

    MainWindow window;
    window.show();

    return app.exec();
}
