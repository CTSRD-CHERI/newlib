#!/usr/bin/env python3

import argparse
import subprocess
import os
import re
import signal
import time
import sys


parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("--qemu", metavar="QEMU_PATH", help="Path to qemu", default=os.getenv("QEMU", "qemu-system-mips64"))
parser.add_argument("--timeout", metavar="SECONDS", help="Timeout for program run (0 = no timeout)", default=0, type=int)
parser.add_argument("--pretend", action="store_true", help="Only print the command that would be run.")
parser.add_argument("CMD", help="The binary to excecute")
parser.add_argument("ARGS", nargs=argparse.ZERO_OR_MORE, help="Arguments to pass to the binary")
args = parser.parse_args()

print(args)
timeout = args.timeout if args.timeout else None
# in pretend mode just echo the command
command = ["echo"] if args.pretend else []

# add the MIPS flags:
command += [
    args.qemu,
    "-M", "malta",
    "-m", "128m",  # TODO: make memsize configurable
    "-nographic",
    "-kernel", args.CMD
]
#for program_arg in args.ARGS:
#    command.extend(["-append", program_arg])
if args.ARGS:
    command.append("-append")
    # QEMU wan't all arguments as a single parameter -> make it space separated with backslash escapes
    command.append(" ".join(x.replace(" ", "\\ ") for x in args.ARGS))
print(command)


captured_stdout = []

endtime = None
if args.timeout:
    endtime = time.time() + 60 * args.timeout

with subprocess.Popen(command, stdout=subprocess.PIPE) as p:
    while True:
        data = p.stdout.read(1024)
        if data:
            captured_stdout.append(data)
            sys.stdout.buffer.write(data)
        status = p.poll()
        if status is not None:
            break
        if endtime and time.time() > endtime:
            sys.exit("TIMEOUT after " + str(args.timeout) + " minutes!")

# TODO: only store the last few bytes of stdout
end_of_stdout = b''.join(captured_stdout)[-1024:]  # only need to search the last few bytes for the exit message
successful_exit = re.search(b"\x1b\\[32mExit code was (\\d+)\n", end_of_stdout)
# print(successful_exit)
# TODO: also search for the crash message?
guest_exit_code = None
if successful_exit:
    guest_exit_code = int(successful_exit.group(1))

print("\n\nQEMU exit code: ", status)
print("Program exit code: ", guest_exit_code)
if guest_exit_code is None:
    print("Could not determine exit code!")
    sys.exit(-signal.SIGSEGV)
sys.exit(guest_exit_code)
