import os
import subprocess

trace_file_dir = "/data1/cloud_photo"
profile_out_dir = "/data1/profile_res"
tmp_file_dir = "/data1/tmp"
split_trace_file_base = "tenant_{}_trace"
profile_res_file_base = "tenant_{}_profile_res-{}-{}-{}.csv"

sample_methods = ["FIX_RATE"]
sample_rates = ["0.01"]
methods = ["REAL"]


if not os.path.exists(tmp_file_dir):
    os.mkdir(tmp_file_dir)

if not os.path.exists(profile_out_dir):
    os.mkdir(profile_out_dir)

def init():
    tanent_id_list = [1, 2, 3, 4, 5, 6]
    tmp_files_path = [os.path.join(tmp_file_dir, split_trace_file_base.format(tanent_id)) for tanent_id in tanent_id_list]
    tmp_files = [open(out_file_path, "w") for out_file_path in tmp_files_path]
    return tmp_files, tmp_files_path

def clean(tmp_files_path):
    for tmp_file_path in tmp_files_path:
        os.remove(tmp_file_path)

def split_trace(trace_file, tmp_files):
    with open(trace_file, "r") as f:
        f.readline()
        for line in f:
            timestamp, key, tenant_id, obj_size = line.strip().split(",")
            timestamp = timestamp.strip()
            key = key.strip()
            tenant_id = int(tenant_id.strip())
            obj_size = obj_size.strip()
            tmp_files[tenant_id-1].write("{},{},{},0\n".format(timestamp, key, obj_size))
    for tmp_file in tmp_files:
        tmp_file.close()
        
def run_exp(trace_file):
    print("running on {}".format(trace_file))
    tmp_files, tmp_files_path = init()
    # split_trace(trace_file, tmp_files)
    trace_res_dir = os.path.join(profile_out_dir, trace_file.split(".")[0])
    if not os.path.exists(trace_res_dir):
        os.mkdir(trace_res_dir)
    for num, tmp_file_path in enumerate(tmp_files_path):
        for sample_method in sample_methods:
            for method in methods:
                for sample_rate in sample_rates:
                    profile_res_file = os.path.join(trace_res_dir, profile_res_file_base.format(num+1, sample_method, method, sample_rate))
                    cmd = "/opt/cache-allocation/third_party/flows/flows -t {} -o {} --sample_method {} --sample_metric {} --method {} > {}"
                    cmd = cmd.format(tmp_file_path, profile_res_file, sample_method, sample_rate, method, profile_res_file)
                    print(cmd)
                    subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    clean(tmp_files_path)

if __name__ == "__main__":
    for trace_file in os.listdir(trace_file_dir):
        if trace_file.endswith(".csv"):
            trace_file_path = os.path.join(trace_file_dir, trace_file)
            run_exp(trace_file_path)
            exit(0)
