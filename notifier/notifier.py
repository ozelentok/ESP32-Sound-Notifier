#!/usr/bin/env python3

import argparse
import socket
import subprocess
import syslog
import traceback
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
from pathlib import Path

__MOUDLE_PATH = Path(__file__).absolute().parent
__SOUND_PATH = __MOUDLE_PATH / 'sound.mp3'
__SOCKET_TIMEOUT = 2
__EXECUTOR = ThreadPoolExecutor(4)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument('udp_hostname', help='Hostname or IP to listen on UDP')
    parser.add_argument('tcp_hostname', help='Hostname or IP to listen on TCP')
    parser.add_argument('port', help='Port to listen on', type=int)
    parser.add_argument('secret', help='Authentication secret')
    parser.add_argument('message', help='Notification message')
    return parser.parse_args()


def notify(text: str, play_sound=False) -> None:
    syslog.syslog(syslog.LOG_INFO, text)
    subprocess.run(['notify-send', '-u', 'critical', text])
    if play_sound:
        __EXECUTOR.submit(subprocess.run,
            ['mpv', '--no-terminal', __SOUND_PATH],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )


def udp_listener(
    clients: list[socket.socket],
    hostname: str,
    port: int,
    secret: str,
    message: str,
) -> None:
    secret_bytes = secret.encode()
    expected_size = len(secret_bytes)

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((hostname, port))
    print(f'Listening on UDP {hostname}:{port}')

    while True:
        try:
            data, _ = s.recvfrom(expected_size)
            if data != secret_bytes:
                continue
            print('Got UDP broadcast')
            t = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            text = f'[{t}] {message}'
            notify(text, True)
            notify_clients(clients, text)

        except Exception:
            traceback.print_exc()
            notify(f'Notifier Error\n{traceback.format_exc()}')


def tcp_listener(
    clients: list[socket.socket], hostname: str, port: int
) -> None:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((hostname, port))
    s.listen()
    print(f'Listening on TCP {hostname}:{port}')

    while True:
        try:
            conn, _ = s.accept()
            conn.settimeout(__SOCKET_TIMEOUT)
            clients.append(conn)

        except Exception:
            traceback.print_exc()
            notify(f'Notifier Error\n{traceback.format_exc()}')


def notify_clients(clients: list[socket.socket], message: str) -> None:
    data = message.encode().replace(b'\n', b' ') + b'\n'
    clients_to_remove_idxs = []
    for i, c in enumerate(clients.copy()):
        try:
            sent_bytes = c.send(data)
            if sent_bytes != len(data):
                raise ValueError('Failed to notify client, not all bytes sent')

        except ConnectionError:
            clients_to_remove_idxs.append(i)

        except Exception:
            clients_to_remove_idxs.append(i)
            traceback.print_exc()
            notify(f'Notifier Error\n{traceback.format_exc()}')

    for i in reversed(clients_to_remove_idxs):
        client = clients.pop(i)
        try:
            client.close()
        except Exception:
            pass


def main() -> None:
    args = parse_args()
    clients: list[socket.socket] = []
    udp_task = __EXECUTOR.submit(
        udp_listener,
        clients,
        args.udp_hostname,
        args.port,
        args.secret,
        args.message,
    )
    tcp_listener_task = __EXECUTOR.submit(
        tcp_listener,
        clients,
        args.tcp_hostname,
        args.port,
    )
    udp_task.result()
    tcp_listener_task.result()


if __name__ == '__main__':
    main()
