#pragma once
#include "runtime_manager.h"
#include <QSet>

namespace industrial
{
class ApplicationService final : public QObject
{
    Q_OBJECT
  public:
    explicit ApplicationService(RuntimeManager &runtime, QObject *parent = nullptr);
    QString submitWrite(const PointKey &key, double value, int timeoutMs = 3000);
    qsizetype pendingCount() const { return m_pending.size(); }
  signals:
    void commandFinished(const industrial::CommandResult &result);

  private:
    RuntimeManager &m_runtime;
    QSet<QString> m_pending;
};
} // namespace industrial
