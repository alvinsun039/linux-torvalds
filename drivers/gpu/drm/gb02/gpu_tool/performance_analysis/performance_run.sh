#!/bin/bash

cp /home/sietium/.Xauthority /root/
export LD_LIBRARY_PATH=/usr/local/sietium/lib64
time1=$(date "+%Y%m%d%H%M%S")
TIME_RECORED_LOG=time_record_${time1}.log
TIME_BASE_KERLEN_LOG=source_kern_${time1}.log


main()
{


        if [ ! -n "$1" ];then

                echo "please save input test case file"
                exit 1
        fi

	echo /dev/null > /var/log/kern.log

	echo "test_tool" 'stage_addr' 'gb_ioctl_submit' 'gb_stage_push' 'gb_stage_run' 'gb_stage_hw_submit' 'gb_stage_irq_handle' 'gb_stage_cleanup' >> $TIME_RECORED_LOG

	while read line
	do

		#local run_cmd=`sed -n ''$i'p' glmark2_case.txt`
		local run_cmd=$line
		
		if [ ! -n "$run_cmd" ];then
			continue
		fi
		
		echo $run_cmd

		glmark2_core=`$run_cmd | grep 'glmark2 Score'`

		echo $run_cmd ": " $glmark2_core >> $TIME_RECORED_LOG
		
		#echo -n " " >> "$TIME_RECORED_LOG"
		#echo -n $run_cmd ": " >> "$TIME_RECORED_LOG"
		
		analyze_cmd=`./analyse_log.sh $TIME_RECORED_LOG $TIME_BASE_KERLEN_LOG "$run_cmd"`
		
		echo "$analyze_cmd"

	done < $1
}
main $@

python parse.py ./source_kern_${time1}.log ${time1}
