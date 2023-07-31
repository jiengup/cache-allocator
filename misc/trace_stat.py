import os
from tqdm import tqdm

SINGLE_DAY_DURATION = 86400
now_a_days = 1

total_req = 0
total_bytes = 0

wss_obj_bytes = {}
client_id_total_req = {}
client_id_total_bytes = {}
client_id_wss_obj_byte = {}

cloud_photo_trace_dir = "../../dataset/cloud_photos-sample"

with open("cluster45.sort", "r") as f:
    req_bucket = []
    for req in f:
        timestamp, key, key_sz, value_sz, client_id, opration, ttl = req.split(",")
        total_req += 1
        total_bytes += int(value_sz)
        if key not in wss_obj_bytes.keys():
            wss_obj_bytes[key] = int(value_sz)
        else:
            wss_obj_bytes[key] += int(value_sz)
        
        if client_id not in client_id_total_req.keys():
            client_id_total_req[client_id] = 1
            client_id_total_bytes[client_id] = int(value_sz)
            client_id_wss_obj_byte[client_id] = {}
            client_id_wss_obj_byte[client_id][key] = int(value_sz)
        else:
            client_id_total_req[client_id] += 1
            client_id_total_bytes[client_id] += int(value_sz)
            if key not in client_id_wss_obj_byte[client_id].keys():
                client_id_wss_obj_byte[client_id][key] = int(value_sz)
            else:
                client_id_wss_obj_byte[client_id][key] += int(value_sz)
        
        if int(timestamp) > SINGLE_DAY_DURATION*now_a_days:
            with open("cluster45.sort.day{}".format(now_a_days), "w") as f:
                for req in req_bucket:
                    f.write(req)
            req_bucket = []
            now_a_days += 1
            print("finish process a day")

        req_bucket.append(req)

    if len(req_bucket) != 0:
         with open("cluster45.sort.remain", "w") as f:
             for req in req_bucket:
                 f.write(req) 

    with open("cluster45.stat", "w") as f:
        f.write("total_req: {}\n".format(total_req))
        f.write("total_bytes: {}\n".format(total_bytes))
        f.write("wss_obj: {}\n".format(len(wss_obj_bytes.keys())))
        wss_bytes = 0
        for key in wss_obj_bytes.keys():
            wss_bytes += wss_obj_bytes[key]
        f.write("wss_bytes: {}\n".format(wss_bytes))
        f.write("unique_client_id: {}\n".format(len(client_id_total_req.keys())))
    
    with open("cluster45_client_stat", "w") as f:
        f.write("clients total reqs: \n")
        for client_id in client_id_total_req.keys():
            f.write("{}: {}\n".format(client_id, client_id_total_req[client_id]))
        f.write("clients total bytes: \n")
        for client_id in client_id_total_bytes.keys():
            f.write("{}: {}\n".format(client_id, client_id_total_bytes[client_id]))
        f.write("clients wss objs: \n")
        for client_id in client_id_wss_obj_byte.keys():
            f.write("{}: {}\n".format(client_id, len(client_id_wss_obj_byte[client_id].keys())))
        f.write("client wss bytes: \n")
        for client_id in client_id_wss_obj_byte.keys():
            wss_bytes = 0
            for key in client_id_wss_obj_byte[client_id].keys():
                wss_bytes += client_id_wss_obj_byte[client_id][key]
            f.write("{}: {}\n".format(client_id, wss_bytes))
