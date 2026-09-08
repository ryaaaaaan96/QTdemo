#include "mainwindow.h"
#include "core/greeting.h"
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("QTdemo | Qt + Clang + CMake"));
    resize(720, 420);
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(32, 32, 32, 32);
    auto *title = new QLabel(QStringLiteral("Qt desktop starter"), container);
    auto font = title->font();
    font.setPointSize(22);
    title->setFont(font);
    auto *name = new QLineEdit(container);
    name->setPlaceholderText(QStringLiteral("Your name"));
    auto *button = new QPushButton(QStringLiteral("Say hello"), container);
    auto *result = new QLabel(app::greeting({}), container);
    layout->addWidget(title);
    layout->addWidget(name);
    layout->addWidget(button);
    layout->addWidget(result);
    layout->addStretch();
    connect(button, &QPushButton::clicked, this, [name, result] {
        result->setText(app::greeting(name->text()));
    });
    setCentralWidget(container);
}
