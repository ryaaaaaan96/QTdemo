#include "mainwindow.h"
#include "services/config/config_repository.h"
#include "ui/qml/page_host.h"
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

using namespace industrial;
namespace
{
const QUrl dashboardUrl("qrc:/qt/qml/PlatformUi/Dashboard.qml");
}

MainWindow::MainWindow(DataHub &hub, RuntimeManager &runtime, ApplicationService &service,
                       QWidget *parent)
    : QMainWindow(parent), m_runtime(runtime), m_service(service),
      m_model(new PointTableModel(hub, this))
{
    setWindowTitle(QStringLiteral("工业数据工作台 · 模拟运行"));
    resize(1220, 760);
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(20, 16, 20, 16);
    auto *title = new QLabel(QStringLiteral("多协议采集工作台"), container);
    auto titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);
    layout->addWidget(new QLabel(
        QStringLiteral("模拟模式：所有数值来自软件模拟器，不连接真实设备。"), container));
    auto *controls = new QHBoxLayout;
    auto *start = new QPushButton(QStringLiteral("启动连接"), container);
    start->setToolTip(QStringLiteral("启动已停止的连接；恢复单个会话请先停止并重新启动其连接。"));
    auto *stop = new QPushButton(QStringLiteral("全部停止"), container);
    m_loadConfig = new QPushButton(QStringLiteral("打开配置…"), container);
    controls->addWidget(start);
    controls->addWidget(stop);
    controls->addWidget(m_loadConfig);
    controls->addStretch();
    layout->addLayout(controls);
    auto *localControls = new QHBoxLayout;
    m_connections = new QComboBox(container);
    m_sessions = new QComboBox(container);
    auto *stopConnection = new QPushButton(QStringLiteral("停止连接"), container);
    auto *stopSession = new QPushButton(QStringLiteral("停止会话"), container);
    localControls->addWidget(m_connections);
    localControls->addWidget(stopConnection);
    localControls->addSpacing(18);
    localControls->addWidget(m_sessions);
    localControls->addWidget(stopSession);
    localControls->addStretch();
    layout->addLayout(localControls);

    auto *splitter = new QSplitter(container);
    auto *left = new QWidget(splitter);
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 8, 0);
    m_table = new QTableView(left);
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    leftLayout->addWidget(m_table, 3);
    auto *writeRow = new QHBoxLayout;
    writeRow->addWidget(new QLabel(QStringLiteral("选中可写点位后设置："), left));
    m_writeValue = new QDoubleSpinBox(left);
    m_writeValue->setDecimals(2);
    m_writeButton = new QPushButton(QStringLiteral("写入模拟设备"), left);
    m_writeButton->setEnabled(false);
    writeRow->addWidget(m_writeValue);
    writeRow->addWidget(m_writeButton);
    writeRow->addStretch();
    leftLayout->addLayout(writeRow);
    m_log = new QPlainTextEdit(left);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(200);
    leftLayout->addWidget(m_log, 1);
    auto *right = new QWidget(splitter);
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    auto *pageControls = new QHBoxLayout;
    auto *reloadPage = new QPushButton(QStringLiteral("重载当前页面"), right);
    auto *openPage = new QPushButton(QStringLiteral("新增 QML 页面…"), right);
    pageControls->addWidget(reloadPage);
    pageControls->addWidget(openPage);
    rightLayout->addLayout(pageControls);
    m_pages = new QTabWidget(right);
    m_pages->setTabsClosable(true);
    rightLayout->addWidget(m_pages);
    splitter->setSizes({820, 400});
    layout->addWidget(splitter, 1);
    setCentralWidget(container);
    addPage(dashboardUrl);

    connect(start, &QPushButton::clicked, this,
            [this]
            {
                if (!m_closing)
                    m_runtime.start();
            });
    connect(stop, &QPushButton::clicked, &m_runtime, &RuntimeManager::stop);
    connect(stopConnection, &QPushButton::clicked, this,
            [this] { m_runtime.stopConnection(m_connections->currentText()); });
    connect(stopSession, &QPushButton::clicked, this,
            [this] { m_runtime.stopSession(m_sessions->currentText()); });
    connect(m_loadConfig, &QPushButton::clicked, this,
            [this]
            {
                const auto path = QFileDialog::getOpenFileName(
                    this, QStringLiteral("打开模拟项目配置"), {}, "JSON (*.json)");
                if (!path.isEmpty())
                    loadConfiguration(path);
            });
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &MainWindow::updateSelection);
    connect(m_writeButton, &QPushButton::clicked, this,
            [this]
            {
                const auto key = m_model->keyAt(m_table->currentIndex().row());
                const auto id = m_service.submitWrite(key, m_writeValue->value());
                appendLog(QStringLiteral("提交 %1 / %2 · %3").arg(key.sourceId, key.pointId, id));
            });
    connect(&m_service, &ApplicationService::commandFinished, this,
            [this](const CommandResult &result) {
                appendLog(outcomeText(result.outcome) + " · " + result.requestId + " · " +
                          result.detail);
            });
    connect(&m_runtime, &RuntimeManager::connectionStateChanged, this,
            [this](const QString &id, const QString &state)
            {
                appendLog(id + " → " + state);
                m_loadConfig->setEnabled(!m_runtime.isRunning());
                updateSelection();
            });
    connect(&m_runtime, &RuntimeManager::sessionStopped, this,
            [this](const QString &id)
            {
                appendLog(id + QStringLiteral(" 已停止；同连接的其他会话继续运行"));
                updateSelection();
            });
    connect(&m_runtime, &RuntimeManager::allStopped, this,
            [this]
            {
                if (m_closing)
                    QTimer::singleShot(0, this, &QWidget::close);
            });
    connect(reloadPage, &QPushButton::clicked, this,
            [this]
            {
                auto *host = qobject_cast<PageHost *>(m_pages->currentWidget());
                if (host)
                    host->load(host->currentUrl().isEmpty() ? dashboardUrl : host->currentUrl());
            });
    connect(openPage, &QPushButton::clicked, this,
            [this]
            {
                const auto path = QFileDialog::getOpenFileName(
                    this, QStringLiteral("打开本地 QML 页面"), {}, "QML (*.qml)");
                if (!path.isEmpty())
                    addPage(QUrl::fromLocalFile(path));
            });
    connect(m_pages, &QTabWidget::tabCloseRequested, this,
            [this](int index)
            {
                auto *page = m_pages->widget(index);
                m_pages->removeTab(index);
                page->deleteLater();
            });
    statusBar()->showMessage(QStringLiteral("原型阶段 · 通信状态独立于界面页面"));
}
bool MainWindow::loadConfiguration(const QString &fileName)
{
    if (m_runtime.isRunning())
        return false;
    const auto result = ConfigRepository::load(fileName);
    if (!result.config)
    {
        appendLog(QStringLiteral("配置未应用：") + result.errors.join("; "));
        return false;
    }
    if (!m_runtime.configure(*result.config))
        return false;
    m_model->configure(*result.config);
    m_connections->clear();
    m_sessions->clear();
    for (const auto &connection : result.config->connections)
        m_connections->addItem(connection.id);
    for (const auto &session : result.config->sessions)
        m_sessions->addItem(session.id);
    appendLog(QStringLiteral("已加载配置：") + fileName);
    m_runtime.start();
    return true;
}
void MainWindow::updateSelection()
{
    const auto row = m_table->currentIndex().row();
    const auto definition = m_model->definitionAt(row);
    m_writeValue->setRange(definition.minimum, definition.maximum);
    m_writeButton->setEnabled(!m_closing && definition.writable &&
                              m_runtime.sourceAvailable(m_model->keyAt(row).sourceId));
}
void MainWindow::addPage(const QUrl &url)
{
    auto *host = new PageHost(*m_model, m_pages);
    const int index = m_pages->addTab(host, QStringLiteral("页面 %1").arg(m_pages->count() + 1));
    connect(host, &PageHost::loadFailed, this,
            [this](const QString &message)
            { appendLog(QStringLiteral("页面加载失败，已有页面保留：") + message); });
    connect(host, &PageHost::loaded, this, [this] { appendLog(QStringLiteral("QML 页面已加载")); });
    host->load(url);
    m_pages->setCurrentIndex(index);
}
bool MainWindow::pageReady() const
{
    auto *host = qobject_cast<PageHost *>(m_pages->currentWidget());
    return host && host->hasPage();
}
void MainWindow::appendLog(const QString &message)
{
    if (m_log)
        m_log->appendPlainText(QTime::currentTime().toString("HH:mm:ss") + "  " + message);
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_runtime.isRunning())
    {
        event->accept();
        return;
    }
    m_closing = true;
    centralWidget()->setEnabled(false);
    statusBar()->showMessage(QStringLiteral("正在停止采集并释放工作线程…"));
    m_runtime.stop();
    event->ignore();
}
