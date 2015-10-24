#! /usr/bin/python
import re
import os
import sys
import json
import glob
import filecmp
import operator


class ExecutetimeReporter:
    fields = ('total', 'io', 'communicate', 'compute')
    expr = '#Rank(\d*): total=(\d*) nsec \(io=(\d*), comm=(\d*), comp=(\d*)\)'
    world_info = 'world_size (\d+)'
    fmt = '{0:30}{1:<18}{2:<18}{3:<18}{4:<18}'

    regex = re.compile(expr)
    wi = re.compile(world_info)

    def __init__(self, filename):
        self.filename = filename
        self.world_info = ''
        self.log = list()
        self.data = None

    def add(self, log):
        m = self.regex.match(log)
        w = self.wi.match(log)
        if m:
            self.log.append([int(e) for e in m.groups()])
        elif w:
            self.world_info = w.group(1)

    def report(self):
        self.filename += ' all=' + self.world_info
        ss = [sum(map(operator.itemgetter(n), self.log)) for n in range(1, 5)]
        avg = [float(s) / len(self.log) / 1000000 for s in ss]
        self.dump(zip(self.fields, avg))
        self.printf(avg)

    def dump(self, out):
        self.data = {
            'env': self.filename,
            'average': out,
            'details': sorted(self.log)
        }
        fp = self.filename.replace(':', '').replace('=', '')
        with open('log/%s' % fp, 'w') as f:
            json.dump(self.data, f, indent=2, ensure_ascii=False)

    @classmethod
    def print_tiltle(self):
        titles = ['Process'] + list(self.fields)
        print(self.fmt.format(*titles))

    def printf(self, avg):
        buf = [self.filename] + avg
        print(self.fmt.format(*buf))


def nature_sort(l):
    convert = lambda text: int(text) if text.isdigit() else text.lower()
    alphanum_keys = lambda x: [convert(c) for c in re.split('(\d+)', x)]
    return sorted(l, key=alphanum_keys)


def validate_job(answer_case, b):
    same = filecmp.cmp('testcase/sorted%s' % answer_case, b)
    print('===> %s' % ('Pass' if same else 'Fail!'))


def get_job_meta(filename):
    with open(filename, 'r') as f:
        return re.search('PBS -l ([^\n]*)', f.read()).groups(1)[0]


if __name__ == '__main__':
    jobs = glob.glob('*.o*')
    answer_case = sys.argv[1]

    directory = 'log/'

    if not os.path.exists(directory):
        os.makedirs(directory)

    queue = list()

    for i, job in enumerate(nature_sort(jobs), start=1):
        qid = re.split('(\d+)', job)[1]

        job_script = 'experiment/experiment%d.sh' % i
        m = get_job_meta(job_script)
        print(m)

        output = 'judge_exp_%d' % i
        try:
            validate_job(answer_case, output)
        except:
            continue

        e = ExecutetimeReporter(m)
        queue.append(e)
        with open(job, 'r') as f:
            for line in f.readlines():
                e.add(line)

    ExecutetimeReporter.print_tiltle()
    for e in queue:
        e.report()
