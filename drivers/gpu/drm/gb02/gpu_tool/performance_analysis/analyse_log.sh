#!/bin/bash

main()
{

	if [ ! -n "$1" ];then

                echo "please input time log file"
                exit 1
        fi

	if [ ! -n "$2" ];then

                echo "please input kern log save file"
                exit 1
        fi


	local n=0

	stage=`cat '/var/log/kern.log' | grep -a 'gb_ioctl_submit'| grep -a 'TIME:UTC time' | awk -F ':' '{print $NF}'`


	for i in $stage
	do

		if [ $n -gt 0 ];then
			j=$[$n + 1 ]
			
			local gb_ioctl_submit_cur=`cat '/var/log/kern.log' | grep -a 'gb_ioctl_submit'| grep -a 'TIME:UTC time'| sed -n ''$j'p' | awk -F ' ' '{print $(NF-1)}' | awk -F ':' '{print $NF}'` 
			local gb_stage_push_cur=`cat '/var/log/kern.log' | grep -a 'gb_stage_push'|grep -a 'TIME:UTC time'| sed -n ''$j'p' | awk -F ' ' '{print $(NF-1)}' | awk -F ':' '{print $NF}'`
			
			local gb_stage_run_cur=`cat '/var/log/kern.log' | grep -a 'gb_stage_run' | grep -a 'TIME:UTC time'| sed -n ''$j'p' | awk -F ' ' '{print $(NF-1)}' | awk -F ':' '{print $NF}'`
			
			local gb_stage_hw_submit_cur=`cat '/var/log/kern.log' | grep -a 'gb_stage_hw_submit'|grep -a 'TIME:UTC time'| sed -n ''$j'p' | awk -F ' ' '{print $(NF-1)}' | awk -F ':' '{print $NF}'`
			local gb_stage_irq_handle_cur=`cat '/var/log/kern.log' | grep -a 'gb_stage_irq_handle'|grep -a 'TIME:UTC time'| sed -n ''$j'p' | awk -F ' ' '{print $(NF-3)}' | awk -F ':' '{print $NF}'`
			local gb_stage_cleanup_cur=`cat '/var/log/kern.log' | grep -a 'gb_stage_cleanup'|grep -a 'TIME:UTC time' | sed -n ''$j'p' | awk -F ' ' '{print $(NF-1)}' | awk -F ':' '{print $NF}'`
			echo "$3 " $i": " $gb_ioctl_submit_cur $gb_stage_push_cur $gb_stage_run_cur $gb_stage_hw_submit_cur $gb_stage_irq_handle_cur $gb_stage_cleanup_cur >> $1
		fi
		let n++

		local gb_ioctl_submit_time="$gb_ioctl_submit_time $gb_ioctl_submit_cur"
		local gb_stage_push_time="$gb_stage_push_time $gb_stage_push_cur"
		local gb_stage_run_time="$gb_stage_run_time $gb_stage_run_cur"
		local gb_stage_hw_submit_time="$gb_stage_hw_submit_time $gb_stage_hw_submit_cur"
		local gb_stage_irq_handle_time="$gb_stage_irq_handle_time $gb_stage_irq_handle_cur"
		local gb_stage_cleanup_time="$gb_stage_cleanup_time $gb_stage_cleanup_cur"

	done
	
	local gb_ioctl_wait_bo=`cat '/var/log/kern.log' | grep -a 'gb_ioctl_wait_bo' | awk -F ' ' '{print $(NF-2)}' | awk -F ':' '{print $NF}'`


	cat /var/log/kern.log >> $2
	echo /dev/null > /var/log/kern.log
	echo /dev/null > /var/log/syslog

}


main $@

