import os, time, re, sys

def main(path, bb):
    pattern = r"[0-9]+"
    fen = r"[0-9]+x\*[0-9]+"
    hw_pattern = r"\.[0-9]{6}"
    stage_pattern = r"stage:[a-f0-9]{16}"
    content = []
    ret_lit = {
        "gb_edma_drm_start": {
            "cnt": 0,
            "edma_start": [],
            "start_trans": [],
            "ioctl":[],
            "per_time": 0,
            "tran_time": 0,
            "tran_size": 0,
            "channel": 0,
            "register": 0,
            "transfer": 0,
            "ioct_total": 0,
            "get_user": 0,
            "get_gpu": 0,
            "start_func":0,
            "size":{},

        },
        "gb_edma_gpu_start": {
            "cnt": 0,
            "edma_start": [],
            "start_trans": [],
            "ioctl":[],
            "per_time": 0,
            "tran_time": 0,
            "tran_size": 0,
            "channel": 0,
            "register": 0,
            "transfer": 0,
            "ioct_total": 0,
            "get_user": 0,
            "get_gpu": 0,
            "start_func":0,
            "size":{},
        },
        "gpu": {
            "cnt": 0,
            "stage_time": {},
            "stage_total": 0
        }
    }
    ram =  ret_lit["gb_edma_gpu_start"]
    vram = ret_lit["gb_edma_drm_start"]
    gpu = ret_lit["gpu"]
 #   abc = 0
    with open(path , "r") as fp:
        content = fp.readlines()
    stage = ""
    for line in content:
        line = line.split("]")[-1]
        tmp_list = re.findall(pattern, line)
        fen_list = re.findall(fen, line)
        fen_list.append(0)
        tmp_list.append(0)

        if "gb_edma_gpu_start" in line:
            ret_lit["gb_edma_gpu_start"]["cnt"] += 1
            ret_lit["gb_edma_gpu_start"]["edma_start"].append(tmp_list)
            ret_lit["gb_edma_gpu_start"]["per_time"] += int(tmp_list[0])
            ret_lit["gb_edma_gpu_start"]["tran_time"] += int(tmp_list[1])
            ret_lit["gb_edma_gpu_start"]["tran_size"] += int(tmp_list[2])
            if ram["size"].get(tmp_list[2]) is None:
                ram["size"][tmp_list[2]] = 1
            else:
                ram["size"][tmp_list[2]] += 1

        if "gb_edma_drm_start" in line:
            ret_lit["gb_edma_drm_start"]["cnt"] += 1
            ret_lit["gb_edma_drm_start"]["edma_start"].append(tmp_list)
            ret_lit["gb_edma_drm_start"]["per_time"] += int(tmp_list[0])
            ret_lit["gb_edma_drm_start"]["tran_time"] += int(tmp_list[1])
            ret_lit["gb_edma_drm_start"]["tran_size"] += int(tmp_list[2])
            if vram["size"].get(tmp_list[2]) is None:
                vram["size"][tmp_list[2]] = [fen_list[0], 1]
            else:
                vram["size"][tmp_list[2]][1] += 1
 #           print(vram["size"])

        if "gb_edma_ll_mode_start" in line and "aaa" in line:
            ret_lit["gb_edma_gpu_start"]["start_trans"].append(tmp_list)
            ret_lit["gb_edma_gpu_start"]["channel"] += int(tmp_list[0])
            ret_lit["gb_edma_gpu_start"]["register"] += int(tmp_list[1])
            ret_lit["gb_edma_gpu_start"]["transfer"] += int(tmp_list[2])
        elif "gb_edma_ll_mode_start" in line:
            ret_lit["gb_edma_drm_start"]["start_trans"].append(tmp_list)
            ret_lit["gb_edma_drm_start"]["channel"] += int(tmp_list[0])
            ret_lit["gb_edma_drm_start"]["register"] += int(tmp_list[1])
            ret_lit["gb_edma_drm_start"]["transfer"] += int(tmp_list[2])

        if "gb_edma_translate_to_fb" in line:
            ret_lit["gb_edma_drm_start"]["ioctl"].append(tmp_list)
            ret_lit["gb_edma_drm_start"]["get_user"] += int(tmp_list[0])
            ret_lit["gb_edma_drm_start"]["get_gpu"] += int(tmp_list[1])
            ret_lit["gb_edma_drm_start"]["start_func"] += int(tmp_list[2])
            ret_lit["gb_edma_drm_start"]["ioct_total"] += int(tmp_list[0]) + int(tmp_list[1]) + int(tmp_list[2])

        if "gb_edma_translate_to_ram" in line:
            ret_lit["gb_edma_gpu_start"]["ioctl"].append(tmp_list)
            ret_lit["gb_edma_gpu_start"]["get_user"] += int(tmp_list[0])
            ret_lit["gb_edma_gpu_start"]["get_gpu"] += int(tmp_list[1])
            ret_lit["gb_edma_gpu_start"]["start_func"] += int(tmp_list[2])
            ret_lit["gb_edma_gpu_start"]["ioct_total"] += int(tmp_list[0]) + int(tmp_list[1]) + int(tmp_list[2])
 
