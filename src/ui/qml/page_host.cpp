#include "page_host.h"
#include <QAbstractItemModel>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QQuickWidget>
#include <QUrlQuery>
#include <QUuid>
#include <QVBoxLayout>

namespace industrial
{
PageHost::PageHost(QAbstractItemModel &model, QWidget *parent)
    : QWidget(parent), m_model(model), m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    QQmlEngine::setObjectOwnership(&model, QQmlEngine::CppOwnership);
}
void PageHost::load(const QUrl &url)
{
    if (m_candidate)
    {
        m_candidate->disconnect(this);
        delete m_candidate;
    }
    auto *candidate = new QQuickWidget(this);
    m_candidate = candidate;
    candidate->hide();
    candidate->setResizeMode(QQuickWidget::SizeRootObjectToView);
    candidate->rootContext()->setContextProperty("pointModel", &m_model);
    connect(candidate, &QQuickWidget::statusChanged, this,
            [this, candidate, url](QQuickWidget::Status status)
            {
                if (candidate != m_candidate)
                    return;
                if (status == QQuickWidget::Error)
                {
                    QStringList messages;
                    for (const auto &error : candidate->errors())
                        messages.append(error.toString());
                    m_candidate = nullptr;
                    candidate->deleteLater();
                    emit loadFailed(messages.join('\n'));
                }
                else if (status == QQuickWidget::Ready)
                {
                    auto *old = m_current;
                    m_current = candidate;
                    m_candidate = nullptr;
                    m_url = url;
                    if (old)
                    {
                        m_layout->removeWidget(old);
                        old->hide();
                        old->deleteLater();
                    }
                    m_layout->addWidget(candidate);
                    candidate->show();
                    emit loaded();
                }
            });
    // A distinct URL prevents a still-live older engine from supplying the previous
    // entry component. Relative assets continue to resolve against the original folder.
    auto loadUrl = url;
    if (url.isLocalFile())
    {
        QUrlQuery query(loadUrl);
        query.addQueryItem("pageRevision", QUuid::createUuid().toString(QUuid::WithoutBraces));
        loadUrl.setQuery(query);
    }
    candidate->setSource(loadUrl);
}
} // namespace industrial
