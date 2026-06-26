// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>
#include "service/autoconnectpolicy.h"

using AutoConnect::Behavior;
using AutoConnect::Decision;
using AutoConnect::Inputs;

class TestAutoConnectPolicy : public QObject
{
    Q_OBJECT

private slots:
    void decision_data();
    void decision();
    void engine_sequence();
    void engine_cooldown();
    void engine_disconnect_reset();
};

// Default obs: WhenWorn, idle phone, has address, not connected, not playing.
static AutoConnect::Observation obs(bool inEar, bool isConnected = false)
{
    AutoConnect::Observation o;
    o.behavior = Behavior::WhenWorn;
    o.inEar = inEar;
    o.isConnected = isConnected;
    o.phoneBusy = false;
    o.localPlaying = false;
    o.hasAddress = true;
    return o;
}

void TestAutoConnectPolicy::decision_data()
{
    QTest::addColumn<int>("behavior");
    QTest::addColumn<bool>("inEar");
    QTest::addColumn<bool>("lastInEar");
    QTest::addColumn<bool>("isConnected");
    QTest::addColumn<bool>("phoneBusy");
    QTest::addColumn<bool>("localPlaying");
    QTest::addColumn<bool>("cooldownActive");
    QTest::addColumn<bool>("hasAddress");
    QTest::addColumn<int>("expected");

    const int Off  = int(Behavior::Off);
    const int Worn = int(Behavior::WhenWorn);
    const int Play = int(Behavior::WhenWornAndPlaying);

    // columns:                       beh | inEar last conn busy play cool addr | expected
    QTest::newRow("disabled")
        << Off  << true  << false << false << false << false << false << true  << int(Decision::SkipDisabled);
    QTest::newRow("no-edge: still in")
        << Worn << true  << true  << false << false << false << false << true  << int(Decision::SkipNoRisingEdge);
    QTest::newRow("no-edge: not worn")
        << Worn << false << false << false << false << false << false << true  << int(Decision::SkipNoRisingEdge);
    QTest::newRow("already connected")
        << Worn << true  << false << true  << false << false << false << true  << int(Decision::SkipAlreadyConnected);
    QTest::newRow("busy on phone")
        << Worn << true  << false << false << true  << false << false << true  << int(Decision::SkipBusyElsewhere);
    QTest::newRow("cooldown")
        << Worn << true  << false << false << false << false << true  << true  << int(Decision::SkipCooldown);
    QTest::newRow("no address")
        << Worn << true  << false << false << false << false << false << false << int(Decision::SkipNoAddress);
    QTest::newRow("connect: when worn")
        << Worn << true  << false << false << false << false << false << true  << int(Decision::Connect);
    QTest::newRow("when-playing: nothing playing")
        << Play << true  << false << false << false << false << false << true  << int(Decision::SkipNotPlaying);
    QTest::newRow("when-playing: playing -> connect")
        << Play << true  << false << false << false << true  << false << true  << int(Decision::Connect);

    // Precedence checks (multiple skip conditions hold; earliest wins).
    QTest::newRow("precedence: busy beats cooldown")
        << Worn << true  << false << false << true  << false << true  << true  << int(Decision::SkipBusyElsewhere);
    QTest::newRow("precedence: connected beats busy")
        << Worn << true  << false << true  << true  << false << false << true  << int(Decision::SkipAlreadyConnected);
    QTest::newRow("precedence: cooldown beats no-address")
        << Worn << true  << false << false << false << false << true  << false << int(Decision::SkipCooldown);
}

void TestAutoConnectPolicy::decision()
{
    QFETCH(int, behavior);
    QFETCH(bool, inEar);
    QFETCH(bool, lastInEar);
    QFETCH(bool, isConnected);
    QFETCH(bool, phoneBusy);
    QFETCH(bool, localPlaying);
    QFETCH(bool, cooldownActive);
    QFETCH(bool, hasAddress);
    QFETCH(int, expected);

    Inputs in;
    in.behavior       = static_cast<Behavior>(behavior);
    in.inEar          = inEar;
    in.lastInEar      = lastInEar;
    in.isConnected    = isConnected;
    in.phoneBusy      = phoneBusy;
    in.localPlaying   = localPlaying;
    in.cooldownActive = cooldownActive;
    in.hasAddress     = hasAddress;

    QCOMPARE(int(AutoConnect::decide(in)), expected);
}

// Fires once; no re-fire while worn; reinsert blocked by cooldown, then allowed.
void TestAutoConnectPolicy::engine_sequence()
{
    using D = Decision;
    AutoConnect::Engine e;
    QCOMPARE(e.step(obs(true),      0), D::Connect);            // put on
    QCOMPARE(e.step(obs(true),    100), D::SkipNoRisingEdge);   // still worn
    QCOMPARE(e.step(obs(false),  5000), D::SkipNoRisingEdge);   // removed
    QCOMPARE(e.step(obs(true),  10000), D::SkipCooldown);       // reinsert < 15s
    QCOMPARE(e.step(obs(false), 20000), D::SkipNoRisingEdge);   // removed
    QCOMPARE(e.step(obs(true),  21000), D::Connect);            // reinsert > 15s
}

// Cooldown boundary: blocked at <15000ms since the last attempt, allowed at >=.
void TestAutoConnectPolicy::engine_cooldown()
{
    using D = Decision;
    AutoConnect::Engine e;
    QCOMPARE(e.step(obs(true),   1000), D::Connect);           // attempt @1000
    QCOMPARE(e.step(obs(false),  2000), D::SkipNoRisingEdge);
    QCOMPARE(e.step(obs(true),  15000), D::SkipCooldown);      // 14000ms elapsed
    QCOMPARE(e.step(obs(false), 16000), D::SkipNoRisingEdge);
    QCOMPARE(e.step(obs(true),  16001), D::Connect);           // 15001ms elapsed
}

// After a deliberate disconnect, only a real reinsert re-grabs.
void TestAutoConnectPolicy::engine_disconnect_reset()
{
    using D = Decision;
    AutoConnect::Engine e;
    QCOMPARE(e.step(obs(true,  false),     0), D::Connect);              // put on
    // Reinsert while connected (a real rising edge) -> already connected.
    QCOMPARE(e.step(obs(false, true),    100), D::SkipNoRisingEdge);     // removed
    QCOMPARE(e.step(obs(true,  true),    200), D::SkipAlreadyConnected); // reinserted
    e.onDisconnected();
    QCOMPARE(e.step(obs(true,  false), 30000), D::SkipNoRisingEdge);     // no re-grab
    QCOMPARE(e.step(obs(false, false), 30100), D::SkipNoRisingEdge);     // removed
    QCOMPARE(e.step(obs(true,  false), 30200), D::Connect);              // reinserted
}

QTEST_MAIN(TestAutoConnectPolicy)
#include "test_autoconnectpolicy.moc"
