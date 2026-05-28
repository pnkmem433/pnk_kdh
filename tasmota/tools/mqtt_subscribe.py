import argparse
import os
import random
import socket
import struct
import sys
import time


def _encode_varint(value: int) -> bytes:
    out = bytearray()
    while True:
        encoded = value % 128
        value //= 128
        if value > 0:
            encoded |= 0x80
        out.append(encoded)
        if value == 0:
            break
    return bytes(out)


def _encode_string(text: str) -> bytes:
    data = text.encode("utf-8")
    return struct.pack("!H", len(data)) + data


def _recv_exact(sock: socket.socket, size: int) -> bytes:
    data = bytearray()
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise ConnectionError("Socket closed while receiving data")
        data.extend(chunk)
    return bytes(data)


def _recv_packet(sock: socket.socket) -> tuple[int, bytes]:
    header = _recv_exact(sock, 1)[0]
    multiplier = 1
    remaining = 0
    while True:
        encoded = _recv_exact(sock, 1)[0]
        remaining += (encoded & 0x7F) * multiplier
        if (encoded & 0x80) == 0:
            break
        multiplier *= 128
    payload = _recv_exact(sock, remaining)
    return header, payload


def _build_connect(client_id: str, username: str | None, password: str | None, keepalive: int) -> bytes:
    variable = bytearray()
    variable.extend(_encode_string("MQTT"))
    variable.append(4)  # MQTT 3.1.1

    flags = 0x02  # clean session
    if username:
        flags |= 0x80
    if password:
        flags |= 0x40
    variable.append(flags)
    variable.extend(struct.pack("!H", keepalive))

    payload = bytearray()
    payload.extend(_encode_string(client_id))
    if username:
        payload.extend(_encode_string(username))
    if password:
        payload.extend(_encode_string(password))

    remaining = _encode_varint(len(variable) + len(payload))
    return bytes([0x10]) + remaining + bytes(variable) + bytes(payload)


def _build_subscribe(packet_id: int, topics: list[str]) -> bytes:
    payload = bytearray()
    for topic in topics:
        payload.extend(_encode_string(topic))
        payload.append(0)  # qos 0
    variable = struct.pack("!H", packet_id)
    remaining = _encode_varint(len(variable) + len(payload))
    return bytes([0x82]) + remaining + variable + bytes(payload)


def _build_pingreq() -> bytes:
    return b"\xC0\x00"


def _parse_publish(header: int, payload: bytes) -> tuple[str, bytes]:
    topic_len = struct.unpack("!H", payload[:2])[0]
    topic = payload[2:2 + topic_len].decode("utf-8", errors="replace")
    body_start = 2 + topic_len
    qos = (header >> 1) & 0x03
    if qos:
        body_start += 2
    body = payload[body_start:]
    return topic, body


def main() -> int:
    parser = argparse.ArgumentParser(description="Minimal MQTT subscriber without external dependencies")
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--username")
    parser.add_argument("--password")
    parser.add_argument("--client-id", default=f"codex-sub-{random.randint(1000, 9999)}")
    parser.add_argument("--topic", action="append", required=True, help="Topic filter. Can be passed multiple times.")
    parser.add_argument("--duration", type=int, default=0, help="Seconds to listen. 0 means forever.")
    parser.add_argument("--keepalive", type=int, default=30)
    args = parser.parse_args()

    with socket.create_connection((args.host, args.port), timeout=10) as sock:
        sock.settimeout(1.0)
        sock.sendall(_build_connect(args.client_id, args.username, args.password, args.keepalive))
        header, payload = _recv_packet(sock)
        if header >> 4 != 2 or len(payload) < 2 or payload[1] != 0:
            raise RuntimeError(f"MQTT CONNECT failed: header={header:#x}, payload={payload!r}")

        sock.sendall(_build_subscribe(1, args.topic))
        header, payload = _recv_packet(sock)
        if header >> 4 != 9:
            raise RuntimeError(f"MQTT SUBSCRIBE failed: header={header:#x}, payload={payload!r}")

        print(f"Connected to {args.host}:{args.port} as {args.client_id}")
        print("Subscribed topics:")
        for topic in args.topic:
            print(f"  - {topic}")
        print("")

        end_time = time.time() + args.duration if args.duration > 0 else None
        last_ping = time.time()

        while True:
            if end_time and time.time() >= end_time:
                return 0

            if time.time() - last_ping >= max(1, args.keepalive // 2):
                sock.sendall(_build_pingreq())
                last_ping = time.time()

            try:
                header, payload = _recv_packet(sock)
            except socket.timeout:
                continue

            packet_type = header >> 4
            if packet_type == 3:
                topic, body = _parse_publish(header, payload)
                try:
                    text = body.decode("utf-8")
                except UnicodeDecodeError:
                    text = body.hex()
                print(f"[{time.strftime('%H:%M:%S')}] {topic} = {text}")
            elif packet_type == 13:
                continue
            else:
                print(f"[{time.strftime('%H:%M:%S')}] packet type {packet_type} received")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("\nStopped by user")
        raise SystemExit(0)
