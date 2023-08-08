# %%
import pandas as pd
import bisect
import os

res_root_dir = "/root/cacheAllocation/result/flows_res_1000"
# %%
def find_first_greater_equal(arr, x):
    index = bisect.bisect_right(arr, x)
    if index < len(arr):
        return index
    else:
        return len(arr)
# %%
GB_base = 1024 * 1024 * 1024
# %%
def linear_interpolation(result_file):
    result_df = pd.read_csv(result_file, skipinitialspace=True)
    cache_size = [0]
    bmrc_ratio = [1.0]
    omrc_ratio = [1.0]
    cache_size.extend(result_df['cache_size'].to_list())
    bmrc_ratio.extend(result_df['BMRC_ratio'].to_list())
    omrc_ratio.extend(result_df['OMRC_ratio'].to_list())
    inter_result_f_path = os.path.join(os.path.dirname(result_file), result_file.split("/")[-1].split(".csv")[0] + "_inter.csv")
    with open(inter_result_f_path, "w") as f:
        f.write("cache_size,BMRC_ratio,OMRC_ratio\n")
        for i in range(1, 3073):
            inter_cache_size = i * GB_base
            if inter_cache_size >= cache_size[-1]:
                f.write("{},{:.6f},{:.6f}\n".format(inter_cache_size, bmrc_ratio[-1], omrc_ratio[-1]))
            else:
                first_bigger_index = find_first_greater_equal(cache_size, inter_cache_size)
                bigger_bmrc_ratio = bmrc_ratio[first_bigger_index]
                smaller_bmrc_ratio = bmrc_ratio[first_bigger_index - 1]
                bigger_omrc_ratio = omrc_ratio[first_bigger_index]
                smaller_omrc_ratio = omrc_ratio[first_bigger_index - 1]
                inter_bmrc_ratio = smaller_bmrc_ratio + (inter_cache_size - cache_size[first_bigger_index - 1]) / (cache_size[first_bigger_index] - cache_size[first_bigger_index - 1]) * (bigger_bmrc_ratio - smaller_bmrc_ratio)
                inter_omrc_ratio = smaller_omrc_ratio + (inter_cache_size - cache_size[first_bigger_index - 1]) / (cache_size[first_bigger_index] - cache_size[first_bigger_index - 1]) * (bigger_omrc_ratio - smaller_omrc_ratio)
                f.write("{},{:.6f},{:.6f}\n".format(inter_cache_size, inter_bmrc_ratio, inter_omrc_ratio))
# %%
for res_dir_f in os.listdir(res_root_dir):
    if os.path.isdir(os.path.join(res_root_dir, res_dir_f)):
        res_dir = os.path.join(res_root_dir, res_dir_f)
        for res_f in os.listdir(res_dir):
            if res_f.endswith(".csv"):
                res_file = os.path.join(res_dir, res_f)
                linear_interpolation(res_file)
