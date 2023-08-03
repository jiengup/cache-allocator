import os

origin_trace_dir = "/data1/cloud_photo"
out_trace_dir = "/data2/cloud_photo"

def run_trace(trace_file, out_file):
    tenant_req = {
        "1": {},
        "2": {},
        "3": {},
        "4": {},
        "5": {},
        "6": {},
    }
    with open(trace_file, "r") as f:
        f.readline()
        for line in f:
            timestamp, key, tenant_id, obj_size = line.strip().split(",")
            timestamp = timestamp.strip()
            key = key.strip()
            tenant_id = tenant_id.strip()
            obj_size = obj_size.strip()
            if key not in tenant_req[tenant_id].keys():
                tenant_req[tenant_id][key] = {"max": 0, "max_item": None}
            if obj_size not in tenant_req[tenant_id][key].keys():
                tenant_req[tenant_id][key][obj_size] = 1
                if tenant_req[tenant_id][key][obj_size] > tenant_req[tenant_id][key]["max"]:
                    tenant_req[tenant_id][key]["max"] = tenant_req[tenant_id][key][obj_size]
                    tenant_req[tenant_id][key]["max_item"] = obj_size
            else:
                tenant_req[tenant_id][key][obj_size] += 1
                if tenant_req[tenant_id][key][obj_size] > tenant_req[tenant_id][key]["max"]:
                    tenant_req[tenant_id][key]["max"] = tenant_req[tenant_id][key][obj_size]
                    tenant_req[tenant_id][key]["max_item"] = obj_size
    
    with open(trace_file, "r") as fin:
        with open(out_file, "w") as fout:
            line = fin.readline()
            fout.write(line)
            for line in fin:
                timestamp, key, tenant_id, obj_size = line.strip().split(",")
                timestamp = timestamp.strip()
                key = key.strip()
                tenant_id = tenant_id.strip()
                obj_size = obj_size.strip()
                fout.write("{},{},{},{}\n".format(timestamp, key, tenant_id, tenant_req[tenant_id][key]["max_item"]))
    
if __name__ == "__main__":
    for origin_trace in os.listdir(origin_trace_dir):
        if "csv" not in origin_trace:
            continue
        print("running on {}".format(origin_trace))
        origin_trace_file = os.path.join(origin_trace_dir, origin_trace)
        out_trace_file = os.path.join(out_trace_dir, origin_trace.split(".")[0]+"_unique"+".csv")
        print("writing to {}".format(out_trace_file))
        run_trace(origin_trace_file, out_trace_file)
