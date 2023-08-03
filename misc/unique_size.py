import os

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
    trace_file_dir = "../dataset/cloud_photos-sample/cloud_photos-sample.csv"
    out_file_dir = "../dataset/cloud_photos-sample/cloud_photos-sample_unique_size.csv"
    run_trace(trace_file_dir, out_file_dir)