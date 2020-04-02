import pandas as pd

def csvs_to_dataframes(csv_files):
    for file in csv_files:
        df = pd.read_csv(file)
        print(len(df))
