import pandas as pd

cloud_photo_trace_path = "../dataset/cloud_photos-sample/cloud_photos-sample"

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
    print(df)
    return df

if __name__ == "__main__":
    trace_df = pd.read_csv(cloud_photo_trace_path, sep=" ", header=None)
    trace_df = trans_size_spec_to_tntid(trace_df)
    trace_df = drop_useless(trace_df)
    trace_df.to_csv("../dataset/cloud_photos-sample/cloud_photos-sample.csv", index=False, header=True, sep=",")