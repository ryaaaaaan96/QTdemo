#include "drivers/simulated/simulated_session.h"
#include "services/config/config_repository.h"
#include "services/runtime/application_service.h"
#include "ui/models/point_table_model.h"
#include "ui/qml/page_host.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace industrial;
class PageTest : public QObject
{
    Q_OBJECT
  private slots:
    void reloadFailurePreservesPageAndAcquisition()
    {
        auto config = *ConfigRepository::load(EXAMPLE_CONFIG_PATH).config;
        for (auto &source : config.sources)
            source.intervalMs = 20;
        DataHub hub;
        RuntimeManager runtime(hub, simulatedSessionFactory());
        ApplicationService service(runtime);
        runtime.configure(config);
        PointTableModel model(hub);
        model.configure(config);
        PageHost first(model), second(model);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const auto path = dir.filePath("Page.qml");
        auto writePage = [&path](const QByteArray &contents)
        {
            QFile file(path);
            return file.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
                   file.write(contents) == contents.size();
        };
        QVERIFY(
            writePage("import QtQuick\nRectangle { width: 320; height: 200; color: 'navy' }\n"));
        QSignalSpy loaded(&first, &PageHost::loaded);
        QSignalSpy errors(&first, &PageHost::loadFailed);
        first.load(QUrl::fromLocalFile(path));
        second.load(QUrl::fromLocalFile(path));
        QTRY_VERIFY(first.hasPage() && second.hasPage());
        runtime.start();
        PointKey key{"field_1_a", "temperature"};
        QTRY_COMPARE(hub.sample(key).quality, Quality::Good);
        const auto generation = hub.sample(key).generation;
        for (int i = 0; i < 3; ++i)
        {
            first.load(QUrl::fromLocalFile(path));
            QTRY_COMPARE(loaded.size(), i + 2);
        }
        QVERIFY(writePage("import QtQuick\nRectangle { deliberately invalid }\n"));
        const auto before = hub.sample(key).sequence;
        first.load(QUrl::fromLocalFile(path));
        QTRY_COMPARE(errors.size(), 1);
        QVERIFY(first.hasPage() && second.hasPage());
        QCOMPARE(first.currentUrl(), QUrl::fromLocalFile(path));
        QTRY_VERIFY(hub.sample(key).sequence > before);
        QCOMPARE(hub.sample(key).generation, generation);
        QSignalSpy results(&service, &ApplicationService::commandFinished);
        service.submitWrite({"field_1_a", "setpoint"}, 66);
        QTRY_COMPARE(results.size(), 1);
        QCOMPARE(qvariant_cast<CommandResult>(results[0][0]).outcome, CommandOutcome::Confirmed);
        runtime.stop();
        QTRY_VERIFY(!runtime.isRunning());
    }
};
QTEST_MAIN(PageTest)
#include "test_pages.moc"
