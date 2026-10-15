#pragma once
#include "types.h"
#include <QObject>
#include <functional>

namespace industrial
{
// A session and its communication resources always live in the connection's I/O thread.
class ProtocolSession : public QObject
{
    Q_OBJECT
  public:
    using QObject::QObject;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void submit(const CommandRequest &request) = 0;
    virtual void abandon(const QString &requestId) = 0;
  signals:
    void samplesReady(const industrial::SampleBatch &batch);
    void operationFinished(const industrial::CommandResult &result);
};
using SessionFactory = std::function<ProtocolSession *(
    const SessionConfig &, const QVector<SourceConfig> &, quint64, int, QObject *)>;
} // namespace industrial
