#PBS -q batch
#PBS -N judge_sh_out
#PBS -r n
#PBS -l nodes=2:ppn=5
#PBS -l walltime=00:01:00

cd $PBS_O_WORKDIR
mpiexec ./$exe 3 testcase/testcase6 judge_out_6
