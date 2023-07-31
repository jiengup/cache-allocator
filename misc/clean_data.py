import pandas as pd
import os

cloud_photo_dir = "/data1/cloud_photo"
clean_cloud_photo_dir = "/data1/cloud_photo/clean"
sample_file_p = "/data1/cloud_photo/2016020100-2016020200.csv_sort.csv"
valid_tenant_id = range(1, 7)

if not os.path.exists(clean_cloud_photo_dir):
    os.mkdir(clean_cloud_photo_dir)

def clean_data(file_path):
    data_df = pd.read_csv(file_path)
    invalid_rows = ~data_df['tenant_id'].isin(valid_tenat_id)
    print(invalid_rows)
    data_df = data_df[~invalid_rwos]
    data_df.to_csv(os.path.join(clean_cloud_photo_dir, file_path.split("/")[-1]), header=True, index=False, sep=",")

if __name__ == "__main__":
    clean_data(sample_file_p)
