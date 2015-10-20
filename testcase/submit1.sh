#PBS -q batch
#PBS -N judge_sh_out
#PBS -r n
#PBS -l nodes=2:ppn=2
#PBS -l walltime=00:01:00

cd $PBS_O_WORKDIR
mpiexec ./$exe 4 testcase/testcase1 judge_out_1
