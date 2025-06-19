#! /usr/bin/env bash

chmod 700 /usr/local/sietium/gb02sf_*
chmod 600 /usr/local/sietium/sietium_gb02_mcu_*

MCU_FW=""
MCU_UPGRADE_TOOL=""

show_tips()
{
	echo "Upgrade Bsp firmware? [Y/N]"
	for ((i=9; i>=0; i--))
	do
		echo -ne "\r"
		#echo -ne "                              "
		echo -ne "\r"
		echo  -n "        剩余时间:"$i"秒"
		sleep 1
	done
}
fw_upgrade()
{
	#read_input
	show_tips &
	read  -t 10 INPUT_VAL
	kill $!
	if [ -z "$INPUT_VAL" ];then
		echo -e "\nDefault Do Nothing."
		exit 0
	fi
	if [ "$INPUT_VAL" == "Y" ];then
		echo "You Select Yes."
		$MCU_UPGRADE_TOOL "FULL" $MCU_FW
		exit 0
	fi
	if [ "$INPUT_VAL" == "N" ];then
		echo "You Select No."
		exit 0;
	fi
	echo "Invalid Input :"$INPUT_VAL",Please Input Y/N."
}
if [ -n "$(lspci -n | grep 8510:0201)" ];then
	CUR_ARCH=$(arch)
	MCU_UPGRADE_TOOL="/usr/local/sietium/gb02sf_"${CUR_ARCH}"*"
	#echo $MCU_UPGRADE_TOOL
	MCU_FW="/usr/local/sietium/sietium_gb02_mcu_*"
	#echo $MCU_FW
	#get current run vbios version
	CUR_VBIOS_VER=$($MCU_UPGRADE_TOOL "CVV" $MCU_FW)
	CUR_VBIOS_VER=$(echo $CUR_VBIOS_VER | awk -F'vbios version: ' '{print $2}')
	#get upgrade image's vbios version
	UPDATE_VBIOS_VER=$(cat $MCU_FW | tr -d '\0' | grep -ia "vbios_v")
	UPDATE_VBIOS_VER=$(echo $UPDATE_VBIOS_VER | awk -F'vbios_' '{print $2}')
	UPDATE_VBIOS_VER=$(echo $UPDATE_VBIOS_VER | awk -F' ' '{print $1}')
	echo "Current Vbios Ver:"${CUR_VBIOS_VER}
	echo "Local Vbios Ver:"${UPDATE_VBIOS_VER}
	if [ "$CUR_VBIOS_VER" != "$UPDATE_VBIOS_VER" ];then
		fw_upgrade
		exit 0
	else
		echo "No Need Upgrade Mcu vbios."
	fi

	#get current run optrom version
	CUR_OPTROM_VER=$($MCU_UPGRADE_TOOL "CUV" $MCU_FW)
	CUR_OPTROM_VER=$(echo $CUR_OPTROM_VER | awk -F'option rom version: ' '{print $2}')
	#get upgrade image's optrom version
	UPDATE_OPTROM_VER=$(cat $MCU_FW | tr -d '\0' | grep -ia "sietium_optrom_v")
	UPDATE_OPTROM_VER=$(echo $UPDATE_OPTROM_VER | awk -F'sietium_optrom_' '{print $2}')
	UPDATE_OPTROM_VER=$(echo $UPDATE_OPTROM_VER | awk -F' ' '{print $1}')
	echo "Current Optrom Ver:"${CUR_OPTROM_VER}
	echo "Local Optrom Ver:"${UPDATE_OPTROM_VER}
	if [ "$CUR_OPTROM_VER" != "$UPDATE_OPTROM_VER" ];then
		fw_upgrade
	else
		echo "No Need Upgrade Mcu optrom."
	fi
else
	echo "No GB02 Graphics Card Found!"
fi
