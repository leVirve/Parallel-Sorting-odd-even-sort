#/bin/bash
stuID=`echo $USER | sed 's/s\(.*\)/\1/'`
job_prefix=HW1_$(uuidgen)

function clean() {
	rm -f judge_*
	sleep 0.5
	rm -f judge_*
}

function judge() {
	local index=$1
	local version=$2
	job_id=$(echo $(qsub -v exe="judge_${job_prefix}_${stuID}_$version" "testcase/submit$index.sh") | awk -F '[. ]' '{print $1}')

	while [ ! -f "judge_sh_out.o$job_id" ]; do
		sleep 0.2
	done
	sleep 0.2

	index_=$(printf '%2d' $index)

	if [ ! -f "judge_out_"$index ]; then
		echo -e "testcase $index_ \E[0;31;40mno output\E[0m"
		pass=0
		return
	fi

	cmp judge_out_$index testcase/sorted$index
	if [ $? -eq 0 ]; then
		echo -e "testcase $index_ \E[0;32;40maccepted\E[0m"
	else
		echo -e "testcase $index_ \E[0;31;40mwrong answer\E[0m"
		pass=0
	fi
}

function print_pass() {
	local pass=$1
	local exe=$2
	local score=$3
	local desc=$4
	if [ $pass -eq 1 ]; then
		echo -e " \E[1m$exe  \E[0;32;40mpassed: [$score%] \E[0;94m${desc}\E[0m"
	elif [ $pass -eq 0 ]; then
		echo -e " \E[1m$exe  \E[0;31;40mfailed: [$score%] \E[0;94m${desc}\E[0m"
	else
		echo -e " \e[1m$exe \E[0;31;40mskipped: [$score%] \E[0;94m${desc}\E[0m"
	fi
}

function print_passes() {
	score=0
	test $check_1 -eq 1 && score=$(($score+5))
	test $check_2 -eq 1 && score=$(($score+10))
	test $check_3 -eq 1 && score=$(($score+15))
	test $check_4 -eq 1 && score=$(($score+20))
	score=$(printf '%2d' $score)
	echo -e "######## Correctness \E[0;33;40m($score/50)\E[0m ########"
	print_pass $check_1 ""$stuID"_basic   " " 5" "(# of input items = # of processes)"
	print_pass $check_2 ""$stuID"_basic   " "10" "(# of input items is divisible by # of processes)"
	print_pass $check_3 ""$stuID"_basic   " "15" "(arbitrary # of input items)"
	print_pass $check_4 ""$stuID"_advanced" "20" "(arbitrary # of input items)"
	echo "#####################################"
}

function judges() {
	local version=$1
	local lang=$2

	src=HW1_"$stuID"_${version}.$lang

	echo --- Reading $src ---
	if [ ! -f $src ]; then
		return
	fi

	exe=judge_${job_prefix}_"$stuID"_$version

	if [ $lang = c ]; then
		mpicc -O3 -o $exe $src
	else
		mpic++ -O3 -o $exe $src
	fi

	if [ -f judge_${job_prefix}_"$stuID"_basic ]; then
		sleep 0.5
		echo --- Judging $src ---

		check_1=0
		check_2=0
		check_3=0

		pass=1
		judge 1 basic
		judge 2 basic
		check_1=$pass
		judge 3 basic
		judge 4 basic
		check_2=$pass

		for((i=5;i<=10;i=i+1))
		do
			judge $i basic
		done
		check_3=$pass

	elif [ -f judge_${job_prefix}_"$stuID"_advanced ]; then
		echo --- Judging $src ---
		sleep 0.5
		check_4=0
		pass=1
		for((i=1;i<=10;i=i+1))
		do
			judge $i advanced
		done
		check_4=$pass
	fi
	echo
	clean
}

clean

check_1=-1
check_2=-1
check_3=-1
check_4=-1
judges basic c
test $check_1 -eq -1 && judges basic cpp
judges advanced c
test $check_4 -eq -1 && judges advanced cpp

print_passes
