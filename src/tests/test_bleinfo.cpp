// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>
#include "ble/bleinfo.h"

class TestBleInfo : public QObject
{
    Q_OBJECT

private slots:
    void worn();
    void busyElsewhere_data();
    void busyElsewhere();
};

void TestBleInfo::worn()
{
    BleInfo d;
    QVERIFY(!ble::isWorn(d));                            // neither pod
    d.isPrimaryInEar = true;
    QVERIFY(ble::isWorn(d));                             // primary only
    d.isPrimaryInEar = false; d.isSecondaryInEar = true;
    QVERIFY(ble::isWorn(d));                             // secondary only
    d.isPrimaryInEar = true;
    QVERIFY(ble::isWorn(d));                             // both
}

void TestBleInfo::busyElsewhere_data()
{
    QTest::addColumn<int>("state");
    QTest::addColumn<bool>("busy");
    using CS = BleInfo::ConnectionState;
    QTest::newRow("disconnected") << int(CS::DISCONNECTED) << false;
    QTest::newRow("idle")         << int(CS::IDLE)         << false;
    QTest::newRow("music")        << int(CS::MUSIC)        << true;
    QTest::newRow("call")         << int(CS::CALL)         << true;
    QTest::newRow("ringing")      << int(CS::RINGING)      << true;
    QTest::newRow("hanging_up")   << int(CS::HANGING_UP)   << true;
    QTest::newRow("unknown")      << int(CS::UNKNOWN)      << false;
}

void TestBleInfo::busyElsewhere()
{
    QFETCH(int, state);
    QFETCH(bool, busy);
    BleInfo d;
    d.connectionState = static_cast<BleInfo::ConnectionState>(state);
    QCOMPARE(ble::isBusyElsewhere(d), busy);
}

QTEST_MAIN(TestBleInfo)
#include "test_bleinfo.moc"
