#PBS -q batch
#PBS -N judge_sh_out
#PBS -r n
#PBS -l nodes=3:ppn=1
#PBS -l walltime=00:01:00

cd $PBS_O_WORKDIR
mpiexec ./$exe 97 testcase/testcase7 judge_out_7
