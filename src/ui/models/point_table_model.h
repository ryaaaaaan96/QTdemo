#pragma once
#include "services/data/data_hub.h"
#include <QAbstractTableModel>

namespace industrial
{
class PointTableModel final : public QAbstractTableModel
{
    Q_OBJECT
  public:
    enum Role
    {
        SourceRole = Qt::UserRole + 1,
        NameRole,
        ValueRole,
        UnitRole,
        QualityRole
    };
    explicit PointTableModel(DataHub &hub, QObject *parent = nullptr);
    void configure(const ProjectConfig &config);
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    PointKey keyAt(int row) const;
    PointDefinition definitionAt(int row) const;

  private:
    struct Row
    {
        PointKey key;
        PointDefinition definition;
    };
    QVector<Row> m_rows;
    DataHub &m_hub;
};
} // namespace industrial
