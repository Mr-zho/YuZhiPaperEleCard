#!/usr/bin/env python3
"""Send a UTF-8 name to the ESP32 e-paper BLE receiver."""

from __future__ import annotations

import argparse
import asyncio
import struct
import sys
from typing import Optional


DEFAULT_DEVICE_NAME = "ePaper-BLE"
SERVICE_UUID = "0000fff0-0000-1000-8000-00805f9b34fb"
RX_CHAR_UUID = "0000fff2-0000-1000-8000-00805f9b34fb"

PROTO_MAGIC = 0xA5
PROTO_VERSION = 0x01
TYPE_TEXT = 0x02


def build_text_frame(text: str) -> bytes:
    payload = text.encode("utf-8")
    header = struct.pack(
        "<BBBBHHI",
        PROTO_MAGIC,
        PROTO_VERSION,
        TYPE_TEXT,
        0,  # flags
        0,  # seq
        1,  # total
        len(payload),
    )
    return header + payload


def load_bleak():
    try:
        from bleak import BleakClient, BleakScanner
    except ImportError:
        print("Missing dependency: bleak. Install it with: python -m pip install -r tools/requirements-ble.txt", file=sys.stderr)
        raise SystemExit(1)

    return BleakClient, BleakScanner


async def find_device(scanner, name: str, address: Optional[str], timeout: float):
    if address:
        print(f"Connecting by address/id: {address}")
        return address

    print(f"Scanning for BLE device named {name!r} ({timeout:.1f}s)...")
    devices = await scanner.discover(timeout=timeout, return_adv=True)

    for device, adv in devices.values():
        if device.name == name or adv.local_name == name:
            print(f"Found {name!r}: {device.address}")
            return device

    candidates = []
    for device, adv in devices.values():
        advertised_services = {uuid.lower() for uuid in adv.service_uuids}
        if SERVICE_UUID in advertised_services:
            candidates.append(device)

    if len(candidates) == 1:
        device = candidates[0]
        print(f"Found service UUID on {device.name or 'unnamed'}: {device.address}")
        return device

    seen = [
        f"{device.name or adv.local_name or 'unnamed'} [{device.address}]"
        for device, adv in devices.values()
    ]
    raise RuntimeError(
        "BLE device not found. "
        f"Looked for name {name!r}. Seen devices: {', '.join(seen) or 'none'}"
    )


async def send_name(args: argparse.Namespace) -> None:
    client_class, scanner_class = load_bleak()
    frame = build_text_frame(args.text)
    device = await find_device(scanner_class, args.name, args.address, args.scan_timeout)

    print(f"Connecting...")
    async with client_class(device, timeout=args.connect_timeout) as client:
        if not client.is_connected:
            raise RuntimeError("BLE connection failed")

        print(f"Connected. Sending {len(frame)} bytes: {args.text!r}")
        response = not args.write_without_response

        for offset in range(0, len(frame), args.chunk_size):
            chunk = frame[offset : offset + args.chunk_size]
            await client.write_gatt_char(RX_CHAR_UUID, chunk, response=response)
            if args.chunk_delay > 0:
                await asyncio.sleep(args.chunk_delay)

        print("Sent. Watch the ESP32 serial log and e-paper refresh.")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Send a name to the ESP32 10.85-inch e-paper seat card over BLE."
    )
    parser.add_argument("text", help="Name/text to display, for example: 张三")
    parser.add_argument("--name", default=DEFAULT_DEVICE_NAME, help="BLE device name to scan for")
    parser.add_argument("--address", help="Connect directly by BLE address/id instead of scanning by name")
    parser.add_argument("--scan-timeout", type=float, default=8.0, help="BLE scan timeout in seconds")
    parser.add_argument("--connect-timeout", type=float, default=15.0, help="BLE connect timeout in seconds")
    parser.add_argument("--chunk-size", type=int, default=20, help="Bytes per BLE write")
    parser.add_argument("--chunk-delay", type=float, default=0.02, help="Delay between chunks in seconds")
    parser.add_argument(
        "--write-without-response",
        action="store_true",
        help="Use GATT Write Without Response instead of Write Request",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.chunk_size <= 0:
        print("--chunk-size must be greater than 0", file=sys.stderr)
        return 2

    try:
        asyncio.run(send_name(args))
        return 0
    except KeyboardInterrupt:
        return 130
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
