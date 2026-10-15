#include "drivers/simulated/simulated_session.h"
#include "ui/mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("QTdemo");
    QCoreApplication::setApplicationName("QTdemo");
    QCommandLineParser parser;
    parser.setApplicationDescription("Industrial data workbench - simulated backend prototype");
    parser.addHelpOption();
    parser.addOption({"config", "Load a simulated project JSON", "file"});
    parser.addOption({"smoke-test", "Exercise UI, data flow and orderly thread shutdown"});
    parser.addOption({"screenshot", "Save a screenshot during smoke test", "file"});
    parser.process(app);
    industrial::DataHub hub;
    industrial::RuntimeManager runtime(hub, industrial::simulatedSessionFactory());
    industrial::ApplicationService service(runtime);
    MainWindow window(hub, runtime, service);
    window.show();
    if (!window.loadConfiguration(parser.isSet("config") ? parser.value("config")
                                                         : ":/config/simulated.json"))
        return 2;
    int smokeResult = 0;
    if (parser.isSet("smoke-test"))
        QTimer::singleShot(800, &window,
                           [&]
                           {
                               if (!window.pageReady())
                                   smokeResult = 3;
                               const auto &source = runtime.config().sources.first();
                               if (hub.sample({source.id, source.points.first().id}).quality !=
                                   industrial::Quality::Good)
                                   smokeResult = 5;
                               if (parser.isSet("screenshot") &&
                                   !window.grab().save(parser.value("screenshot")))
                                   smokeResult = 4;
                               window.close();
                           });
    const int result = app.exec();
    return smokeResult ? smokeResult : result;
}
