#!/usr/bin/env python3
"""ROS 2 to ESP32 wheel-control bridge for the RoVer project.

This public demonstration converts ROS 2 /cmd_vel commands into
left/right drivetrain commands and sends them to the ESP32-S3
motor controller over USB Serial.
"""

from __future__ import annotations

import math
import threading
import time

import rclpy
import serial
from geometry_msgs.msg import Twist
from rclpy.node import Node
from std_msgs.msg import String


DEFAULT_SERIAL_PORT = "/dev/ttyUSB0"
DEFAULT_SAFE_MAX_RAW = 96
MAX_APPROVED_RAW = 193


def finite_or_zero(value: float) -> float:
    value = float(value)
    return value if math.isfinite(value) else 0.0


def sign(value: int) -> int:
    if value < 0:
        return -1
    if value > 0:
        return 1
    return 0


def norm_to_raw(value: float, max_raw: int, deadband: float) -> int:
    if abs(value) < deadband:
        return 0

    value = max(-1.0, min(1.0, value))
    return int(round(value * max_raw))


def twist_to_tank_raw(
    linear_mps: float,
    angular_radps: float,
    max_raw: int,
    linear_full_scale_mps: float,
    angular_full_scale_radps: float,
    deadband: float,
) -> tuple[int, int]:
    """Convert ROS linear/angular velocity into left/right commands."""

    linear = finite_or_zero(linear_mps)
    angular = finite_or_zero(angular_radps)

    max_raw = max(0, min(MAX_APPROVED_RAW, int(max_raw)))

    linear_scale = max(0.05, float(linear_full_scale_mps))
    angular_scale = max(0.05, float(angular_full_scale_radps))

    left_norm = (linear / linear_scale) - (angular / angular_scale)
    right_norm = (linear / linear_scale) + (angular / angular_scale)

    scale = max(
        1.0,
        abs(left_norm),
        abs(right_norm),
    )

    left_norm /= scale
    right_norm /= scale

    return (
        norm_to_raw(left_norm, max_raw, deadband),
        norm_to_raw(right_norm, max_raw, deadband),
    )


def relay_commands(left_raw: int, right_raw: int) -> list[str]:
    """Return relay states required for the requested wheel directions."""

    if left_raw < 0 and right_raw < 0:
        return ["relay both 1"]

    if left_raw >= 0 and right_raw >= 0:
        return ["relay both 0"]

    if left_raw < 0:
        return [
            "relay left 1",
            "relay right 0",
        ]

    return [
        "relay right 1",
        "relay left 0",
    ]


