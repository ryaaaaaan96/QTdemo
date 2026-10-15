#include "drivers/simulated/simulated_session.h"
#include "services/config/config_repository.h"
#include "services/data/sample_mailbox.h"
#include "services/runtime/application_service.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

using namespace industrial;
namespace
{
ProjectConfig demo()
{
    auto result = ConfigRepository::load(EXAMPLE_CONFIG_PATH);
    Q_ASSERT(result.config);
    auto config = *result.config;
    for (auto &source : config.sources)
        source.intervalMs = 20;
    return config;
}
CommandResult resultAt(const QSignalSpy &spy, qsizetype i)
{
    return qvariant_cast<CommandResult>(spy.at(i).at(0));
}
} // namespace
class RuntimeTest : public QObject
{
    Q_OBJECT
  private slots:
    void configValidation()
    {
        QFile file(EXAMPLE_CONFIG_PATH);
        QVERIFY(file.open(QIODevice::ReadOnly));
        auto json = QJsonDocument::fromJson(file.readAll()).object();
        QVERIFY(ConfigRepository::parse(QJsonDocument(json).toJson()).config);
        auto connections = json["connections"].toArray();
        auto connection = connections[0].toObject();
        connection["backendKind"] = "can";
        connections[0] = connection;
        json["connections"] = connections;
        auto result = ConfigRepository::parse(QJsonDocument(json).toJson());
        QVERIFY(!result.config);
        QVERIFY(result.errors.join(' ').contains("backendKind"));
        connection["backendKind"] = "simulated";
        connection["responseDelayMs"] = 1.5;
        connections[0] = connection;
        json["connections"] = connections;
        QVERIFY(!ConfigRepository::parse(QJsonDocument(json).toJson()).config);
        QVERIFY(!ConfigRepository::parse(R"({"schemaVersion":1})").config);
    }
    void configRejectsBrokenReferencesAndDuplicateIds()
    {
        QFile file(EXAMPLE_CONFIG_PATH);
        QVERIFY(file.open(QIODevice::ReadOnly));
        auto json = QJsonDocument::fromJson(file.readAll()).object();
        auto sessions = json["sessions"].toArray();
        auto session = sessions[1].toObject();
        session["id"] = "a1";
        session["connectionId"] = "missing";
        sessions[1] = session;
        json["sessions"] = sessions;
        const auto result = ConfigRepository::parse(QJsonDocument(json).toJson());
        QVERIFY(!result.config);
        QVERIFY(result.errors.join(' ').contains("duplicate"));
        QVERIFY(result.errors.join(' ').contains("unknown connection"));
    }
    void mailboxIsBoundedAndWakeupsCoalesce()
    {
        PointKey key{"field_1_a", "temperature"};
        SampleMailbox mailbox({key});
        for (int i = 0; i < 10000; ++i)
        {
            DataSample sample;
            sample.key = key;
            sample.value = i;
            QCOMPARE(mailbox.publish({sample}), i == 0);
        }
        QCOMPARE(mailbox.coalesced(), quint64(9999));
        const auto batch = mailbox.take();
        QCOMPARE(batch.size(), 1);
        QCOMPARE(batch[0].value, 9999.0);
        DataSample unknown;
        unknown.key = {"unknown", "point"};
        QVERIFY(!mailbox.publish({unknown}));
        QVERIFY(mailbox.publish(batch));
    }
    void dataHubRejectsOldGenerationAndSequence()
    {
        DataHub hub;
        hub.configure(demo());
        PointKey key{"field_1_a", "temperature"};
        hub.activateSource(key.sourceId, 10);
        DataSample sample{key, 10, 2, 100, monotonicMs(), 12, Quality::Good};
        hub.applyBatch({sample});
        QCOMPARE(hub.sample(key).value, 12.0);
        sample.sequence = 1;
        sample.value = 99;
        hub.applyBatch({sample});
        QCOMPARE(hub.sample(key).value, 12.0);
        hub.activateSource(key.sourceId, 11);
        sample.sequence = 100;
        hub.applyBatch({sample});
        QCOMPARE(hub.sample(key).quality, Quality::Stale);
        sample.generation = 11;
        hub.applyBatch({sample});
        QCOMPARE(hub.sample(key).value, 99.0);
        hub.invalidateSource(key.sourceId);
        sample.sequence = 101;
        hub.applyBatch({sample});
        QCOMPARE(hub.sample(key).quality, Quality::Stale);
    }
    void writesAndLocalStopAreIsolated()
    {
        DataHub hub;
        RuntimeManager runtime(hub, simulatedSessionFactory());
        ApplicationService app(runtime);
        QVERIFY(runtime.configure(demo()));
        runtime.start();
        QTRY_VERIFY(runtime.sourceAvailable("field_1_a"));
        QTRY_VERIFY(runtime.sourceAvailable("field_2_a"));
        PointKey a{"field_1_a", "setpoint"}, b{"field_2_a", "setpoint"};
        QSignalSpy results(&app, &ApplicationService::commandFinished);
        const auto id = app.submitWrite(a, 73);
        QCOMPARE(results.size(), 0);
        QTRY_COMPARE(results.size(), 1);
        QCOMPARE(resultAt(results, 0).requestId, id);
        QCOMPARE(resultAt(results, 0).outcome, CommandOutcome::Confirmed);
        QTRY_COMPARE(hub.sample(a).value, 73.0);
        QCOMPARE(hub.sample(b).value, 40.0);
        runtime.stopSession("a1");
        QVERIFY(!runtime.sourceAvailable("field_1_a"));
        QVERIFY(runtime.sourceAvailable("field_1_b"));
        QVERIFY(runtime.sourceAvailable("field_2_a"));
        QTRY_COMPARE(hub.sample(a).quality, Quality::Stale);
        const auto before = hub.sample(b).sequence;
        QTRY_VERIFY(hub.sample(b).sequence > before);
        runtime.stopConnection("sim_port_1");
        QTRY_VERIFY(!runtime.sourceAvailable("field_1_b"));
        QVERIFY(runtime.sourceAvailable("field_2_a"));
        runtime.stop();
        QTRY_VERIFY(!runtime.isRunning());
        QCOMPARE(app.pendingCount(), 0);
    }
    void timeoutIsUnknownAndNeverReplayed()
    {
        auto config = demo();
        config.connections[0].responseDelayMs = 200;
        DataHub hub;
        RuntimeManager runtime(hub, simulatedSessionFactory());
        ApplicationService app(runtime);
        runtime.configure(config);
        runtime.start();
        QTRY_VERIFY(runtime.sourceAvailable("field_1_a"));
        QSignalSpy results(&app, &ApplicationService::commandFinished);
        app.submitWrite({"field_1_a", "setpoint"}, 81, 20);
        QTRY_COMPARE(results.size(), 1);
        QCOMPARE(resultAt(results, 0).outcome, CommandOutcome::Unknown);
        QTest::qWait(250);
        QCOMPARE(results.size(), 1);
        QCOMPARE(hub.sample({"field_1_a", "setpoint"}).value, 81.0);
        runtime.stop();
        QTRY_VERIFY(!runtime.isRunning());
    }
    void stopSettlesAllAcceptedCommands()
    {
        auto config = demo();
        config.connections[0].responseDelayMs = 1000;
        DataHub hub;
        RuntimeManager runtime(hub, simulatedSessionFactory());
        ApplicationService app(runtime);
        runtime.configure(config);
        runtime.start();
        QTRY_VERIFY(runtime.sourceAvailable("field_1_a"));
        QSignalSpy results(&app, &ApplicationService::commandFinished);
        QSet<QString> submitted;
        for (int i = 0; i < 100; ++i)
            submitted.insert(app.submitWrite({"field_1_a", "setpoint"}, i));
        QVERIFY(app.pendingCount() <= 64);
        QTest::qWait(30);
        runtime.stop();
        QTRY_VERIFY(!runtime.isRunning());
        QTRY_COMPARE(results.size(), 100);
        QSet<QString> completed;
        bool unknown = false, cancelled = false, rejected = false;
        for (qsizetype i = 0; i < results.size(); ++i)
        {
            const auto result = resultAt(results, i);
            QVERIFY(!completed.contains(result.requestId));
            completed.insert(result.requestId);
            unknown |= result.outcome == CommandOutcome::Unknown;
            cancelled |= result.outcome == CommandOutcome::Cancelled;
            rejected |= result.outcome == CommandOutcome::Rejected;
        }
        QCOMPARE(completed, submitted);
        QVERIFY(unknown && cancelled && rejected);
        QCOMPARE(app.pendingCount(), 0);
    }
    void restartInvalidatesOldSamples()
    {
        DataHub hub;
        RuntimeManager runtime(hub, simulatedSessionFactory());
        runtime.configure(demo());
        runtime.start();
        PointKey key{"field_1_a", "temperature"};
        QTRY_COMPARE(hub.sample(key).quality, Quality::Good);
        auto old = hub.sample(key);
        runtime.stop();
        QTRY_VERIFY(!runtime.isRunning());
        runtime.start();
        QTRY_VERIFY(hub.sample(key).generation != old.generation);
        const auto generation = hub.sample(key).generation;
        old.sequence += 1000;
        old.value = 999;
        hub.applyBatch({old});
        QCOMPARE(hub.sample(key).generation, generation);
        QVERIFY(hub.sample(key).value != 999);
        runtime.stop();
        QTRY_VERIFY(!runtime.isRunning());
    }
};
QTEST_GUILESS_MAIN(RuntimeTest)
#include "test_runtime.moc"
