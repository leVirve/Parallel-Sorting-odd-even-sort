CFLAGS = -std=c11 -O3
CC = mpicc

#LOGFLAGS = -D_TIME
#uuid := $(shell uuidgen)
#TARGET = judge_HW1_$(uuid)_101062337
#REPORT = judge_sh_out.*
OUTPUT = judge_* *.o *.rpt *basic *advanced measure_*

all: clean basic submit watch

debug: CFLAGS += -D_DEBUG -D_INFO
debug: exe

basic:
	$(eval TARGET := $(TARGET)_basic)
	$(CC) -o $(TARGET) src/basic.c src/utils.c $(CFLAGS) $(LOGFLAGS)

advanced:
	$(eval TARGET := $(TARGET)_advanced)
	$(CC) -o $(TARGET) src/advanced.c src/utils.c $(CFLAGS) $(LOGFLAGS)

measure:
	$(CC) -o measure_io src/measurements/measure_io.c -std=gnu11 -O3 -Wno-unused-result $(flag)
	$(CC) -o measure_basic src/measurements/measure_basic.cpp -std=c++11 -O3 -Wno-unused-result $(flag)

submit:
	qsub -v exe=$(TARGET) testcase/submit$(i).sh

watch:
	watch qstat -a

result:
	cat $(REPORT) | more

cmp:
	cmp testcase/sorted$(i) judge_out_$(i)

judge:
	cp src/basic.c HW1_101062337_basic.c
	cp src/advanced.c HW1_101062337_advanced.c
	./judge.sh

experiment: $(exe)
	@number=$(s) ; while [[ $$number -le $(e) ]] ; do \
        qsub -v exe=$(TARGET) experiment/experiment$$number.sh ; \
        ((number = number + 1)) ; \
    done

exe:
	$(CC) -o basic src/basic.c src/utils.c $(CFLAGS)
	$(CC) -o advanced src/advanced.c src/utils.c $(CFLAGS)

test:
	mpirun -n 4 ./$(exe) 4 ./testcase/testcase1 judge_out_1 && cmp judge_out_1 ./testcase/sorted1
	mpirun -n 4 ./$(exe) 15 ./testcase/testcase2 judge_out_2 && cmp judge_out_2 ./testcase/sorted2
	mpirun -n 4 ./$(exe) 21 ./testcase/testcase3 judge_out_3 && cmp judge_out_3 ./testcase/sorted3
	mpirun -n 4 ./$(exe) 50 ./testcase/testcase4 judge_out_4 && cmp judge_out_4 ./testcase/sorted4
	mpirun -n 4 ./$(exe) 11 ./testcase/testcase5 judge_out_5 && cmp judge_out_5 ./testcase/sorted5
	mpirun -n 4 ./$(exe) 3 ./testcase/testcase6 judge_out_6 && cmp judge_out_6 ./testcase/sorted6
	mpirun -n 4 ./$(exe) 97 ./testcase/testcase7 judge_out_7 && cmp judge_out_7 ./testcase/sorted7
	mpirun -n 4 ./$(exe) 65537 ./testcase/testcase8 judge_out_8 && cmp judge_out_8 ./testcase/sorted8
	mpirun -n 4 ./$(exe) 12347 ./testcase/testcase9 judge_out_9 && cmp judge_out_9 ./testcase/sorted9
	mpirun -n 4 ./$(exe) 123457 ./testcase/testcase10 judge_out_10 && cmp judge_out_10 ./testcase/sorted10

clean:
	-@rm $(OUTPUT) 2>/dev/null || true
