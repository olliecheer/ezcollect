#!/usr/bin/env python3

import subprocess
import sys
import os


def ldd(binary_path: str) -> list:
    res = []
   
    p = subprocess.Popen("ldd {}".format(binary_path), shell=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)

    o, _ = p.communicate()

    for line in o.decode().splitlines():
        line = line.strip()
        if len(line) == 0:
            continue

        # if line.startswith('linux-vdso') or line == 'not a dynamic executable':
        if line.startswith('linux-vdso'):
            continue

        words = line.split()

        libpath = words[-2]

        res.append(libpath)

    return res


def readlink_util_nonlink(libpath: str) ->list:
    res = []
    res.append(libpath)

    cur = libpath

    while os.path.islink(cur):
        dirname = os.path.dirname(cur)
        cur = os.readlink(cur)
        res.append(os.path.join(dirname, cur))

    return res



all_files = set()



def func(files: list):
    global all_files
    for it in files:
        res = ldd(it)
        for it2 in res:
            all_files= all_files.union(set(readlink_util_nonlink(it2)))


def main():
    func(sys.argv[1:])
    res = list(all_files)
    res.sort()

    for it in res:
        print(it)

main()
