#!/usr/bin/env python3

# pylint: disable=invalid-name,missing-function-docstring,unused-argument

from datetime import datetime, timedelta
from pathlib import Path

import argparse
import contextlib
import io
import os
import signal
import subprocess
import sys
import time


DEBUG_SESSION_FILE = '.debug-session'
GDBINIT_FILE = '.gdbinit'


class _Getch:
    """Gets a single character from standard input. Does not echo to the screen."""
    def __init__(self):
        try:
            self.impl = _GetchWindows()
        except ImportError:
            self.impl = _GetchUnix()

    def __call__(self): return self.impl()


class _GetchUnix:
    def __init__(self):
        import tty, sys

    def __call__(self):
        import sys, tty, termios
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch


class _GetchWindows:
    def __init__(self):
        import msvcrt

    def __call__(self):
        import msvcrt
        return msvcrt.getch()


getch = _Getch()


def get_full_wd():
    p = Path(".").resolve()
    return p


def mkdelta(deltavalue):
    _units = dict(d=60 * 60 * 24, h=60 * 60, m=60, s=1)
    seconds = 0
    defaultunit = unit = _units['s']  # default to seconds
    value = ''
    for ch in list(str(deltavalue).strip()):
        if ch.isdigit():
            value += ch
            continue
        if ch in _units:
            unit = _units[ch]
            if value:
                seconds += unit * int(value)
                value = ''
                unit = defaultunit
            continue
        if ch in ' \t':
            # skip whitespace
            continue
        raise ValueError('Invalid time delta: %s' % deltavalue)
    if value:
        seconds = unit * int(value)
    return timedelta(seconds=seconds)


def make_signal_handler(process):
    def signal_handler(sig, frame):
        print('\nterminating due to C-c')
        process.terminate()
    return signal_handler


def pipe_bytes(r, w):
    for c in iter(lambda: r.read(1), b''):
        w.buffer.write(c)
        w.flush()


# Contents of .gdbinit are taken from here:
# https://wiki.qemu.org/Documentation/Platforms/RISCV#Attaching_GDB
def write_gdb_files(binary, is_32bit, is_multicore):
    with open(DEBUG_SESSION_FILE, 'w') as f:
        f.write(binary)
    with open(GDBINIT_FILE, 'w') as f:
        if is_32bit:
            f.write('set arch riscv:rv32\n')
        f.write('target extended-remote localhost:1234\n')
        if is_multicore:
            f.write('add-inferior\n')
            f.write('inferior 2\n')
            f.write('attach 2\n')
            f.write('set schedule-multiple\n')
            # f.write('set scheduler-locking on\n')
            f.write('thread 1.1\n')


def cleanup_gdb_files():
    with contextlib.suppress(FileNotFoundError):
        os.unlink(DEBUG_SESSION_FILE)
        os.unlink(GDBINIT_FILE)


def make_docker_cmd(is_32bit, is_interactive):
    mount =  'type=bind,source={},target=/host'.format(get_full_wd())
    cmd = ['docker', 'run']
    if is_interactive:
        cmd.extend(['-it', '--name', 'qemu-image'])
    cmd.extend(['--rm', '--mount', mount, '--net=host', 'qemu-image:latest'])
    if is_32bit:
        cmd.append('rv32')
    return cmd


def make_qemu_command(args):
    binary = args.binary

    # First try to derive qemu and machine from binary name:
    is_32bit = False
    if binary.endswith('32'):
        is_32bit = True

    if binary.endswith('os_virt'):
        machine = 'virt'
    elif binary.strip('32').endswith('_u'):
        machine = 'sifive_u'
    else:
        machine = 'sifive_e'

    is_interactive = ('test' not in binary) and (
        args.bootargs not in ['dry-run', 'smoke-test', 'tiny-stack']
    )
    cmd = make_docker_cmd(is_32bit, is_interactive)

    # Override our guesses if args were passed explicitly:
    if args.qemu is not None:
        cmd = [args.qemu]
    if args.machine is not None:
        machine = args.machine

    cmd.extend([
        '-nographic', '-machine', machine, '-bios', 'none', '-kernel', binary,
    ])
    if args.debug:
        cmd.extend([
            '-S',  # only loads an image, but stops the CPU, giving a chance to attach gdb
            '-s',  # a shorthand to listen for gdb on localhost:1234
        ])
    if args.bootargs:
        cmd.extend(['-append', args.bootargs])
    return cmd, machine, binary, is_32bit


def kill_qemu_container():
    cp = subprocess.run([
        'docker', 'ps', '-q', '--filter', 'ancestor=qemu-image'],
        capture_output=True,
    )
    container_id = cp.stdout.strip()
    subprocess.run(['docker', 'kill', container_id], capture_output=True)


def run(args):
    """Runs qemu in a bit more user-friendly way, capturing stdout+stderr to a
    file, allowing to terminate it with C-c and optionally exit automatically
    with timeout.

    The implementation has a couple nuances.

    signal.signal() installs a signal handler for SIGINT, which handles C-c in
    a friendly fashion: terminates the qemu and avoids printing out the stack
    trace of this script.

    subprocess.Popen() redirects stdout and stderr to a file, which is then
    echoed to stdout as well. Very importantly, it tells the subprocess to read
    stdin from a pipe (currently not written to), which prevents qemu from
    taking ownership of stdin and allowing us to capture C-c.
    """
    timeout = None
    if args.timeout is not None:
        timeout = mkdelta(args.timeout)
    cmd, machine, binary, is_32bit = make_qemu_command(args)
    if args.debug:
        is_multicore = machine == 'sifive_u'
        write_gdb_files(binary, is_32bit, is_multicore)
    filename = 'out/test-run-{}.log'.format(os.path.basename(args.binary))
    with io.open(filename, 'wb') as writer, io.open(filename, 'rb', 1) as reader:
        p = subprocess.Popen(cmd,
                             # stdin=subprocess.PIPE,
                             stdout=writer, stderr=writer,
                             )
        signal.signal(signal.SIGINT, make_signal_handler(p))
        start = datetime.now()
        while p.poll() is None:
            pipe_bytes(reader, sys.stdout)
            if timeout is not None:
                if start + timeout < datetime.now():
                    print('\nqemu-launcher: killing qemu due to timeout')
                    kill_qemu_container()
                    break
            time.sleep(0.001)  # yield CPU
        # write the remainder:
        pipe_bytes(reader, sys.stdout)
    cleanup_gdb_files()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--timeout', help='terminate qemu after timeout (e.g. 15s, 3m)')
    parser.add_argument('--qemu', help='qemu binary')
    parser.add_argument('--machine', help='"-machine" arg to pass to qemu')
    parser.add_argument('--binary', help='binary to execute in qemu (defaults to os_sifive_u)',
                        default='out/os_sifive_u')
    parser.add_argument('--debug', help='stop to wait for gdb before executing binary',
                        action='store_true')
    parser.add_argument('--bootargs', help='pass this as bootargs to the kernel')
    args = parser.parse_args()
    run(args)


if __name__ == '__main__':
    main()
