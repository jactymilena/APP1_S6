from os import listdir
from os.path import isfile, join

import csv

dir_path = 'data'
onlyfiles = [f for f in listdir(dir_path) if isfile(join(dir_path, f))]

with open('data-files2.csv', 'w', newline='') as csvfile:
    wr = csv.writer(csvfile)
#     for i, f in enumerate(onlyfiles):
#         wr.writerow([f"../data/{f};output/out-{i}.png;480"])

    for i, f in enumerate(onlyfiles):
        wr.writerow([f"../data/{onlyfiles[i % 10]};output/out-{i}.png;480"])





