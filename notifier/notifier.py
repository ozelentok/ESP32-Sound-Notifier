#!/usr/bin/env python3

import argparse
import socket
import subprocess
import traceback
from datetime import datetime


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument('hostname', help='Hostname or IP to listen on')
    parser.add_argument('port', help='UDP Port to listen on', type=int)
    parser.add_argument('secret', help='Authentication secret')
    parser.add_argument('message', help='Notification message')
    return parser.parse_args()


def notify(text: str) -> None:
    print(text)
    subprocess.run(['notify-send', '-u', 'critical', text])


def main() -> None:
    args = parse_args()
    s = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
    s.bind((args.hostname, args.port))

    expected_size = len(args.secret)
    while True:
        try:
            data, _ = s.recvfrom(expected_size)
            if len(data) != expected_size and data != args.secret:
                continue
            t = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            text = f'[{t}] {args.message}'
            notify(text)

        except Exception:
            traceback.print_exc()


if __name__ == '__main__':
    main()
