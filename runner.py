#! /usr/bin/python
import os
import re
import sys
import time
import filecmp
import subprocess

path = '../testcase'

testcases = [
    e
    for e in os.listdir(path)
    if e.startswith('testcase')
]


def nature_sort(l):
    alphanum_keys = lambda x: int(re.split('(\d+)', x)[1])
    return sorted(l, key=alphanum_keys)


def get_filesize(fp):
    size = 0
    with open(fp, 'rb') as f:
        size = str(int(len(f.read()) / 4))
    return size


def run_job(testcase, result):
    size = get_filesize(testcase)
    print('Test<%s>: conatains %s integers.' % (testcase, size))
    s = time.time()
    subprocess.call(['./basic.o', size, testcase, result])
    return time.time() - s


def validate_job(answer, result, time):
    same = filecmp.cmp(answer, result)
    log = '===> %s in %s sec' % ('Pass' if same else 'Fail!', int(time * 1000) / 1000)
    print(log)


if __name__ == '__main__':

    cases = int(sys.argv[1])

    for i, case in enumerate(nature_sort(testcases)[:cases], start=1):

        testcase = '%s/%s' % (path, case)
        answer = '%s/sorted%d' % (path, i)
        result = '%s.result' % case

        t = run_job(testcase, result)

        validate_job(answer, result, t)
