import csv
import heapq

def external_sort(input_file, output_file, key_column):
    chunk_size = 100000  # 每个块的大小，根据实际情况进行调整
    chunks = []

    # 第一阶段：分割CSV文件为多个块并进行内部排序
    with open(input_file, 'r') as file:
        reader = csv.reader(file)
        header = next(reader)  # 读取文件头部并保存
        chunk = []

        for row in reader:
            chunk.append(row)
            if len(chunk) >= chunk_size:
                chunk.sort(key=lambda x: x[key_column])  # 根据指定列进行内部排序
                chunks.append(chunk)
                chunk = []

        if chunk:
            chunk.sort(key=lambda x: x[key_column])  # 对剩余的数据进行内部排序
            chunks.append(chunk)

    # 第二阶段：多路归并排序并输出到结果文件
    with open(output_file, 'w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(header)  # 写入文件头部

        heap = []
        indexes = [0] * len(chunks)  # 每个块的当前读取索引

        # 初始化堆，将每个块的第一行数据加入堆中
        for i, chunk in enumerate(chunks):
            row = chunk[indexes[i]]
            heapq.heappush(heap, (row[key_column], i, row))

        # 多路归并排序
        while heap:
            _, chunk_index, row = heapq.heappop(heap)
            writer.writerow(row)

            indexes[chunk_index] += 1
            chunk = chunks[chunk_index]

            if indexes[chunk_index] < len(chunk):
                row = chunk[indexes[chunk_index]]
                heapq.heappush(heap, (row[key_column], chunk_index, row))
            else:
                del chunk[:]  # 释放已处理的块的内存

    # 删除临时的中间块文件
    for chunk in chunks:
        del chunk

if __name__ == '__main__':
    input_file = '/root/cacheAllocation/dataset/cloud_photos-sample/cloud_photos-sample.csv'  # 输入的超大CSV文件路径
    output_file = 'sorted_file.csv'  # 输出的排序后的文件路径
    key_column = 0  # 根据哪一列进行排序，这里假设按第一列进行排序

    external_sort(input_file, output_file, key_column)