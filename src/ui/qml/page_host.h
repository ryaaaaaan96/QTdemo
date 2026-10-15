#pragma once
#include <QUrl>
#include <QWidget>

class QAbstractItemModel;
class QQuickWidget;
class QVBoxLayout;
namespace industrial
{
// This milestone exposes only read-only models. Each view owns a separate QML engine.
class PageHost final : public QWidget
{
    Q_OBJECT
  public:
    explicit PageHost(QAbstractItemModel &model, QWidget *parent = nullptr);
    void load(const QUrl &url);
    bool hasPage() const { return m_current != nullptr; }
    QUrl currentUrl() const { return m_url; }
  signals:
    void loaded();
    void loadFailed(const QString &message);

  private:
    QAbstractItemModel &m_model;
    QVBoxLayout *m_layout;
    QQuickWidget *m_current = nullptr;
    QQuickWidget *m_candidate = nullptr;
    QUrl m_url;
};
} // namespace industrial
