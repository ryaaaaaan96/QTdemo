#include "ui/mainwindow.h"
#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("QTdemo");
    QCoreApplication::setApplicationName("QTdemo");
    MainWindow window;
    window.show();
    // Exercises platform-plugin loading and the event loop in deployment checks.
    if (app.arguments().contains("--smoke-test"))
        QTimer::singleShot(200, &app, &QCoreApplication::quit);
    return app.exec();
}
