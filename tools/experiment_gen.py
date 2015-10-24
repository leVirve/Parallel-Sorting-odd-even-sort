#! /usr/bin/python
import os

directory = 'experiment'

job = """
#PBS -q batch
#PBS -N judge_sh_out
#PBS -r n
#PBS -l nodes={}:ppn={}
#PBS -l walltime=00:01:00

cd $PBS_O_WORKDIR
mpiexec ./$exe 1000000 testcase/testcase1000000 judge_exp_%d
"""
cases = [
    # (1, 1), (1, 2), (1, 3), (1, 4),
    # (1, 5), (1, 6), (1, 7), (1, 8),
    # (1, 9), (1, 10), (1, 11), (1, 12),
    # (1, 1), (2, 1), (3, 1), (4, 1),
    (1, 2), (2, 1), (2, 2), (4, 2),
    (4, 4), (4, 6), (4, 8), (4, 9), (4, 12),
]

if not os.path.exists(directory):
    os.makedirs(directory)

for i, case in enumerate(cases, 1):
    with open(os.path.join(directory, 'experiment%d.sh' % i), 'w') as f:
        f.write(job.format(*case) % i)
