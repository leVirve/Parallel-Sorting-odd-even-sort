#! /usr/bin/python
import sys
import struct


def readfile(name):
    with open(name, 'rb') as f:
        for chunk in iter(lambda: f.read(4), ''):
            i = struct.unpack('<i', chunk)[0]
            print(i)


if __name__ == '__main__':
    readfile(sys.argv[1])
