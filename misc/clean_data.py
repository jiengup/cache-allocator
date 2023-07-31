import pandas as pd
import os

cloud_photo_dir = "/data1/cloud_photo"
clean_cloud_photo_dir = "/data1/cloud_photo/clean"
sample_file_p = "/data1/cloud_photo/2016020200-2016020300.csv_sort.csv"
valid_tenant_id = range(1, 7)

column_dict = {
    "timestamp": 0,
    "photo_id": 1,
    "size_spec": 2,
    "return_size": 3,
}


if not os.path.exists(clean_cloud_photo_dir):
    os.mkdir(clean_cloud_photo_dir)

def clean_data(file_path):
    data_df = pd.read_csv(file_path, header=None)
    data_df = data_df.drop(data_df.index[-1])
    print("tail:\n")
    print(data_df.tail())
    data_df = data_df.rename(columns={column_dict["timestamp"]: "timestamp",
                            column_dict["photo_id"]: "key",
                            column_dict["size_spec"]: "tnt_id", 
                            column_dict["return_size"]: "size"})
    invalid_rows = ~data_df["tnt_id"].isin(valid_tenant_id)
    print(data_df[invalid_rows])
    data_df = data_df[~invalid_rows]
    data_df.to_csv(os.path.join(clean_cloud_photo_dir, file_path.split("/")[-1]), header=True, index=False, sep=",")

if __name__ == "__main__":
    clean_data(sample_file_p)
