import subprocess
import os

res_dir = "/opt/profile_res"

def grep_traffic(res_dir):
	print(res_dir.split("/")[-1])
	print("tenant_id,total_request,total_traffic,unique_access,unique_traffic")
	basic_cmd = "cat {} | grep {}"
	base_log_file = "tenant_{}_profile_res-FIX_RATE-REAL-0.01.log"
	for i in range(1, 7):
		log_file_path = os.path.join(res_dir, base_log_file.format(i))
		total_res = str(subprocess.run(basic_cmd.format(log_file_path, "total"), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True).stdout)
		unique_res = str(subprocess.run(basic_cmd.format(log_file_path, "unique"), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True).stdout)
		total_request = total_res.split("\n")[0].split(":")[-1].strip()
		total_traffic = total_res.split("\n")[1].split(":")[-1].strip()
		unique_access = unique_res.split("\n")[1].split(":")[-1].strip()
		unique_traffic = unique_res.split("\n")[2].split(":")[-1].strip()
		print("{},{},{},{},{}".format(i, total_request, total_traffic, unique_access, unique_traffic))

if __name__ == "__main__":
	for subdir in os.listdir(res_dir):
		sub_res_dir = os.path.join(res_dir, subdir)
		if os.path.isdir(sub_res_dir) and len(os.listdir(sub_res_dir)) == 12:
			grep_traffic(sub_res_dir)