class RoverSerialBridge(Node):
    def __init__(self) -> None:
        super().__init__("rover_serial_bridge")

        self.declare_parameter(
            "serial_port",
            DEFAULT_SERIAL_PORT,
        )

        self.declare_parameter(
            "baud",
            115200,
        )

        self.declare_parameter(
            "enable_motion",
            False,
        )

        self.declare_parameter(
            "max_raw",
            DEFAULT_SAFE_MAX_RAW,
        )

        self.declare_parameter(
            "linear_full_scale_mps",
            0.5,
        )

        self.declare_parameter(
            "angular_full_scale_radps",
            1.2,
        )

        self.declare_parameter(
            "deadband",
            0.04,
        )

        self.declare_parameter(
            "command_timeout_sec",
            0.6,
        )

        port = str(
            self.get_parameter("serial_port").value
        )

        baud = int(
            self.get_parameter("baud").value
        )

        self.enable_motion = bool(
            self.get_parameter("enable_motion").value
        )

        self.max_raw = max(
            0,
            min(
                MAX_APPROVED_RAW,
                int(self.get_parameter("max_raw").value),
            ),
        )

        self.linear_full_scale_mps = max(
            0.05,
            float(
                self.get_parameter(
                    "linear_full_scale_mps"
                ).value
            ),
        )

        self.angular_full_scale_radps = max(
            0.05,
            float(
                self.get_parameter(
                    "angular_full_scale_radps"
                ).value
            ),
        )

        self.deadband = max(
            0.0,
            float(
                self.get_parameter(
                    "deadband"
                ).value
            ),
        )

        self.command_timeout_sec = max(
            0.2,
            float(
                self.get_parameter(
                    "command_timeout_sec"
                ).value
            ),
        )

        self.serial_lock = threading.Lock()

        self.serial = serial.Serial(
            port,
            baud,
            timeout=0,
            write_timeout=0.5,
        )

        self.rx_buffer = bytearray()

        self.active = False
        self.last_signs: tuple[int, int] | None = None
        self.last_cmd_at = 0.0

        # Opening USB Serial may reset the ESP32.
        time.sleep(1.2)

        self.serial.reset_input_buffer()

        self.safe_stop(
            "startup",
            force=True,
        )

        self.status_pub = self.create_publisher(
            String,
            "/rover/esp_serial",
            10,
        )

        self.cmd_sub = self.create_subscription(
            Twist,
            "/cmd_vel",
            self.on_cmd_vel,
            10,
        )

        self.watchdog_timer = self.create_timer(
            0.1,
            self.watchdog_tick,
        )

        self.serial_timer = self.create_timer(
            0.05,
            self.read_serial,
        )

        state = (
            "enabled"
            if self.enable_motion
            else "disabled"
        )

        self.get_logger().info(
            f"ESP32 connected at {port} @ {baud}; "
            f"motion forwarding is {state}"
        )


    def send_lines(self, *commands: str) -> None:
        with self.serial_lock:
            for command in commands:
                packet = (
                    f"{command.strip()}\n"
                ).encode("ascii")

                self.serial.write(packet)

            self.serial.flush()


    def start_drive(
        self,
        left_raw: int,
        right_raw: int,
    ) -> None:

        commands = [
            *relay_commands(
                left_raw,
                right_raw,
            ),
            f"max {self.max_raw}",
            "arm",
            f"drive {abs(left_raw)} {abs(right_raw)}",
        ]

        self.send_lines(*commands)


    def keep_drive(
        self,
        left_raw: int,
        right_raw: int,
    ) -> None:

        self.send_lines(
            f"drive {abs(left_raw)} {abs(right_raw)}"
        )


    def on_cmd_vel(
        self,
        msg: Twist,
    ) -> None:

        if not self.enable_motion:
            if self.active:
                self.safe_stop(
                    "motion parameter disabled"
                )
            return

        left_raw, right_raw = twist_to_tank_raw(
            msg.linear.x,
            msg.angular.z,
            self.max_raw,
            self.linear_full_scale_mps,
            self.angular_full_scale_radps,
            self.deadband,
        )

        if left_raw == 0 and right_raw == 0:
            self.safe_stop(
                "zero cmd_vel"
            )
            return

        signs = (
            sign(left_raw),
            sign(right_raw),
        )

        try:

            if (
                not self.active
                or signs != self.last_signs
            ):
                self.start_drive(
                    left_raw,
                    right_raw,
                )
            else:
                self.keep_drive(
                    left_raw,
                    right_raw,
                )

        except (
            OSError,
            serial.SerialException,
        ) as exc:

            self.active = False
            self.last_signs = None

            self.get_logger().error(
                f"Serial drive command failed: {exc}"
            )

            return

        self.active = True
        self.last_signs = signs
        self.last_cmd_at = time.monotonic()

        self.get_logger().info(
            "cmd_vel linear=%.3f angular=%.3f "
            "-> left=%d right=%d"
            % (
                msg.linear.x,
                msg.angular.z,
                left_raw,
                right_raw,
            )
        )


    def watchdog_tick(self) -> None:

        if (
            self.active
            and time.monotonic()
            - self.last_cmd_at
            > self.command_timeout_sec
        ):
            self.safe_stop(
                "ROS cmd_vel timeout"
            )


    def safe_stop(
        self,
        reason: str,
        *,
        force: bool = False,
    ) -> None:

        if not force and not self.active:
            return

        try:

            self.send_lines(
                "zero",
                "drive 0 0",
                "relay both 0",
                "disarm",
                f"max {DEFAULT_SAFE_MAX_RAW}",
            )

            self.get_logger().info(
                f"Safe Stop: {reason}"
            )

        except (
            OSError,
            serial.SerialException,
        ) as exc:

            self.get_logger().error(
                f"Safe Stop serial failure: {exc}"
            )

        finally:

            self.active = False
            self.last_signs = None
            self.last_cmd_at = 0.0


    def read_serial(self) -> None:

        try:

            with self.serial_lock:

                waiting = self.serial.in_waiting

                if waiting:
                    self.rx_buffer.extend(
                        self.serial.read(waiting)
                    )

        except (
            OSError,
            serial.SerialException,
        ) as exc:

            self.get_logger().error(
                f"Serial read failed: {exc}"
            )

            return

        while b"\n" in self.rx_buffer:

            raw_line, _, remainder = (
                self.rx_buffer.partition(b"\n")
            )

            self.rx_buffer = bytearray(
                remainder
            )

            line = (
                raw_line
                .rstrip(b"\r")
                .decode(
                    "utf-8",
                    errors="replace",
                )
            )

            if line:
                self.status_pub.publish(
                    String(data=line)
                )


    def close(self) -> None:

        self.safe_stop(
            "ROS node shutdown",
            force=True,
        )

        with self.serial_lock:

            if self.serial.is_open:
                self.serial.close()


def main(args=None) -> None:

    rclpy.init(args=args)

    node = RoverSerialBridge()

    try:
        rclpy.spin(node)

    except KeyboardInterrupt:
        pass

    finally:

        node.close()
        node.destroy_node()

        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()
