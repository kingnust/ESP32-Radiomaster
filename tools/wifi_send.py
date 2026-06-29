import argparse
import queue
import socket
import sys
import threading
import time


DEFAULT_HOST = "192.168.4.1"
DEFAULT_PORT = 7777


def reader(sock: socket.socket) -> None:
    while True:
        data = sock.recv(1024)
        if not data:
            print("\n[disconnected]")
            return
        print(data.decode(errors="replace"), end="")


def send_line(sock: socket.socket, line: str) -> None:
    sock.sendall((line.rstrip("\r\n") + "\n").encode())


def keyboard(lines: "queue.Queue[str]") -> None:
    while True:
        line = sys.stdin.readline()
        if not line:
            lines.put("")
            return
        lines.put(line)


def main() -> int:
    parser = argparse.ArgumentParser(description="Send commands to the ESP32 RadioMaster bridge.")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--keepalive", action="store_true", help="Send KEEPALIVE once per second.")
    parser.add_argument("commands", nargs="*", help="Commands to send, then exit.")
    args = parser.parse_args()

    with socket.create_connection((args.host, args.port), timeout=5) as sock:
        sock.settimeout(None)

        if args.commands:
            for command in args.commands:
                send_line(sock, command)
                time.sleep(0.1)
            time.sleep(0.5)
            return 0

        threading.Thread(target=reader, args=(sock,), daemon=True).start()

        lines: "queue.Queue[str]" = queue.Queue()
        threading.Thread(target=keyboard, args=(lines,), daemon=True).start()

        next_keepalive = time.monotonic() + 1.0
        while True:
            if args.keepalive and time.monotonic() >= next_keepalive:
                send_line(sock, "KEEPALIVE")
                next_keepalive = time.monotonic() + 1.0

            try:
                line = lines.get_nowait()
            except queue.Empty:
                line = None

            if line is not None:
                if not line:
                    return 0
                send_line(sock, line)

            time.sleep(0.02)


if __name__ == "__main__":
    raise SystemExit(main())
