import pandas as pd
import numpy as np
import os
from sklearn.ensemble import ExtraTreesRegressor
from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import train_test_split
import treelite.gallery.sklearn


def csvs_to_dataframes(csv_files):
    dfs = []
    for file in csv_files:
        dfs.append(pd.read_csv(file))

    return pd.concat(dfs, ignore_index=True)


def get_arg_cols(df):
    return [x for x in df.keys() if x.startswith('arg')]


def extract_medians(df, arg_cols=None, target_field="time"):
    if arg_cols is None:
        arg_cols = get_arg_cols(df)

    return df.groupby(arg_cols).apply(
                lambda x:
                pd.Series(x[target_field].median(), index=[target_field])
            ).reset_index()


def add_percentiles(df, arg_cols=None, target_field="time"):
    def compute_rank(values, bins=10):
        if len(values) < bins:
            raise Exception("Cannot parse this kernel with {} bins it a group "
                            "only has {} samples".format(bins, len(values)))
        varray = values.to_numpy()
        svals = sorted(varray)
        nvals = len(svals)
        indices = list(np.linspace(nvals//bins, nvals - 1, bins).astype(int))
        bin_vals = np.take(svals, indices)

        for i, val in enumerate(bin_vals):
            mask = varray <= val
            varray[mask] = i

        return varray.astype(int)

    if arg_cols is None:
        arg_cols = get_arg_cols(df)

    try:
        df['argp'] = df.groupby(arg_cols)[target_field].transform(compute_rank)
    except:
        print("add_percentiles failed to add any percentiles.")


def split_test_train(df, train_size=0.8, arg_cols=None, target_field='time'):
    if arg_cols is None:
        arg_cols = get_arg_cols(df)

    return train_test_split(df[arg_cols], df[target_field],
                            train_size=train_size)


def train_ETR_model(X, y, n_jobs=-1, criterion="mse", n_estimators=100):
    model = ExtraTreesRegressor(n_jobs=n_jobs,
                                criterion=criterion, n_estimators=n_estimators)
    model.fit(X, y)
    return model


def train_RF_model(X, y, n_jobs=-1, criterion="mse", n_estimators=100):
    model = RandomForestRegressor(n_jobs=n_jobs,
                                  criterion=criterion,
                                  n_estimators=n_estimators)
    model.fit(X, y)
    return model


def compile_RF_model(model, model_path, model_name, n_files=1):
    tl_model = treelite.gallery.sklearn.import_model(model)
    model_zip = os.path.join(model_path, model_name + ".zip")
    model_so = model_name + ".so"
    tl_model.export_srcpkg(platform='unix', toolchain='gcc',
                           pkgpath=model_zip, libname="ignored.so",
                           params={'parallel_comp': n_files},
                           verbose=True)
