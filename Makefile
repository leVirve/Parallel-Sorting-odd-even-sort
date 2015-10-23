CFLAGS = -std=gnu99 -O3
LOGFLAGS = -D_INFO -D_DEBUG
uuid := $(shell uuidgen)
TARGET = judge_HW1_$(uuid)_101062337
REPORT = judge_sh_out.*
OUTPUT = judge_* *.o *.rpt

all: clean advanced submit watch

basic:
	$(eval TARGET := $(TARGET)_basic)
	mpicc -o $(TARGET) src/basic.c $(CFLAGS) $(LOGFLAGS)

advanced:
	$(eval TARGET := $(TARGET)_advanced)
	mpicc -o $(TARGET) src/advanced.c $(CFLAGS) $(LOGFLAGS)

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

clean:
	-@rm $(OUTPUT) 2>/dev/null || true