#        if "gb_stage_hw_submit" in line:
#            time = int(re.findall(hw_pattern, line)[0][1:])
#            stage = re.findall(stage_pattern, line)[0] + " " + str(abc)
#            print(stage)
#            gpu["cnt"] += 1
#            if gpu["stage_time"].get(stage) is None:
#                gpu["stage_time"][stage] = [time , 0]


#        if "gb_stage_irq_handler" in line:
#            time = int(re.findall(hw_pattern, line)[0][1:])
#            #stage = re.findall(stage_pattern, line)[0] + " " + str(abc)
#            gpu["stage_time"][stage][1] = time
#            gpu["stage_total"] += gpu["stage_time"][stage][1] - gpu["stage_time"][stage][0]
#            stage = ""
 #       abc += 1


        tmp_list = []

    bb_path  = "./dma_result{}.log".format(bb)
    print(bb_path)
    with open(bb_path , "w") as fp:

        ram_cnt = ram["cnt"] if  ram["cnt"] else 1
        vram_cnt = vram["cnt"] if vram["cnt"] else 1
        print(ram_cnt, vram_cnt)
        fp.write("-{:20s}-{:>20s}-{:>20s}-\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("", "to RAM",   "to Vram"))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("ioctl total", ram["ioct_total"], vram["ioct_total"]))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("ioctl averge", ram["ioct_total"] / ram_cnt, vram["ioct_total"] / vram_cnt))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("software total", ram["ioct_total"] - ram["transfer"], vram["ioct_total"] - vram["transfer"]))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("software average", (ram["ioct_total"] - ram["transfer"]) / ram_cnt, (vram["ioct_total"] - vram["transfer"])/ vram_cnt))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("get user", ram["get_user"], vram["get_user"]))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("get user average", ram["get_user"]/ ram_cnt, vram["get_user"]/ vram_cnt))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("get gpu", ram["get_gpu"], vram["get_gpu"]))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("get gpu average", ram["get_gpu"]/ ram_cnt, vram["get_gpu"]/ vram_cnt))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("create ll", ram["per_time"], vram["per_time"]))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("create ll average", ram["per_time"]/ ram_cnt, vram["per_time"]/ vram_cnt))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("get channel", ram["channel"], vram["channel"]))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("get channel average", ram["channel"]/ ram_cnt, vram["channel"]/ vram_cnt))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("dma trans", ram["transfer"], vram["transfer"]))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>17.0f} us|{:>17.0f} us|\n".format("dma trans average", ram["transfer"]/ ram_cnt, vram["transfer"]/ vram_cnt))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>18.0f}MB|{:>18.0f}MB|\n".format("tran size", ram["tran_size"] /1024 /1024, vram["tran_size"]/1024/1024))
        fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
        fp.write("|{:20s}|{:>20.0f}|{:>20.0f}|\n".format("cnt",ram_cnt , vram_cnt))
        cou_size = 0
        ram_size_cnt = len(ram["size"].keys())
        ram_items = list(ram["size"].keys())
#        print(ram_items)
        vram_size_cnt = len(vram["size"].keys())
        vram_items = list(vram["size"].keys())
#        print(vram_items)
        cou_size = ram_size_cnt if ram_size_cnt > vram_size_cnt else vram_size_cnt
        for i in range(cou_size):
            a = 0
            b = 0
            c = 0
            d = 0
            e = 0
            f = 0
            g = ""
            if i < ram_size_cnt :
                a = int(ram_items[i])/ 1024 / 1024
                b = ram["size"][ram_items[i]]
                c = a * b

            if i < vram_size_cnt :
                e = int(vram_items[i])/ 1024 / 1024
                d = vram["size"][vram_items[i]][1]
                f = d * e
                g = vram["size"][vram_items[i]][0]
            fp.write("|{:20s}|{:>20s}|{:>20s}|\n".format("-"*20, "-"*20, "-"*20))
            fp.write("|{:20s}|{:>4.3f}MB|{:>4.0f}|{:>5.0f}MB|{:>4.3f}MB|{:>4.0f}|{:>5.0f}MB|\n".format(g, a, b, c, e ,d,f))
        fp.write("-{:20s}-{:>20s}-{:>20s}-\n".format("-"*20, "-"*20, "-"*20))
#        print(gpu["cnt"])
#        print(gpu["stage_total"])
#        print(gpu["stage_total"]/gpu["cnt"])





if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
