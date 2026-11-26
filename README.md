# RoVer — Autonomous Delivery on Mapped Routes

We build Rover6 in Uzbekistan to carry orders between pickup and handoff points along mapped routes. Food delivery is our first use case. We’re looking for a café partner for our first paid pilot. The same platform can also be adapted for routine transport in warehouses and industrial facilities.

[English presentation (PDF)](docs/rover-overview-en.pdf) · [Prototype demonstrations](docs/demonstrations.md) · [ROS 2 and ESP32 demo](jury_demo/README.md)

<p align="center">
  <img src="media/1.jpg" width="850" alt="Rover6 Autonomous Mobile Robot">
</p>

## 🎬 Rover6 in Motion

https://github.com/user-attachments/assets/35bc0364-f688-46a4-8c9d-c0f6164b4cb8

*Development prototype.* Rover6 drives away from the camera during an outdoor test on asphalt.

[View all prototype demonstrations](docs/demonstrations.md).


---

## 🚀 Project Status

Rover6 is currently at the **working MVP stage**.

The robot is physically assembled and capable of autonomous navigation between points after initial mapping of the operating environment.

Current capabilities include:

- Autonomous mapping and localization
- Autonomous point-to-point navigation
- Route planning
- Wheel odometry
- LiDAR-based environment perception
- Depth-camera perception
- IMU-based motion data
- ROS 2 integration
- Emergency Stop system
- On-board computing
- Independent left/right drivetrain control

---

## 🤖 Hardware Platform

Rover6 is based on a six-wheel mobile robotic platform.
## 📸 Rover6 Platform

<p align="center">
  <img src="media/1.jpg" width="48%">
  <img src="media/2.jpg" width="48%">
</p>

<p align="center">
  <img src="media/3.jpg" width="48%">
  <img src="media/4.jpg" width="48%">
</p>

See the [workshop hardware overview](docs/demonstrations.md#hardware-overview) for a closer look at the open-body development prototype and six-wheel chassis.

Main components include:

- 6-wheel electric drivetrain
- Wheel encoders
- ESP32-S3 low-level controller
- On-board high-performance computer
- RPLIDAR
- Intel RealSense depth camera
- 9-axis IMU
- Motor drivers
- Battery-powered architecture
- Emergency Stop system

Current platform weight is approximately **50 kg**.

The mechanical architecture is designed for further testing with payloads exceeding **100 kg**.

---

## 🧪 Field Testing

### Curb Traversal

https://github.com/user-attachments/assets/64afe0c8-857d-4f76-8a03-fc134440ae34

*Development prototype.* The rover crosses a low concrete curb from asphalt to soil, showing the six-wheel chassis adapting to the height change.

Explore the [complete demonstration gallery](docs/demonstrations.md) for additional workshop, driving and load demonstrations.

---

## 🧠 Software Architecture

For implementation details, see the [ROS 2 and ESP32-S3 control demo](jury_demo/README.md), which includes selected source code for the velocity-command bridge, drivetrain controller and communication watchdogs.

The autonomous navigation stack is based on:

- Ubuntu Linux
- ROS 2
- SLAM
- Localization
- Navigation and path planning
- Sensor integration
- Wheel odometry
- LiDAR processing
- Depth-camera data
- IMU data
- ESP32 motor-control interface

High-level architecture:

Sensor Data  
↓  
Localization & Mapping  
↓  
Route Planning  
↓  
Autonomous Navigation  
↓  
Velocity Commands  
↓  
ESP32-S3  
↓  
Motor Drivers  
↓  
6-Wheel Drivetrain

---

## 🗺 Autonomous Navigation

During the initial deployment, Rover6 maps its operating environment.

After the map is created, the robot can localize itself and autonomously navigate between specified points without continuous human control.

This makes Rover6 suitable for repetitive logistics operations in structured industrial environments.

---

## 🏭 Target Applications

Rover6 is being developed for:

- Manufacturing facilities
- Warehouses
- Logistics centers
- Industrial campuses
- Internal cargo transportation
- Material movement between production areas
- Autonomous delivery within controlled facilities

---

## 🎯 Project Goal

The goal of Rover6 is to reduce the cost and human workload associated with repetitive internal logistics operations.

The platform is being developed as a locally engineered autonomous logistics solution for Uzbekistan with future expansion potential across Central Asia.

---

## 📌 Development Stage

**Stage:** MVP

The main hardware and software systems have been integrated and autonomous navigation is operational.

Current development focus:

- Reliability testing
- Payload testing
- Obstacle-handling validation
- Industrial pilot preparation
- Commercialization

Watch the [prototype load demonstration](docs/demonstrations.md#load-demonstration) and [driving and maneuvering test](docs/demonstrations.md#driving-and-maneuvering).

---

## 🇺🇿 Developed in Uzbekistan

Rover6 is designed and developed in Uzbekistan as an autonomous robotics platform for industrial logistics.

---

## 🔒 Repository Notice

This public repository contains selected project information and demonstration materials.

Some production source code, hardware configurations and proprietary components are intentionally not published.

---

## About RoVer

We are a team from Uzbekistan building an autonomous robot that carries orders along mapped routes.

[View the English presentation (PDF)](docs/rover-overview-en.pdf)

### How We Started

We started with the chassis. We designed and built a six-wheel platform in Uzbekistan, then refined the mechanics, navigation, and controls over six versions. Along the way, we completed around 200 test deliveries. **Rover6** is our current working prototype, with autonomous navigation and a dedicated cargo compartment.

<p align="center">
  <img src="media/rover-build-frame.jpg" height="280" alt="Early RoVer frame on a workshop bench">
  <img src="media/rover-build-bodywork.jpg" height="280" alt="A RoVer team member working on a metal panel">
</p>

[Watch us build RoVer](https://www.youtube.com/shorts/TMK4AAX9Rsg).

### Our Team

- **Nursultan** handles mechanical design, parts fabrication, assembly, and testing.
- **Abduazim** develops the software, navigation, and robot control systems.
- **Arslan** brings experience in robotics and works on project development.

<table>
  <tr>
    <td align="center"><img src="media/team-nursultan.png" height="162" alt="Portrait of Nursultan"><br><strong>Nursultan</strong></td>
    <td align="center"><img src="media/team-abduazim.png" width="32%" alt="Portrait of Abduazim"> <img src="media/team-abduazim-rover-1.png" width="32%" alt="Abduazim on Rover6 showing a peace sign"> <img src="media/team-abduazim-rover-2.png" width="32%" alt="Abduazim on Rover6 looking at the camera"><br><strong>Abduazim</strong></td>
  </tr>
  <tr>
    <td align="center"><img src="media/team-arslan.png" height="162" alt="Portrait of Arslan"><br><strong>Arslan</strong></td>
    <td align="center"><img src="media/team-arslan-2.png" height="162" alt="Second portrait of Arslan"><br><strong>Arslan</strong></td>
  </tr>
</table>

### Where We're Going

We want to make autonomous delivery practical for everyday use in Uzbekistan. Our first use case is food delivery from a pickup point to a handoff point. We map the route in advance so Rover6 can carry the order autonomously. We're improving the prototype so it can make these trips reliably day after day.
