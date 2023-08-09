import os
import subprocess
import concurrent.futures

trace_file_dir = "/data1/tenant_trace"
profile_out_dir = "/opt/shards_res_1000"
split_trace_file_base = "tenant_{}_trace"
profile_res_file_base = "tenant_{}_profile_res-{}-{}-{}.csv"
profile_log_file_base = "tenant_{}_profile_res-{}-{}-{}.log"

sample_methods = ["FIX_RATE"]
sample_rates = ["0.01"]
methods = ["SHARDS"]

if not os.path.exists(profile_out_dir):
    os.mkdir(profile_out_dir)

def init(day_split_trace_dir):
    tanent_id_list = [1, 2, 3, 4, 5, 6]
    tmp_files_path = [os.path.join(day_split_trace_dir, split_trace_file_base.format(tanent_id)) for tanent_id in tanent_id_list]
    return tmp_files_path
        
def run_exp(trace_dir, res_dir):
    print("running on {}".format(trace_dir))
    tmp_files_path = init(trace_dir)
    for num, tmp_file_path in enumerate(tmp_files_path):
        for sample_method in sample_methods:
            for method in methods:
                for sample_rate in sample_rates:
                    profile_res_file = os.path.join(res_dir, profile_res_file_base.format(num+1, sample_method, method, sample_rate))
                    profile_log_file = os.path.join(res_dir, profile_log_file_base.format(num+1, sample_method, method, sample_rate))
                    cmd = 'sudo bash -c "/opt/cache-allocator/third_party/flows/flows -t {} -o {} --sample_method {} --sample_metric {} --method {} --logpath {} --tracetype CSV"'
                    cmd = cmd.format(tmp_file_path, profile_res_file, sample_method, sample_rate, method, profile_log_file)
                    print(cmd)
                    subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

if __name__ == "__main__":
    max_threads = 4
    with concurrent.futures.ThreadPoolExecutor(max_workers=max_threads) as executor:
        futures = []
        for day_trace_dir in os.listdir(trace_file_dir):
            trace_dir = os.path.join(trace_file_dir, day_trace_dir)
            res_dir = os.path.join(profile_out_dir, day_trace_dir)
            print(trace_dir)
            print(res_dir)
            if not os.path.exists(res_dir):
                os.mkdir(res_dir)
            futures.append(executor.submit(run_exp, trace_dir, res_dir))
        concurrent.futures.wait(futures)
