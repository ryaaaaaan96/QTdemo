#pragma once

#include <QHash>
#include <QMetaType>
#include <QString>
#include <QVector>
#include <chrono>
#include <utility>

namespace industrial
{
inline qint64 monotonicMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

struct PointKey
{
    QString sourceId;
    QString pointId;
    bool operator==(const PointKey &) const = default;
};
inline size_t qHash(const PointKey &key, size_t seed = 0)
{
    return qHashMulti(seed, key.sourceId, key.pointId);
}

struct PointDefinition
{
    QString id;
    QString name;
    QString unit;
    bool writable = false;
    double minimum = 0;
    double maximum = 65535;
    double initial = 0;
    double step = 0;
};
struct SourceConfig
{
    QString id;
    QString sessionId;
    int intervalMs = 250;
    QVector<PointDefinition> points;
};
struct SessionConfig
{
    QString id;
    QString connectionId;
    QString protocolType;
};
struct ConnectionConfig
{
    QString id;
    int responseDelayMs = 60;
};
struct ProjectConfig
{
    QVector<ConnectionConfig> connections;
    QVector<SessionConfig> sessions;
    QVector<SourceConfig> sources;
};

enum class Quality
{
    NoData,
    Good,
    Stale
};
struct DataSample
{
    PointKey key;
    quint64 generation = 0;
    quint64 sequence = 0;
    qint64 receivedAtMs = 0;
    qint64 receivedMonoMs = 0;
    double value = 0;
    Quality quality = Quality::NoData;
};
using SampleBatch = QVector<DataSample>;

enum class CommandOutcome
{
    Confirmed,
    Rejected,
    Expired,
    Cancelled,
    Unknown
};
struct CommandRequest
{
    QString requestId;
    PointKey key;
    QString sessionId;
    quint64 generation = 0;
    qint64 deadlineMonoMs = 0;
    double value = 0;
};
struct CommandResult
{
    QString requestId;
    CommandOutcome outcome = CommandOutcome::Rejected;
    QString detail;
};
QString outcomeText(CommandOutcome outcome);
} // namespace industrial

Q_DECLARE_METATYPE(industrial::DataSample)
Q_DECLARE_METATYPE(industrial::SampleBatch)
Q_DECLARE_METATYPE(industrial::CommandRequest)
Q_DECLARE_METATYPE(industrial::CommandResult)
