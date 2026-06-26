#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Tier-2 integration test: verifies BluetoothMonitor::connectDevice() issues
# org.bluez.Device1.Connect() on the matching device, using a mock BlueZ from
# python-dbusmock (no real Bluetooth hardware).
#
# Run standalone:
#     CONNECTDEVICE_HARNESS=./connectdevice_harness \
#         python3 test_connectdevice_dbusmock.py
# or via ctest (wired up in src/CMakeLists.txt when LINUXPODS_BUILD_TESTS=ON).
#
# Requires: python3-dbusmock, python3-dbus. Skips cleanly if missing.

import os
import subprocess
import sys
import unittest

try:
    import dbus
    from dbusmock import DBusTestCase
except ImportError:
    print("SKIP: python3-dbusmock / python3-dbus not installed")
    sys.exit(0)

HARNESS = os.environ.get("CONNECTDEVICE_HARNESS", "./connectdevice_harness")
MAC = "AA:BB:CC:DD:EE:FF"
OTHER_MAC = "11:22:33:44:55:66"


class TestConnectDevice(DBusTestCase):
    @classmethod
    def setUpClass(cls):
        cls.start_system_bus()
        cls.dbus_con = cls.get_dbus(system_bus=True)

    def setUp(self):
        # bluez5 template provides org.bluez with an ObjectManager at "/".
        (self.p_mock, self.obj_bluez) = self.spawn_server_template(
            "bluez5", {}, stdout=subprocess.DEVNULL
        )
        self.obj_bluez.AddAdapter("hci0", "test-host")

    def tearDown(self):
        self.p_mock.terminate()
        self.p_mock.wait()

    def _add_device(self, mac, alias="AirPods"):
        return str(self.obj_bluez.AddDevice("hci0", mac, alias))

    def _connect_calls(self, path):
        # dbusmock records every method call; query org.bluez.Device1.Connect.
        mock = dbus.Interface(
            self.dbus_con.get_object("org.bluez", path), "org.freedesktop.DBus.Mock"
        )
        return mock.GetMethodCalls("Connect")

    def _run_harness(self, mac):
        # The harness inherits DBUS_SYSTEM_BUS_ADDRESS from start_system_bus().
        return subprocess.call([HARNESS, mac], timeout=15)

    def test_connect_is_issued(self):
        path = self._add_device(MAC)
        self.assertEqual(len(self._connect_calls(path)), 0, "precondition: no Connect() yet")

        self.assertEqual(self._run_harness(MAC), 0, "harness exited cleanly")

        # connectDevice() must issue exactly one org.bluez.Device1.Connect().
        # We assert on the recorded call rather than the Connected property: the
        # bluez5 mock emits PropertiesChanged but doesn't sync Connected for Get.
        self.assertEqual(len(self._connect_calls(path)), 1,
                         "connectDevice() should issue exactly one Connect()")

    def test_match_is_case_insensitive(self):
        # BlueZ stores the address upper-case; we ask with a lower-case MAC.
        path = self._add_device(MAC)
        self.assertEqual(self._run_harness(MAC.lower()), 0)
        self.assertEqual(len(self._connect_calls(path)), 1,
                         "MAC matching should be case-insensitive")

    def test_no_matching_device_is_noop(self):
        # Only a non-matching device exists: must not crash or connect it.
        other = self._add_device(OTHER_MAC)
        self.assertEqual(self._run_harness(MAC), 0, "harness exits cleanly when absent")
        self.assertEqual(len(self._connect_calls(other)), 0)

    def test_only_matching_device_connects(self):
        match = self._add_device(MAC)
        other = self._add_device(OTHER_MAC)
        self.assertEqual(self._run_harness(MAC), 0)
        self.assertEqual(len(self._connect_calls(match)), 1)
        self.assertEqual(len(self._connect_calls(other)), 0, "only the matching device")


if __name__ == "__main__":
    unittest.main()
