#PBS -q batch
#PBS -N judge_sh_out
#PBS -r n
#PBS -l nodes=3:ppn=6
#PBS -l walltime=00:01:00

cd $PBS_O_WORKDIR
mpiexec ./$exe 65537 testcase/testcase8 judge_out_8
