import os
import sys
from pathlib import Path
from statistics import stdev, mean

"""
Print stats of benchmarks in './benchmarks' folder.
If FILTER is given, only benchmarks containing FILTER in their filename
are taken into account.

Usage:
    python stats.py [FILTER]
e.g.
    python stats.py
    python stats.py comp02
"""

benchmarks_folder = "results/benchmarks"


def values_subset(values):
    # return values[1:-1]
    return values


def tabulate(headers, data):
    if len(headers) != len(data):
        return
    SEP = "  |  "
    COLS = len(headers)
    ROWS = max(len(data[i]) for i in range(COLS))
    lengths = [0] * COLS
    for c in range(COLS):
        lengths[c] = max(lengths[c], len(headers[c]))
        for item in data[c]:
            lengths[c] = max(lengths[c], len(item))

    s = SEP.join([header.ljust(lengths[c]) for c, header in enumerate(headers)])
    s = s + "\n" + "-" * len(s) + "\n"
    for r in range(ROWS):
        s += SEP.join(data[c][r].ljust(lengths[c]) for c in range(COLS)) + "\n"
    print(s, end="")


def compute_benchmark_stats(benchmark_file):
    if not benchmark_file.exists():
        return None
    costs = []
    with benchmark_file.open() as bench:
        for line in bench:
            if line.startswith("#"):
                continue
            fields = line.split()
            if not fields:
                continue
            costs.append(int(fields[len(fields) - 1]))
    if not costs:
        return None
    costs = values_subset(costs)
    if not costs:
        return None
    best = min([c for c in costs])
    avg = mean(costs)
    std = stdev(costs) if len(costs) >= 2 else 0
    return best, avg, std


if __name__ == "__main__":
    benchmark_filter = sys.argv[1] if len(sys.argv) > 1 else None
    benchmarks_folder = Path(benchmarks_folder)

    if not benchmarks_folder.exists():
        print(f"ERROR: benchmark folder not found at '{benchmarks_folder}'",
              file=sys.stderr)
        exit(1)

    headers = ["NAME", "BEST", "MEAN", "STD"]
    data = [[], [], [], []]

    def add_data(name, best, avg, std):
        data[0].append(name)
        data[1].append(best)
        data[2].append(avg)
        data[3].append(std)

    for root, dirs, files in os.walk(benchmarks_folder):
        if not files:
            continue
        br = False
        for f in sorted(files):
            p = Path(root, f)
            if benchmark_filter and benchmark_filter not in str(p):
                continue
            res = compute_benchmark_stats(p)
            if not res:
                continue
            best, avg, std = res
            add_data(str(p), f"{best:.1f}", f"{avg:.1f}", f"{std:.1f}")
            br = True
        if br:
            add_data("", "", "", "")
    tabulate(headers=headers, data=[d[:-1] for d in data])
