import os
import pandas as pd
import concurrent.futures

src_dir = "/data1/cloud_photo/hust-Tencent_Cloud_Photo_Storage_Cache_Trace"
trace_dirs = [trace_dir for trace_dir in os.listdir("/data1/cloud_photo/hust-Tencent_Cloud_Photo_Storage_Cache_Trace") if os.path.isdir(os.path.join(src_dir, trace_dir))]
trace_dirs = [os.path.join(src_dir, trace_dir) for trace_dir in trace_dirs]
des_dir = "/data2/cloud_photo"

durations = [
#     ("2016020100", "2016020200"),
    ("2016020200", "2016020300"),
    ("2016020300", "2016020400"),
    ("2016020400", "2016020500"),
    ("2016020500", "2016020600"),
    ("2016020600", "2016020700"),
    ("2016020700", "2016020800"),
    ("2016020800", "2016020900"),
    ("2016020900", "2016021000")
]

column_dict = {
    "timestamp": 0,
    "photo_id": 1,
    "image_format": 2,
    "size_spec": 3,
    "return_size": 4,
    "cache_hit": 5, 
    "terminal_type": 6,
    "response_time": 7
}

size_spec_tnt_map = {
    "l": 1,
    "a": 2,
    "o": 3,
    "m": 4,
    "c": 5,
    "b": 6,
}

def trans_size_spec_to_tntid(df: pd.DataFrame) -> pd.DataFrame:
    df[column_dict["size_spec"]] = df[column_dict["size_spec"]].map(lambda x: size_spec_tnt_map[x])
    return df

def drop_useless(df: pd.DataFrame) -> pd.DataFrame:
    df = df[df[column_dict["image_format"]].isin([0, 5])]
    df = df.iloc[:, [column_dict["timestamp"], column_dict["photo_id"], column_dict["size_spec"], column_dict["return_size"]]]
    df = df.rename(columns={column_dict["timestamp"]: "timestamp",
                            column_dict["photo_id"]:"key",
                            column_dict["size_spec"]: "tnt_id", 
                            column_dict["return_size"]: "size"})
    return df

def make_duration_trace(st_d, ed_d):
    print("writing to {}".format(os.path.join(des_dir, "{}-{}_sorted.csv".format(st_d, ed_d))))
    st_d = int(st_d)
    ed_d = int(ed_d)
    df = pd.DataFrame()
    for duration in range(st_d, ed_d):
        duration = str(duration)
        for trace_dir in trace_dirs:
            duration_dir = os.path.join(trace_dir, duration)
            if not os.path.exists(duration_dir):
                print("No such directory: {}".format(duration_dir))
                continue
            for trace_file in os.listdir(duration_dir):
                trace_file_path = os.path.join(duration_dir, trace_file)
                print(trace_file_path)
                trace_df = pd.read_csv(trace_file_path, sep=" ", header=None)
                trace_df = trans_size_spec_to_tntid(trace_df)
                trace_df = drop_useless(trace_df)
                df = pd.concat([df, trace_df], ignore_index=True)
                print(df.shape)
    print("{}-{} final shape: ".format(st_d, ed_d), df.shape)
#    df = df.sort_values(by='timestamp', stable=True)
    df.to_csv(os.path.join(des_dir, "{}-{}_sorted.csv".format(st_d, ed_d)), index=False, header=True, sep=",")


if __name__ == "__main__":
    with concurrent.futures.ThreadPoolExecutor() as executor:
        futures = [executor.submit(make_duration_trace, st_duration, ed_duration) for st_duration, ed_duration in durations]
        concurrent.futures.wait(futures)

