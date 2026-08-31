# RoVer ROS 2 + ESP32 Control Demo

This directory contains selected source code from the working RoVer autonomous robotic platform.

The demo is intended for technical review and demonstrates the communication between ROS 2 and the ESP32-S3 drivetrain controller.

## Control Architecture

```text
ROS 2 /cmd_vel
        |
        v
ros2_cmd_vel_serial_bridge.py
        |
        | USB Serial, 115200 baud
        v
ESP32-S3 Wheel Controller
        |
        v
PCF8591 DAC + Direction Relays
        |
        v
Motor Drivers
        |
        v
6-Wheel Drivetrain
```

## Included Source Code

### ROS 2

`ros2_cmd_vel_serial_bridge.py`

The ROS 2 node:

- receives `/cmd_vel`
- converts linear and angular velocity into left/right drivetrain commands
- communicates with the ESP32-S3 over USB Serial
- implements command timeout protection
- performs Safe Stop when commands are lost

### ESP32-S3

`esp32_wheel_controller/src/main.cpp`

The ESP32-S3 firmware provides:

- independent left/right motor control
- PCF8591 DAC control
- forward/reverse relay control
- arm/disarm logic
- smooth throttle ramping
- command watchdog
- fault handling
- Safe Stop behavior

## Repository Structure

```text
jury_demo/
├── README.md
├── ros2_cmd_vel_serial_bridge.py
└── esp32_wheel_controller/
    ├── platformio.ini
    └── src/
        └── main.cpp
```

## Build

The ESP32-S3 firmware can be built using PlatformIO:

```bash
pio run --project-dir jury_demo/esp32_wheel_controller
```

## Safety

The public demonstration includes several safety mechanisms:

- zero throttle at startup
- explicit arm requirement
- bounded DAC output
- smooth throttle ramp
- zero throttle before direction changes
- ROS 2 command watchdog
- independent ESP32-S3 watchdog
- automatic stop on communication or I2C errors

## Project

**RoVer** is an autonomous robotic platform developed in Uzbekistan for autonomous logistics, delivery and service applications.

The platform combines ROS 2, embedded motor control, sensor integration and autonomous navigation in a modular robotic architecture.

This public repository contains selected source code from the working RoVer MVP.

Some production-specific software, configuration files and proprietary components are intentionally not published.
