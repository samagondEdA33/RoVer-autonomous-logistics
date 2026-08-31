# RoVer ROS 2 + ESP32 Control Demo

This directory contains a compact demonstration based on the working RoVer control stack.

It is intended for technical review and GitHub presentation and does not replace the complete production runtime.

## Overview

The demo shows the basic control chain used by the RoVer autonomous robotic platform:

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
Motor Drivers
        |
        v
6-Wheel Drivetrain
