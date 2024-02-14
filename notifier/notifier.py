#!/usr/bin/env python3

import argparse
import socket
import subprocess
import syslog
import traceback
from datetime import datetime
from pathlib import Path

__MOUDLE_PATH = Path(__file__).absolute().parent
__SOUND_PATH = __MOUDLE_PATH / 'sound.mp3'


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument('hostname', help='Hostname or IP to listen on')
    parser.add_argument('port', help='UDP Port to listen on', type=int)
    parser.add_argument('secret', help='Authentication secret')
    parser.add_argument('message', help='Notification message')
    return parser.parse_args()


def notify(text: str) -> None:
    syslog.syslog(syslog.LOG_INFO, text)
    subprocess.run(['notify-send', '-u', 'critical', text])
    subprocess.run(['mpv', '--no-terminal', __SOUND_PATH],
                   stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL)


def main() -> None:
    args = parse_args()
    s = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
    s.bind((args.hostname, args.port))

    secret = args.secret.encode()
    expected_size = len(secret)
    while True:
        try:
            data, _ = s.recvfrom(expected_size)
            if len(data) != expected_size or data != secret:
                continue
            t = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            text = f'[{t}] {args.message}'
            notify(text)

        except Exception:
            traceback.print_exc()


if __name__ == '__main__':
    main()
