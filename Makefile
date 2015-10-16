all:
	mpicc -o basic.o basic.c -std=c11 -D_DEBUG
	qsub ./job.sh

result:
	qstat -a
	cat JOB.o*

clean:
	rm JOB.*
	rm *.o

