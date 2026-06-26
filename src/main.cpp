#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("SRTForge");
    QApplication::setOrganizationName("SRTForge");

    MainWindow window;
    window.show();
    window.raise();
    window.activateWindow();

    return app.exec();
}
