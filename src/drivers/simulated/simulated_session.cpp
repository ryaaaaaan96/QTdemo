#include "simulated_session.h"
#include <QDateTime>
#include <QThread>
#include <cmath>

namespace industrial
{
SessionFactory simulatedSessionFactory()
{
    return [](const SessionConfig &session, const QVector<SourceConfig> &sources,
              quint64 generation, int responseDelayMs, QObject *parent) -> ProtocolSession *
    { return new SimulatedSession(session, sources, generation, responseDelayMs, parent); };
}
SimulatedSession::SimulatedSession(const SessionConfig &session,
                                   const QVector<SourceConfig> &sources, quint64 generation,
                                   int responseDelayMs, QObject *parent)
    : ProtocolSession(parent), m_counter(session.protocolType == "simulated_counter"),
      m_generation(generation), m_replyTimer(new QTimer(this))
{
    for (const auto &source : sources)
    {
        SourceState state;
        state.config = source;
        for (const auto &point : source.points)
            state.values.append(point.initial);
        state.timer = new QTimer(this);
        state.timer->setInterval(source.intervalMs);
        const qsizetype index = m_sources.size();
        connect(state.timer, &QTimer::timeout, this, [this, index] { publish(index, true); });
        m_sources.append(state);
    }
    m_replyTimer->setSingleShot(true);
    m_replyTimer->setInterval(responseDelayMs);
    connect(m_replyTimer, &QTimer::timeout, this,
            [this]
            {
                if (!m_pending)
                    return;
                const QString id = m_pending->requestId;
                m_pending.reset();
                emit operationFinished({id, CommandOutcome::Confirmed,
                                        QStringLiteral("模拟设备已确认；采样值独立更新")});
            });
}
void SimulatedSession::start()
{
    Q_ASSERT(thread() == QThread::currentThread());
    if (m_running)
        return;
    m_running = true;
    for (qsizetype i = 0; i < m_sources.size(); ++i)
    {
        publish(i, false);
        m_sources[i].timer->start();
    }
}
void SimulatedSession::stop()
{
    m_running = false;
    for (auto &source : m_sources)
        source.timer->stop();
    m_replyTimer->stop();
    m_pending.reset();
}
void SimulatedSession::publish(qsizetype sourceIndex, bool advance)
{
    auto &source = m_sources[sourceIndex];
    if (!m_running)
        return;
    ++source.sequence;
    SampleBatch batch;
    for (qsizetype i = 0; i < source.config.points.size(); ++i)
    {
        const auto &point = source.config.points[i];
        double &value = source.values[i];
        if (advance && m_counter)
        {
            value += point.step;
            if (value > point.maximum)
                value = point.minimum;
            if (value < point.minimum)
                value = point.maximum;
        }
        batch.append({{source.config.id, point.id},
                      m_generation,
                      source.sequence,
                      QDateTime::currentMSecsSinceEpoch(),
                      monotonicMs(),
                      value,
                      Quality::Good});
    }
    emit samplesReady(batch);
}
void SimulatedSession::submit(const CommandRequest &request)
{
    if (!m_running || m_pending || request.generation != m_generation)
    {
        emit operationFinished(
            {request.requestId, CommandOutcome::Rejected, "Session unavailable"});
        return;
    }
    for (auto &source : m_sources)
    {
        if (source.config.id != request.key.sourceId)
            continue;
        for (qsizetype i = 0; i < source.config.points.size(); ++i)
        {
            const auto &point = source.config.points[i];
            if (point.id != request.key.pointId)
                continue;
            if (!point.writable || !std::isfinite(request.value) || request.value < point.minimum ||
                request.value > point.maximum)
                break;
            // Simulate device execution before acknowledgement. Losing the acknowledgement
            // must not pretend to undo a potentially executed operation.
            source.values[i] = request.value;
            m_pending = request;
            m_replyTimer->start();
            return;
        }
    }
    emit operationFinished({request.requestId, CommandOutcome::Rejected, "Invalid point or value"});
}
void SimulatedSession::abandon(const QString &requestId)
{
    if (!m_pending || m_pending->requestId != requestId)
        return;
    m_replyTimer->stop();
    m_pending.reset();
}
} // namespace industrial
