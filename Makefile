all:
	-rm debug_job.*
	-rm judge_out_*
	-rm *.o
	# mpicc -o basic.o basic.c -std=c11 -D_DEBUG -D_INFO -D_FILE_DEBUG
	mpicc -o basic.o basic.c -std=c11 -D_INFO -D_DEBUG
	qsub ./job.sh

result:
	qstat -a
	cat debug_job.*

clean:
	-rm debug_job.*
	-rm judge_out_*
	-rm *.o
	-rm *.result

