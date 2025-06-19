#!/bin/bash
path=$(cd $(dirname $0); pwd)
func_file="$path/fun.txt"
macro_file="$path/macro.txt"
struct_file="$path/struct.txt"
flag="ture"

if [ ! -f "$func_file" ]; then
	touch $func_file
fi

if [ ! -f "$macro_file" ]; then
	touch $macro_file
fi

if [ ! -f "$struct_file" ]; then
	touch $struct_file
fi

echo "search start..."

if [ "$2" = "on" ];then
	macro_switch="on"
else
	macro_switch="off"
fi


collect_file() {
	check_skip="no"
	#-------search entire file----------------
	while read line
	do
	#-------skip comment----------------
	line_letter="${line:0:1}"
	if [ "$line_letter" = "*" ] || [ "$line_letter" = "/" ]; then
		#echo "commet $line_letter"
		continue;
	fi

	version_code_s=`echo "$line" | grep "#if" | grep "LINUX_VERSION_CODE"`
	if [ -n "$version_code_s" ];then
		check_skip="yes"
	fi

	version_code_e=`echo "$line" | grep "#endif"`
	if [ -n "$version_code_e" ];then
		check_skip="no"
	fi
	if [ "$check_skip" = "yes" ];then
		continue;
	fi

	equal_sign=`echo "$line" | grep '='`
	colon=`echo "$line" | grep "\""`
	semicolon=`echo "$line" | grep ";"`
	if [ -n "$semicolon" ] || [ -n  "$equal_sign" ] || [ -n  "$colon" ]; then
		continue;
	fi

        first_word=`echo "$line" | awk -F' ' '{print $1}'`
        if  [ "$first_word" = "extern" ]; then
                continue;
        fi

	if [ "$macro_switch" = "on" ];then
	#-------seek macro----------------
	flag="ture"
	macro=`echo "$line" | grep "#define" | awk -F' ' '{print $2}'`
	if [ -n "$macro" ]; then
		comma=`echo "$line" | grep ","`
		if [ -n  "$comma" ]; then
			continue;
		fi
		
		macro_len=`echo "$macro" | awk -F "" '{print NF}'`
		macro_num=$((macro_len))
		if [ $macro_num -lt 6 ];then
			continue;
		fi

		macro_first_letter=${macro:0:1}
		if [ "$macro_first_letter" = "_" ] || [[ $macro_first_letter =~ ^[a-z]+$ ]];then
			continue;
		fi

		macro_tmp=`echo $macro | grep "(" | awk -F'(' '{print $1}'`
		if [ -n "$macro_tmp" ]; then
			macro=$macro_tmp	
		fi

		macro_underline=`echo "$macro" | grep '_' `
		if [ -z  "$macro_underline" ]; then
			continue
		fi
		
		if  [ "$macro" = "ARRAY_SIZE" ]; then
			continue
		fi
		#echo $funally
		if  [ "$flag" = "ture" ]; then
			while read line
			do
				if  [ "$macro" = "$line" ]; then
					flag="false"
					break
				fi		
			done < $macro_file 
		fi
		#echo "$flag"
		if  [ "$flag" = "ture" ]; then
			echo $macro >> $macro_file 
		else
			flag="ture"
		fi		
	fi

	fi
	#-------search function----------------
	flag="ture"
	tmp=`echo "$line" | grep '(' | awk -F'(' '{print $1}' | awk -F' ' '{print $NF}'`
	define_fun=`echo "$line" | grep "#define"`
	if [ -n "$tmp" ] && [ -z "$define_fun" ]; then
		nu=`echo "$line" | grep '(' | awk -F'(' '{print $1}' | awk '{print NF}'`
		num=$((nu)) 
		if [ $num -gt 1 ]; then
			penultimate=`echo "$line" | grep '(' | awk -F'(' '{print $1}' | awk -F' ' '{print $(NF-1)}'`
			if  [ "$penultimate" = "return" ]; then
				continue
			fi
			string_len=`echo "$penultimate" | awk -F "" '{print NF}'`
			string_num=$((string_len))
			if [ $string_num -gt 2 ] || [ "$penultimate" = "u8" ]; then
				comma=`echo "$tmp" | grep ","`
				if [ -n  "$comma" ]; then
					continue;
				fi

				first_letter="${tmp:0:1}"
				if [ "$first_letter" = "*" ]; then
					tmp=${tmp:1}
				fi

				asterisk=`echo "$tmp" | grep "*"`
				if [ -n  "$asterisk" ]; then
					continue;
				fi

				first_letter="${tmp:0:1}"
				if [[ $first_letter =~ ^[a-z]+$ ]] || [ "$first_letter" = "_" ];then
					flag="ture"
				else
					flag="false"
				fi

				underline=`echo "$tmp" | grep '_' `
				if [ -z  "$underline" ]; then
					continue
				fi

				if [ "$tmp" = "log_ctrl_show" ] || [ "$tmp" = "log_ctrl_store" ]; then
					continue
				fi

				if [ "$tmp" = "pwm_config" ] || [ "$tmp" = "pwm_enable" ]; then
					continue
				fi

				if  [ "$flag" = "ture" ]; then
					while read line
					do
						if  [ "$tmp" = "$line" ]; then
							flag="false"
							break
						fi		
					done < $func_file 
				fi
	
				if  [ "$flag" = "ture" ]; then
					echo $tmp >> $func_file 
				else
					flag="ture"
				fi		
			fi
		fi
	fi
	#-------seek struct----------------
	flag="ture"
	struct=`echo "$line" | grep "struct" | grep '{' | awk -F'{' '{print $1}' | awk -F' ' '{print $NF}'` 
	if [ -n "$struct" ]; then
		comma=`echo "$line" | grep ","`
		if [ -n  "$comma" ]; then
			continue;
		fi

		struct_underline=`echo "$struct" | grep '_' `
		if [ -z  "$struct_underline" ]; then
			continue
		fi
		str1=`echo "$line" | grep "typedef"`
		right_p=`echo "$line" | grep ')'`
		if [ -z  "$right_p" ] && [ -z  "$str1" ]; then
			while read line
			do
				if  [ "$struct" = "$line" ]; then
					flag="false"
					break
				fi		
			done < $struct_file

			if  [ "$flag" = "ture" ]; then
				echo $struct >> $struct_file
			else
				flag="ture"
			fi
		fi
	fi

	done < $1
}

collect_dir() {
	echo $1
	for file in $1/*
	do
		if [ -f $file ]; then
			last_letter=${file: -2}
			if [ "$last_letter" = ".c" ] || [ "$last_letter" = ".h" ];then
				if [ "$file" = "common/gb_kernel_ver.h" ] || [ "$file" = "common/gb_osal.c" ] || [ "$file" = "common/sync_file.h" ];then
					echo "skip $file"
				else
					echo $file
					collect_file $file
				fi
			fi
		elif [ -d $file ]; then
			collect_dir $file
		fi
	done
}

if [ -f "$1" ]; then
        collect_file $1
elif [ -d "$1" ]; then
	file_name=$1
	file_name_last=${file_name: -1}
	if [ "$file_name_last" = "/" ]; then
		file_name=${file_name%?}
	fi
        collect_dir $file_name
else
        echo "input error parameter"
        echo "example:"
        echo "execute on a single file: ./collect.sh xxx.c(xxx.h)"
        echo "execute on folder: ./collect.sh xxx"
        exit
fi

echo "search end"
