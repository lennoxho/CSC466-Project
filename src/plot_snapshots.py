from __future__ import print_function
import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import gaussian_kde
import sys
import re

SAVE_PLOTS = True
SNAPSHOT_HEADER_RE = re.compile(r"ss \d+ \((\d+),(\d+)\):")
COORD_HEADER_RE = re.compile(r"\((-?\d+(?:\.\d+)?),(-?\d+(?:\.\d+)?)\)")

def get_snapshots(filepath):
    width = None
    height = None

    try:
        hfile = open(filepath)
    except Exception as e:
        sys.stderr.write("ERROR: " + filepath + " cannot be opened: " + str(e))
        exit(1)

    snapshots = []
    curr_snapshot = None

    for line in hfile:
        line = line.strip()
        if line == "":
            continue

        match = re.match(SNAPSHOT_HEADER_RE, line)
        if match is not None:
            new_width = int(match.group(1))
            new_height = int(match.group(2))
            if width is None:
                width = new_width
                height = new_height
            else:
                assert width == new_width
                assert height == new_height

            if curr_snapshot is not None:
                snapshots.append(curr_snapshot)
            curr_snapshot = []
            continue

        match = re.match(COORD_HEADER_RE, line)
        if match is None:
            sys.stderr.write("ERROR: " + filepath + " file format not recognized : \"" + line + "\"\n")
            exit(1)

        curr_snapshot.append((float(match.group(1)), float(match.group(2))))

    if curr_snapshot is not None:
        snapshots.append(curr_snapshot)

    return snapshots, width, height

def plot_snapshots(snapshots, width, height, ori_filepath):
    for i, ss in enumerate(snapshots):
        fig = plt.figure(figsize=(8, 8))

        ax = plt.gca()
        ax.set_xlim([-1, width+1])
        ax.set_ylim([-1, height+1])

        X = [x for x, _ in ss]
        Y = [y for _, y in ss]

        if "qp" in ori_filepath:
            XY = np.vstack([X, Y])
            Z = gaussian_kde(XY)(XY)

            ax.scatter(X, Y, c=Z, s=10, edgecolor='')
        else:
            ax.scatter(X, Y, s=5)

        plt.grid()
        plt.tight_layout()
        if SAVE_PLOTS:
            plt.savefig(ori_filepath + "." + str(i) + ".png")
        plt.show()
        plt.close()

if __name__ == "__main__":
    if (len(sys.argv) < 2):
        print("USAGE:  python plot_snapshot.py <filename> [<filename>...]")
        exit(1)

    files = sys.argv[1:]
    for filepath in files:
        filepath = filepath.strip()
        snapshots, width, height = get_snapshots(filepath)
        plot_snapshots(snapshots, width, height, filepath)