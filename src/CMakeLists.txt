# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

project(linuxpods VERSION 1.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fedora packaging: no RPATH in installed binaries — system libs resolve via ld.so.
set(CMAKE_SKIP_INSTALL_RPATH ON)
set(CMAKE_SKIP_BUILD_RPATH ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
set(QT_DISABLE_RPATH ON)

# Build the standalone Qt6 + QML GUI (deprecated; prefer the Plasma 6 widget
# in plasmoid/ together with linuxpods-daemon). Distribution packagers should
# pass -DLINUXPODS_BUILD_GUI=OFF.
option(LINUXPODS_BUILD_GUI "Build standalone Qt6 GUI front-end" ON)

if(LINUXPODS_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Quick Widgets Bluetooth DBus LinguistTools)
    find_package(KF6StatusNotifierItem REQUIRED)
else()
    find_package(Qt6 REQUIRED COMPONENTS Core Bluetooth DBus)
endif()
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSEAUDIO REQUIRED libpulse)

qt_standard_project_setup()

include(GNUInstallDirs)

# ── Standalone Qt6/QML GUI (linuxpods-gui) ───────────────────────────────
if(LINUXPODS_BUILD_GUI)
    set(TS_FILES translations/linuxpods-gui_tr.ts)

    qt_add_executable(linuxpods-gui
        main.cpp
        logger.h
        service/linuxpodsservice.cpp
        service/linuxpodsservice.h
        service/autoconnectpolicy.cpp
        service/autoconnectpolicy.h
        dbus/linuxpodsdbusadaptor.h
        media/mediacontroller.cpp
        media/mediacontroller.h
        media/pulseaudiocontroller.cpp
        media/pulseaudiocontroller.h
        airpods_packets.h
        trayiconmanager.cpp
        trayiconmanager.h
        enums.h
        battery.hpp
        BluetoothMonitor.cpp
        BluetoothMonitor.h
        autostartmanager.hpp
        BasicControlCommand.hpp
        deviceinfo.hpp
        ble/bleutils.cpp
        ble/bleutils.h
        ble/blemanager.cpp
        ble/blemanager.h
        thirdparty/QR-Code-generator/qrcodegen.cpp
        thirdparty/QR-Code-generator/qrcodegen.hpp
        QRCodeImageProvider.hpp
        eardetection.hpp
        media/playerstatuswatcher.cpp
        media/playerstatuswatcher.h
        systemsleepmonitor.hpp
    )

    qt_add_qml_module(linuxpods-gui
        URI linux
        VERSION 1.0
        QML_FILES
            Main.qml
            BatteryIndicator.qml
            SegmentedControl.qml
            PodColumn.qml
            Icon.qml
            KeysQRDialog.qml
            SettingsPage.qml
            HearingAidPage.qml
            SettingRow.qml
    )

    qt_add_resources(linuxpods-gui "resources"
        PREFIX "/icons"
        FILES
            assets/airpods.png
            assets/pod.png
            assets/pod_case.png
            assets/pod3.png
            assets/pod3_case.png
            assets/pod4_case.png
            assets/podpro.png
            assets/podpro_case.png
            assets/podmax.png
    )

    target_link_libraries(linuxpods-gui
        PRIVATE Qt6::Quick Qt6::Widgets Qt6::Bluetooth Qt6::DBus
                KF6::StatusNotifierItem
                OpenSSL::SSL OpenSSL::Crypto ${PULSEAUDIO_LIBRARIES}
    )
    target_include_directories(linuxpods-gui PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})

    set_target_properties(linuxpods-gui PROPERTIES
        INSTALL_RPATH ""
        BUILD_WITH_INSTALL_RPATH OFF
        SKIP_BUILD_RPATH ON
    )

    install(TARGETS linuxpods-gui
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(FILES assets/io.github.Explor3Universe.LinuxPods.desktop
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
    install(FILES assets/linuxpods-gui.svg
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps")
    install(FILES assets/io.github.Explor3Universe.LinuxPods.metainfo.xml
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo")

    qt_add_translations(linuxpods-gui
        TS_FILES ${TS_FILES}
        QM_FILES_OUTPUT_VARIABLE QM_FILES
    )
    install(FILES ${QM_FILES}
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/linuxpods-gui/translations")
endif()

# ── Command-line control tool (linuxpods) ────────────────────────
qt_add_executable(linuxpods
    linuxpods.cpp
)
target_link_libraries(linuxpods
    PRIVATE Qt6::Core Qt6::Network
)
set_target_properties(linuxpods PROPERTIES
    INSTALL_RPATH ""
    BUILD_WITH_INSTALL_RPATH OFF
    SKIP_BUILD_RPATH ON
)
install(TARGETS linuxpods
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ── Headless daemon (linuxpods-daemon) ───────────────────────────────
# Shares all backend sources with linuxpods-gui but has no QML/GUI.
set(DAEMON_SOURCES
    daemon/main.cpp
    service/linuxpodsservice.cpp
    service/linuxpodsservice.h
    service/autoconnectpolicy.cpp
    service/autoconnectpolicy.h
    dbus/linuxpodsdbusadaptor.h
    media/mediacontroller.cpp
    media/mediacontroller.h
    media/pulseaudiocontroller.cpp
    media/pulseaudiocontroller.h
    airpods_packets.h
    enums.h
    battery.hpp
    BluetoothMonitor.cpp
    BluetoothMonitor.h
    autostartmanager.hpp
    BasicControlCommand.hpp
    deviceinfo.hpp
    ble/bleutils.cpp
    ble/bleutils.h
    ble/blemanager.cpp
    ble/blemanager.h
    thirdparty/QR-Code-generator/qrcodegen.cpp
    thirdparty/QR-Code-generator/qrcodegen.hpp
    eardetection.hpp
    media/playerstatuswatcher.cpp
    media/playerstatuswatcher.h
    systemsleepmonitor.hpp
)
qt_add_executable(linuxpods-daemon ${DAEMON_SOURCES})
target_include_directories(linuxpods-daemon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PULSEAUDIO_INCLUDE_DIRS})
target_link_libraries(linuxpods-daemon
    PRIVATE Qt6::Core Qt6::Bluetooth Qt6::DBus
            OpenSSL::SSL OpenSSL::Crypto ${PULSEAUDIO_LIBRARIES}
)
set_target_properties(linuxpods-daemon PROPERTIES
    INSTALL_RPATH ""
    BUILD_WITH_INSTALL_RPATH OFF
    SKIP_BUILD_RPATH ON
)
install(TARGETS linuxpods-daemon
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ── Unit tests (opt-in: -DLINUXPODS_BUILD_TESTS=ON) ───────────────────
option(LINUXPODS_BUILD_TESTS "Build unit tests" OFF)
if(LINUXPODS_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)

    qt_add_executable(test_autoconnectpolicy
        tests/test_autoconnectpolicy.cpp
        service/autoconnectpolicy.cpp
        service/autoconnectpolicy.h
    )
    target_include_directories(test_autoconnectpolicy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_autoconnectpolicy PRIVATE Qt6::Core Qt6::Test)
    set_target_properties(test_autoconnectpolicy PROPERTIES
        INSTALL_RPATH "" BUILD_WITH_INSTALL_RPATH OFF SKIP_BUILD_RPATH ON)
    add_test(NAME autoconnectpolicy COMMAND test_autoconnectpolicy)

    # BLE field interpretation (isWorn / isBusyElsewhere) — header-only.
    qt_add_executable(test_bleinfo tests/test_bleinfo.cpp)
    target_include_directories(test_bleinfo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_bleinfo PRIVATE Qt6::Core Qt6::Test)
    set_target_properties(test_bleinfo PROPERTIES
        INSTALL_RPATH "" BUILD_WITH_INSTALL_RPATH OFF SKIP_BUILD_RPATH ON)
    add_test(NAME bleinfo COMMAND test_bleinfo)

    # ── Tier 2: connectDevice() integration test (mock BlueZ) ────────────
    # Harness binary calls BluetoothMonitor::connectDevice(); the Python test
    # drives it against python-dbusmock. The test self-skips (exit 0) when
    # python3-dbusmock isn't installed, so it's safe in the default run.
    qt_add_executable(connectdevice_harness
        tests/connectdevice_harness.cpp
        BluetoothMonitor.cpp
        BluetoothMonitor.h
    )
    target_include_directories(connectdevice_harness PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(connectdevice_harness PRIVATE Qt6::Core Qt6::DBus)
    set_target_properties(connectdevice_harness PROPERTIES
        INSTALL_RPATH "" BUILD_WITH_INSTALL_RPATH OFF SKIP_BUILD_RPATH ON)

    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(NAME connectdevice_dbusmock
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_connectdevice_dbusmock.py)
        set_tests_properties(connectdevice_dbusmock PROPERTIES
            ENVIRONMENT "CONNECTDEVICE_HARNESS=$<TARGET_FILE:connectdevice_harness>")
    endif()
endif()

# ── Locate side data and plasmoid trees ──────────────────────────────
# When building locally with `cmake -S src -B build`, they are at ../data.
# When building from RPM tarball, they are siblings: data/ and plasmoid/.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../data/io.github.Explor3Universe.LinuxPods.service")
    set(DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../data")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/io.github.Explor3Universe.LinuxPods.service")
    set(DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../plasmoid/metadata.json")
    set(PLASMOID_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../plasmoid")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plasmoid/metadata.json")
    set(PLASMOID_DIR "${CMAKE_CURRENT_SOURCE_DIR}/plasmoid")
endif()

# D-Bus session activation + systemd user unit
if(DATA_DIR)
    install(FILES "${DATA_DIR}/io.github.Explor3Universe.LinuxPods.service"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/dbus-1/services")
    install(FILES "${DATA_DIR}/linuxpods-daemon.service"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/systemd/user")
    # NOTE: user presets are NOT installed — Fedora policy reserves preset
    # files for the redhat-systemd-presets package exclusively.
endif()

# ── Plasma 6 plasmoid ────────────────────────────────────────────────
if(PLASMOID_DIR)
    install(DIRECTORY "${PLASMOID_DIR}/"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/plasma/plasmoids/io.github.Explor3Universe.LinuxPods"
        FILES_MATCHING
        PATTERN "*.json"
        PATTERN "*.qml"
    )
endif()
