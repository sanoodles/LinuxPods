// SPDX-License-Identifier: GPL-3.0-or-later

#include "service/autoconnectpolicy.h"

namespace AutoConnect
{
    Decision decide(const Inputs &in)
    {
        // First matching guard wins (determines the reported reason).
        if (in.behavior == Behavior::Off)
            return Decision::SkipDisabled;

        const bool risingEdge = in.inEar && !in.lastInEar;   // out -> in-ear
        if (!risingEdge)
            return Decision::SkipNoRisingEdge;

        if (in.isConnected)
            return Decision::SkipAlreadyConnected;

        if (in.phoneBusy)   // in use on the phone — don't steal
            return Decision::SkipBusyElsewhere;

        if (in.behavior == Behavior::WhenWornAndPlaying && !in.localPlaying)
            return Decision::SkipNotPlaying;

        if (in.cooldownActive)
            return Decision::SkipCooldown;

        if (!in.hasAddress)
            return Decision::SkipNoAddress;

        return Decision::Connect;
    }

    Decision Engine::step(const Observation &obs, qint64 nowMs)
    {
        const bool cooldownActive = m_lastAttemptMs >= 0
                                    && (nowMs - m_lastAttemptMs) < kCooldownMs;

        Inputs in;
        in.behavior       = obs.behavior;
        in.inEar          = obs.inEar;
        in.lastInEar      = m_lastInEar;
        in.isConnected    = obs.isConnected;
        in.phoneBusy      = obs.phoneBusy;
        in.localPlaying   = obs.localPlaying;
        in.cooldownActive = cooldownActive;
        in.hasAddress     = obs.hasAddress;

        m_lastInEar = obs.inEar;   // keep edge state warm

        const Decision d = decide(in);
        if (d == Decision::Connect)
            m_lastAttemptMs = nowMs;
        return d;
    }

    void Engine::onDisconnected()
    {
        // Assume worn, so a deliberate disconnect needs a real reinsert to re-grab.
        m_lastInEar = true;
    }
}
