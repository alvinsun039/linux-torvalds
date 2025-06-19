#!/bin/bash
path=$(cd $(dirname $0); pwd) 
func_file="$path/fun.txt"
macro_file="$path/macro.txt"
struct_file="$path/struct.txt"
collect_file="$path/collect.sh"
replace_file="$path/replace.sh"

PARAMS=(
    "audio on"
    "common on"
    "gpu on"
    "gpu_test on"
    "ip on"
    "kms on"
    "vpu on"
    "mcu_peripherals on"
)

if [ ! -f "$collect_file" ]; then
	echo "$collect_file not exist"
	exit
fi

for param in "${PARAMS[@]}"; do
	echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	IFS=' ' read -ra args <<< "$param"
  
	case ${#args[@]} in
	1)
            echo "collecting folder: ${args[0]}"
	    if [ "$1" = "on" ];then
            	$collect_file "${args[0]}" &
	    else
            	$collect_file "${args[0]}"
	    fi
            ;;
        2)
            echo "collecting folder: ${args[0]} ${args[1]}"
	    if [ "$1" = "on" ];then
            	$collect_file "${args[0]}" "${args[1]}" &
	    else
            	$collect_file "${args[0]}" "${args[1]}"
	    fi
            ;;
        *)
            echo " error (${param})"
            ;;
    esac
done

wait
#-----------------delete struct-----------------------------
STR_DEL=(
    "gbdc_plane"
    "gbdc_plane_state"
    "gbdc_crtc"
    "gbdc_crtc_state"
    "gbdc_connector"
    "gbdc_connector_state"
    "gb_devfreq"
    "gpu_info"
)

if [ ! -f "$struct_file" ]; then
    echo "$struct_file not exist"
    exit 1
fi

TMP_FILE=$(mktemp)

while IFS= read -r line; do
    delete_line=0
    for pattern in "${STR_DEL[@]}"; do
        if [[ "$line" == *"$pattern"* ]]; then
            delete_line=1
            break
        fi
    done
    
    if [ "$delete_line" -eq 0 ]; then
        echo "$line" >> "$TMP_FILE"
    fi
done < "$struct_file"

mv "$TMP_FILE" "$struct_file"

echo "$struct_file filtering completeed"

#-----------------delete func-----------------------------
FUNC_DEL=(
	"__attribute"
	"drm_mode_vrefresh"
	"infinity_info_show"
	"infinity_state_show"
	"infinity_prop_show"
	"__genbu_drm_atomic_helper_plane_duplicate_state"
	"name_show"
	"available_governors_show"
	"cur_freq_show"
	"target_freq_show"
	"available_frequencies_show"
	"trans_stat_show"
	"governor_store"
	"polling_interval_store"
	"min_freq_show"
	"max_freq_store"
	"max_freq_show"
	"log_ctrl_show"
	"log_ctrl_store"
	"gb02_tpu_store"
	"direction_store"
	"value_store"
	"emul_temp_store"
	"perf_test_store"
	"governor_show"
	"polling_interval_show"
	"min_freq_store" 
	"direction_show"
	"gb02_tpu_show"
	"unexport_store"
	"export_store"
	"show_gpu_info"
	"value_show"
	"value_store"
)

if [ ! -f "$func_file" ]; then
    echo "$func_file not exist"
    exit 1
fi

TMP_FILE=$(mktemp)

while IFS= read -r line; do
    delete_line=0
    for pattern in "${FUNC_DEL[@]}"; do
        if [[ "$line" == *"$pattern"* ]]; then
            delete_line=1
            break
        fi
    done
    
    if [ "$delete_line" -eq 0 ]; then
        echo "$line" >> "$TMP_FILE"
    fi
done < "$func_file"

mv "$TMP_FILE" "$func_file"

echo "$func_file filtering completed"

#-----------------delete MAC-----------------------------
MAC_DEL=(
"AS_PRESENT"
"SS_PRESENT"
"L2_FEATURES"
"TILER_FEATURES"
"MEM_FEATURES"
"MMU_FEATURES"
"THREAD_FEATURES"
"COHERENCY_FEATURES"
)

if [ ! -f "$macro_file" ]; then
    echo "$macro_file not exist"
    exit 1
fi

TMP_FILE=$(mktemp)

while IFS= read -r line; do
    delete_line=0
    for pattern in "${MAC_DEL[@]}"; do
        if [[ "$line" == *"$pattern"* ]]; then
            delete_line=1
            break
        fi
    done
    
    if [ "$delete_line" -eq 0 ]; then
        echo "$line" >> "$TMP_FILE"
    fi
done < "$macro_file"

mv "$TMP_FILE" "$macro_file"

echo "$macro_file filtering completed"
#--------------------------------replae-------
if [ ! -f "$replace_file" ]; then
	echo "$replace_file not exist"
	exit
fi

for param in "${PARAMS[@]}"; do
	echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	IFS=' ' read -ra args <<< "$param"
  
	case ${#args[@]} in
	1)
            echo "replace folder: ${args[0]}"
	    if [ "$1" = "on" ];then
            	$replace_file "${args[0]}" &
	    else
            	$replace_file "${args[0]}"
	    fi
            ;;
        2)
            echo "replace folder: ${args[0]} ${args[1]}"
	    if [ "$1" = "on" ];then
            	$replace_file "${args[0]}" "${args[1]}" &
	    else
            	$replace_file "${args[0]}" "${args[1]}"
	    fi
            ;;
        *)
            echo " error (${param})"
            ;;
    esac
done

wait
echo "complete all"
