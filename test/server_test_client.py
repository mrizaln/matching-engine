#!/usr/bin/env python

import struct
import sys
import time
import json
import random
from argparse import ArgumentParser
from socket import socket, AF_INET, SOCK_STREAM

MIN_PRICE = 1
MAX_PRICE = 10000
MIN_QUANTITY = 1
MAX_QUANTITY = 10000

MAX_ITERATION = 100000


class Protocol:
    @staticmethod
    def receive(sock: socket) -> tuple[str, int] | None:
        (raw_msglen, packet_count) = Protocol.__recvall(sock, 4)
        if len(raw_msglen) <= 0:
            return None

        msglen = struct.unpack(">I", raw_msglen)[0]
        (data, packet_count) = Protocol.__recvall(sock, msglen)
        if data is None or len(data) != msglen:
            return None

        return data.decode(), packet_count

    @staticmethod
    def send(sock: socket, string: str):
        msglen = struct.pack(">I", len(string))
        sock.sendall(msglen)
        sock.sendall(string.encode())

    @staticmethod
    def __recvall(sock: socket, n: int) -> tuple[bytearray, int]:
        # Helper function to recv n bytes or return None if EOF is hit
        data = bytearray()
        packet_count = 0
        while len(data) < n:
            packet = sock.recv(n - len(data))
            if not packet:
                break
            data.extend(packet)
            packet_count += 1
        return data, packet_count


def try_connect(
    host: str, port: int, retry_attempt: int, retry_delay: int
) -> socket | None:
    sock = socket(AF_INET, SOCK_STREAM)

    retry_atttempt = 0
    while True:
        try:
            sock.connect((host, port))
            return sock
        except ConnectionRefusedError:
            eprint(
                f"Failed to connect. Retrying in {retry_delay} second. Attempt {retry_atttempt} / {retry_attempt}"
            )
            time.sleep(retry_delay)
            retry_atttempt += 1

            if retry_atttempt > retry_attempt:
                return None


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def main() -> int:
    parser = ArgumentParser(
        description="Simple TCP client with length-based message protocol"
    )

    parser.add_argument("host", help="server host")
    parser.add_argument("port", type=int, help="server port")
    parser.add_argument(
        "--retry-attempt",
        type=int,
        default=5,
        help="number of retry attempts before giving up",
    )
    parser.add_argument(
        "--retry-delay",
        type=int,
        default=1,
        help="delay in seconds before retrying",
    )
    parser.add_argument(
        "--verbose", action="store_true", help="print debug information"
    )

    args = parser.parse_args()

    price_range = range(MIN_PRICE, MAX_PRICE)
    quantity_range = range(MIN_QUANTITY, MAX_QUANTITY)

    def gen_order():
        return {
            "price": random.choice(price_range),
            "quantity": random.choice(quantity_range),
            "type": random.choice(["buy", "sell"]),
        }

    sock = None
    try:
        for i in range(MAX_ITERATION):
            sock = try_connect(
                args.host, args.port, args.retry_attempt, args.retry_delay
            )
            if sock is None:
                eprint("Failed to connect to server. Exiting...")
                return 1

            order = json.dumps(gen_order())
            Protocol.send(sock, order)

            if args.verbose:
                print(f"{i}: {order}")
            else:
                if (i + 1) % 1000 == 0:
                    print(f"sent {i + 1} orders")

            sock.close()

    except KeyboardInterrupt:
        eprint("Interrupt received. stopping client...")

    except Exception as e:
        eprint(e)

    eprint("closing socket")

    assert sock is not None
    sock.close()

    return 0


if __name__ == "__main__":
    ret = main()
    exit(ret)
