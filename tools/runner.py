#! /usr/bin/python
import re
import os
import sys
import glob
import filecmp
from os.path import join
import report


def nature_sort(l):
    alphanum_keys = lambda x: int(re.split('(\d+)', x)[1])
    return sorted(l, key=alphanum_keys)


def validate_job(a, b):
    same = filecmp.cmp(join(path, a), b)
    print('===> %s' % ('Pass' if same else 'Fail!'))


def get_job_meta(filename):
    with open(filename, 'r') as f:
        return re.search('PBS -l ([^\n]*)', f.read()).groups(1)[0]

if __name__ == '__main__':
    jobs = glob.glob('*.o*')
    answer_case = sys.argv[1]

    path = 'testcase/'
    spath = 'experiment/'
    directory = 'log/'

    if not os.path.exists(directory):
        os.makedirs(directory)

    for i, job in enumerate(nature_sort(jobs), start=1):
        qid = re.split('(\d+)', job)[1]
        answer = 'sorted%s' % answer_case
        output = 'judge_exp_%d' % i
        job_script = join(spath, 'experiment%d.sh' % i)

        validate_job(answer, output)
        m = get_job_meta(job_script)

        e = report.ExecutetimeReporter('basic_' + m)
        with open(job, 'r') as f:
            for line in f.readlines():
                e.add(line)
        print(m)
        e.report()
