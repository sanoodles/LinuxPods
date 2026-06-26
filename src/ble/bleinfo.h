// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QString>
#include <QDateTime>
#include <QtGlobal>
#include "enums.h"

// Decoded fields from an AirPods BLE advertisement. Kept separate from
// BleManager (a QObject) so it can be used and tested without dragging in
// QtBluetooth or moc.
class BleInfo
{
public:
    QString name;
    QString address;
    int leftPodBattery = -1; // -1 indicates not available
    int rightPodBattery = -1;
    int caseBattery = -1;
    bool leftCharging = false;
    bool rightCharging = false;
    bool caseCharging = false;
    AirpodsTrayApp::Enums::AirPodsModel modelName = AirpodsTrayApp::Enums::AirPodsModel::Unknown;
    quint8 lidOpenCounter = 0;
    QString color = "Unknown"; // Default color
    quint8 status = 0;
    QByteArray rawData;
    QByteArray encryptedPayload; // 16 bytes of encrypted payload

    // Additional status flags from Kotlin version
    bool isLeftPodInEar = false;
    bool isRightPodInEar = false;
    bool isPrimaryInEar = false;
    bool isSecondaryInEar = false;
    bool isLeftPodMicrophone = false;
    bool isRightPodMicrophone = false;
    bool isThisPodInTheCase = false;
    bool isOnePodInCase = false;
    bool areBothPodsInCase = false;
    bool primaryLeft = true; // True if left pod is primary, false if right pod is primary

    // Lid state enumeration
    enum class LidState
    {
        OPEN = 0x0,
        CLOSED = 0x1,
        UNKNOWN,
    } lidState = LidState::UNKNOWN;

    // Connection state enumeration
    enum class ConnectionState : uint8_t
    {
        DISCONNECTED = 0x00,
        IDLE = 0x04,
        MUSIC = 0x05,
        CALL = 0x06,
        RINGING = 0x07,
        HANGING_UP = 0x09,
        UNKNOWN = 0xFF // Using 0xFF for representing null in the original
    } connectionState = ConnectionState::UNKNOWN;

    QDateTime lastSeen; // Timestamp of last detection
};

namespace ble
{
    // Worn if either pod is in an ear.
    inline bool isWorn(const BleInfo &d)
    {
        return d.isPrimaryInEar || d.isSecondaryInEar;
    }

    // Actively in use on another device, so we shouldn't grab the AirPods.
    inline bool isBusyElsewhere(const BleInfo &d)
    {
        using CS = BleInfo::ConnectionState;
        switch (d.connectionState)
        {
        case CS::MUSIC:
        case CS::CALL:
        case CS::RINGING:
        case CS::HANGING_UP:
            return true;
        default:
            return false;
        }
    }
}
