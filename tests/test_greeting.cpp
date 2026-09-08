#include "core/greeting.h"
#include <QtTest>

class GreetingTest : public QObject
{
    Q_OBJECT
private slots:
    void emptyName() { QCOMPARE(app::greeting("  "), QString("Hello, Qt!")); }
    void trimmedName() { QCOMPARE(app::greeting("  Developer  "), QString("Hello, Developer!")); }
};
QTEST_GUILESS_MAIN(GreetingTest)
#include "test_greeting.moc"
