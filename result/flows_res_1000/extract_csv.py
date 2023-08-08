import os

for res in os.listdir(os.getcwd()):
    if os.path.isdir(res):
        res_dir = os.path.join(os.getcwd(), res)
        for f in os.listdir(res_dir):
            if f.endswith(".log"):
                res_file = os.path.join(res_dir, f)
                csv_file = os.path.join(res_dir, f.split(".log")[0] + ".csv")
                with open(res_file, "r") as rf:
                    lines = rf.readlines()
                    with open(csv_file, "w") as wf:
                        for line in lines[-1002:-2]:
                            wf.write(line)