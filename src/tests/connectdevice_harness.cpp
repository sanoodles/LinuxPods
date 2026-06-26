// SPDX-License-Identifier: GPL-3.0-or-later
//
// Tier-2 runner: calls BluetoothMonitor::connectDevice(<MAC>) against a
// python-dbusmock BlueZ. Driven by test_connectdevice_dbusmock.py.

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QTimer>

#include "BluetoothMonitor.h"

// BluetoothMonitor.cpp logs via this category (normally defined in the daemon).
Q_LOGGING_CATEGORY(linuxpods, "linuxpods")

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc < 2)
    {
        qWarning("usage: connectdevice_harness <MAC>");
        return 2;
    }

    BluetoothMonitor monitor;
    monitor.connectDevice(QString::fromUtf8(argv[1]));

    // Give the async Connect() call time to land on the (mock) bus.
    QTimer::singleShot(800, &app, &QCoreApplication::quit);
    return app.exec();
}
