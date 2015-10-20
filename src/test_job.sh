#PBS -q batch
#PBS -N debug_job
#PBS -r n
#PBS -l nodes=1:ppn=3
#PBS -l walltime=00:00:30

cd $PBS_O_WORKDIR
mpiexec ./basic.o 97 ../testcase/testcase7 judge_out_7
