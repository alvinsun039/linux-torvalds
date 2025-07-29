#!/bin/bash
path=$(cd $(dirname $0); pwd)
func_name=GB02FUNC
func_file="$path/fun.txt"
macro_name=GB02MAC
macro_file="$path/macro.txt"
sturct_name=GB02STR
struct_file="$path/struct.txt"

if [ ! -f "$func_file" ]; then
	echo "$func_file does not exist"
	exit
fi

if [ ! -f "$macro_file" ]; then
	echo "$macro_file does not exist"
	exit
fi

if [ ! -f "$struct_file" ]; then
	echo "$struct_file does not exist"
	exit
fi

echo "replace start..."

if [ "$2" = "on" ];then
        macro_switch="on"
else
        macro_switch="off"
fi

replace_file() {
	#-------use the results found to change the sorurce file--------------
	#echo "$1"
	declare -i data=1
	while read line
	do
	rename_func="${func_name}${data}"
	#echo $rename_func
	data=$data+1
	sed -i "/#include/b; s/\b$line\b/$rename_func/g" $1
	done < $func_file

	if [ "$macro_switch" = "on" ];then
	#-------use the results found to change the sorurce file--------------
	declare -i data1=1
	while read line
	do
	rename_macro="${macro_name}${data1}"
	#echo $rename_macro
	data1=$data1+1
	sed -i "/#include/b; s/\b$line\b/$rename_macro/g" $1
	done < $macro_file
	fi

	#-------use the results found to change the sorurce file--------------
	declare -i data2=1
	while read line
	do
	rename_struct="${sturct_name}${data2}"
	#echo $rename_struct
	data2=$data2+1
	sed -i "/#include/b; s/\b$line\b/$rename_struct/g" $1
	done < $struct_file
}

replace_dir() {
	echo $1
	for file in $1/*
	do  
		if [ -f $file ]; then
			last_letter=${file: -2}
				if [ "$last_letter" = ".c" ] || [ "$last_letter" = ".h" ];then
					echo $file
					replace_file $file
				fi  
		elif [ -d $file ]; then
			replace_dir $file
		fi  
	done
}

if [ -f "$1" ]; then
	replace_file $1
elif [ -d "$1" ]; then
	file_name=$1
	file_name_last=${file_name: -1}
	if [ "$file_name_last" = "/" ]; then
		file_name=${file_name%?}
	fi
	replace_dir $file_name 
else
	echo "input error parameter"
	echo "example:"
	echo "execute on a single file: ./replace.sh xxx.c(xxx.h)"
	echo "execute on folder: ./replace.sh xxx"
	exit
fi
echo "replace end"
