// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QtGlobal>

// Auto-connect-on-wear decision logic. Pure (no Qt/BlueZ/D-Bus) for unit testing.
namespace AutoConnect
{
    // Mirror of LinuxPodsService::AutoConnectBehavior (static_assert'd).
    enum class Behavior
    {
        Off = 0,                // never auto-connect
        WhenWorn = 1,           // connect on in-ear (unless phone busy)
        WhenWornAndPlaying = 2  // ...and only while playing locally
    };

    constexpr qint64 kCooldownMs = 15000;   // min gap between attempts (anti-flap)

    enum class Decision
    {
        Connect,                // page the AirPods now
        SkipDisabled,           // feature turned off
        SkipNoRisingEdge,       // not an out -> in-ear transition
        SkipAlreadyConnected,   // already on this host
        SkipBusyElsewhere,      // actively in use on the phone — don't steal
        SkipNotPlaying,         // WhenWornAndPlaying, but nothing playing locally
        SkipCooldown,           // attempted too recently
        SkipNoAddress           // no known classic BT address to connect to
    };

    struct Inputs
    {
        Behavior behavior;
        bool inEar;          // either pod currently in an ear
        bool lastInEar;      // previous in-ear reading (for rising-edge detection)
        bool isConnected;    // AirPods already connected to this host
        bool phoneBusy;      // BLE connectionState in {MUSIC,CALL,RINGING,HANGING_UP}
        bool localPlaying;   // audio playing on this host right now
        bool cooldownActive; // a connect was attempted within kCooldownMs
        bool hasAddress;     // a classic BT address is known for the AirPods
    };

    // Pure decision: same Inputs -> same Decision.
    Decision decide(const Inputs &in);

    // Per-advertisement facts; Engine adds its remembered state.
    struct Observation
    {
        Behavior behavior;
        bool inEar;
        bool isConnected;
        bool phoneBusy;
        bool localPlaying;
        bool hasAddress;
    };

    // Stateful wrapper over decide(): tracks rising edge + cooldown (clock injected).
    class Engine
    {
    public:
        Decision step(const Observation &obs, qint64 nowMs);
        void onDisconnected();   // re-arm conservatively after a disconnect

    private:
        bool m_lastInEar = false;
        qint64 m_lastAttemptMs = -1;   // -1 = none yet
    };
}
