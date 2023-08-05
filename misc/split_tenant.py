import os
import subprocess
import concurrent.futures

trace_file_dir = "/data2/cloud_photo"
profile_out_dir = "/opt/profile_res"
tenant_file_dir = "/data1/tenant_trace"
split_trace_file_base = "tenant_{}_trace"
profile_res_file_base = "tenant_{}_profile_res-{}-{}-{}.csv"
profile_log_file_base = "tenant_{}_profile_res-{}-{}-{}.log"

sample_methods = ["FIX_RATE"]
sample_rates = ["0.01"]
methods = ["REAL"]


if not os.path.exists(tenant_file_dir):
    os.mkdir(tenant_file_dir)

if not os.path.exists(profile_out_dir):
    os.mkdir(profile_out_dir)

def init(day_tenant_file_dir):
    if not os.path.exists(day_tenant_file_dir):
        os.mkdir(day_tenant_file_dir)
    tanent_id_list = [1, 2, 3, 4, 5, 6]
    tmp_files_path = [os.path.join(day_tenant_file_dir, split_trace_file_base.format(tanent_id)) for tanent_id in tanent_id_list]
    tmp_files = [open(out_file_path, "w") for out_file_path in tmp_files_path]
    return tmp_files, tmp_files_path

def clean(tmp_files_path):
    for tmp_file_path in tmp_files_path:
        os.remove(tmp_file_path)

def split_trace(trace_file, tmp_files):
    cnt = 0
    with open(trace_file, "r") as f:
        f.readline()
        for line in f:
            cnt += 1
            timestamp, key, tenant_id, obj_size = line.strip().split(",")
            timestamp = timestamp.strip()
            key = key.strip()
            tenant_id = int(tenant_id.strip())
            obj_size = obj_size.strip()
            tmp_files[tenant_id-1].write("{},{},{},0\n".format(timestamp, key, obj_size))
#            if cnt % 100000 == 0:
#                print("read {} lines".format(cnt))
    for tmp_file in tmp_files:
        tmp_file.close()
        
def run_exp(trace_file, res_dir):
    print("running on {}".format(trace_file))
    tmp_tenant_file_dir = os.path.join(tenant_file_dir, res_dir)
    print(tmp_tenant_file_dir)
    if not os.path.exists(tmp_tenant_file_dir):
        os.mkdir(tmp_tenant_file_dir)
    tmp_files, tmp_files_path = init(tmp_tenant_file_dir)
    split_trace(trace_file, tmp_files)

if __name__ == "__main__":
	max_threads = 4
	with concurrent.futures.ThreadPoolExecutor(max_workers=max_threads) as executor:
		futures = []
		for trace_file in os.listdir(trace_file_dir):
			if not trace_file.endswith(".csv"):
				continue
			res_dir = trace_file.split(".")[0]
			trace_file_path = os.path.join(trace_file_dir, trace_file)
			futures.append(executor.submit(run_exp, trace_file_path, res_dir))
		concurrent.futures.wait(futures)
