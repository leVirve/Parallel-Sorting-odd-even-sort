import re
import os
import time
import subprocess


job_format = '''
#PBS -q batch
#PBS -N {name}
#PBS -r n
#PBS -l nodes={nodes}:ppn={ppn}
#PBS -l walltime={walltime}
cd $PBS_O_WORKDIR
mpiexec ./{program} {num} {data} {output}
'''


class Testcase:

    def __init__(self, program, nodes, ppn, data, walltime='00:01:00'):
        self.program = program
        self.walltime = walltime
        self.nodes = nodes
        self.ppn = ppn
        self.data = data
        self.name = '%s-%dx%d' % (program, nodes, ppn)

    @property
    def content(self):
        return job_format.format(**{
            'name': self.name,
            'nodes': self.nodes,
            'ppn': self.ppn,
            'walltime': self.walltime,
            'program': self.program,
            'num': self.count_num(),
            'data': self.data,
            'output': 'judge_out_' + self.data.split('/')[-1]
        })

    def count_num(self):
        return os.stat(self.data).st_size / 4


class Scheduler:

    def __init__(self, testcases):
        self.testcases = testcases
        self.retries = 5
        self.delay = 10

    def start(self):
        print('Total jobs: %d' % len(self.testcases))
        for i, case in enumerate(self.testcases):
            self.submit(i, case)

    def submit(self, i, case):

        def qsub():
            p = subprocess.Popen(
                ['qsub'],
                stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            p.stdin.write(case.content)
            return p.communicate()

        for _ in range(self.retries):
            stdout, _ = qsub()
            if re.match('\d+\.\w.', stdout):
                print("Testcase#%d submitted. (job: %s)" % (i, stdout))
                return
            time.sleep(self.delay)
