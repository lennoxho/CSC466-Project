from __future__ import print_function
import matplotlib.pyplot as plt
import sys

def get_points(filepath):
    try:
        hfile = open(filepath)
    except Exception as e:
        sys.stderr.write("ERROR: " + filepath + " cannot be opened: " + str(e))
        exit(1)

    points = []
    for line in hfile:
        line = line.strip()
        if line != "":
            points.append(int(line))

    return points

def plot_iterates(y_plot, ori_filepath):
    fig = plt.figure(figsize=(15, 8))

    plt.plot(y_plot)

    plt.xlabel('k, iterations')
    plt.ylabel('BBOX distance')

    plt.grid()
    plt.tight_layout()
    plt.show()
    #plt.savefig(ori_filepath + ".png")
    #plt.close()

if __name__ == "__main__":
    if (len(sys.argv) < 2):
        print("USAGE:  python plot_iterations.py <filename> [<filename>...]")
        exit(1)

    files = sys.argv[1:]
    for filepath in files:
        filepath = filepath.strip()
        y_plot = get_points(filepath)
        plot_iterates(y_plot, filepath)