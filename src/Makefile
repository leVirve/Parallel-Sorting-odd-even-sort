CFLAGS = -std=gnu99 -O3
LOGFLAGS = -D_INFO -D_DEBUG
uuid := $(shell uuidgen)
TARGET = judge_HW1_$(uuid)_101062337
REPORT = judge_sh_out.*
OUTPUT = judge_* *.o *.rpt

all: clean advanced submit watch

basic:
	$(eval TARGET := $(TARGET)_basic)
	mpicc -o $(TARGET) basic.c $(CFLAGS) $(LOGFLAGS)

advanced:
	$(eval TARGET := $(TARGET)_advanced)
	mpicc -o $(TARGET) advanced.c $(CFLAGS) $(LOGFLAGS)

submit:
	qsub -v exe=$(TARGET) test_job.sh

watch:
	watch qstat -a

result:
	cat $(REPORT) | more

cmp:
	cmp ../testcase/sorted$(i) judge_out_$(i)

clean:
	-@rm $(OUTPUT) 2>/dev/null || true
