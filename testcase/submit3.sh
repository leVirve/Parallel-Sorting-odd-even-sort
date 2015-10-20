#PBS -q batch
#PBS -N judge_sh_out
#PBS -r n
#PBS -l nodes=1:ppn=7
#PBS -l walltime=00:01:00

cd $PBS_O_WORKDIR
mpiexec ./$exe 21 testcase/testcase3 judge_out_3
