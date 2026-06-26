// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BLEMANAGER_H
#define BLEMANAGER_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QMap>
#include <QString>
#include <QDateTime>
#include "enums.h"
#include "ble/bleinfo.h"

class QTimer;

// BleInfo is defined in ble/bleinfo.h (included above).

class BleManager : public QObject
{
    Q_OBJECT
public:
    explicit BleManager(QObject *parent = nullptr);
    ~BleManager();

    void startScan();
    void stopScan();
    bool isScanning() const;

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onScanFinished();
    void onErrorOccurred(QBluetoothDeviceDiscoveryAgent::Error error);

signals:
    void deviceFound(const BleInfo &device);

private:
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
};

#endif // BLEMANAGER_H