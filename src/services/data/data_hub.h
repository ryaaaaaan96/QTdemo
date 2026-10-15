#pragma once
#include "core/types.h"
#include <QObject>
#include <QTimer>

namespace industrial
{
class DataHub final : public QObject
{
    Q_OBJECT
  public:
    explicit DataHub(QObject *parent = nullptr);
    void configure(const ProjectConfig &config);
    void activateSource(const QString &sourceId, quint64 generation);
    void invalidateSource(const QString &sourceId);
    void applyBatch(const SampleBatch &batch);
    DataSample sample(const PointKey &key) const { return m_samples.value(key); }
  signals:
    void changed();

  private:
    QHash<PointKey, DataSample> m_samples;
    QHash<QString, quint64> m_generations;
    QHash<QString, int> m_staleAfterMs;
    QTimer m_staleTimer;
};
} // namespace industrial
