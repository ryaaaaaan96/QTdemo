#pragma once
#include "core/protocol_session.h"
#include <QTimer>
#include <optional>

namespace industrial
{
SessionFactory simulatedSessionFactory();
class SimulatedSession final : public ProtocolSession
{
    Q_OBJECT
  public:
    SimulatedSession(const SessionConfig &session, const QVector<SourceConfig> &sources,
                     quint64 generation, int responseDelayMs, QObject *parent);
    void start() override;
    void stop() override;
    void submit(const CommandRequest &request) override;
    void abandon(const QString &requestId) override;

  private:
    struct SourceState
    {
        SourceConfig config;
        QVector<double> values;
        quint64 sequence = 0;
        QTimer *timer = nullptr;
    };
    void publish(qsizetype sourceIndex, bool advance);
    QVector<SourceState> m_sources;
    bool m_counter = false;
    bool m_running = false;
    quint64 m_generation;
    QTimer *m_replyTimer;
    std::optional<CommandRequest> m_pending;
};
} // namespace industrial
