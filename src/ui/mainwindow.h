#pragma once
#include "services/runtime/application_service.h"
#include "ui/models/point_table_model.h"
#include <QMainWindow>

class QComboBox;
class QDoubleSpinBox;
class QPlainTextEdit;
class QPushButton;
class QTableView;
class QTabWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT
  public:
    MainWindow(industrial::DataHub &hub, industrial::RuntimeManager &runtime,
               industrial::ApplicationService &service, QWidget *parent = nullptr);
    bool loadConfiguration(const QString &fileName);
    bool pageReady() const;

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    void updateSelection();
    void addPage(const QUrl &url);
    void appendLog(const QString &message);
    industrial::RuntimeManager &m_runtime;
    industrial::ApplicationService &m_service;
    industrial::PointTableModel *m_model;
    QTableView *m_table;
    QPlainTextEdit *m_log = nullptr;
    QComboBox *m_connections;
    QComboBox *m_sessions;
    QDoubleSpinBox *m_writeValue;
    QPushButton *m_writeButton;
    QPushButton *m_loadConfig;
    QTabWidget *m_pages;
    bool m_closing = false;
};
