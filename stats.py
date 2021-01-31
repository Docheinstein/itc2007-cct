import os
import sys
from pathlib import Path
from statistics import stdev, mean

benchmarks_folder = Path("benchmarks")


def print_benchmark_stats(benchmark_file, width):
    if not benchmark_file.exists():
        return
    costs = []
    with benchmark_file.open() as f:
        for line in f:
            if line.startswith("#"):
                continue
            fields = line.split()
            if not fields:
                continue
            costs.append(int(fields[len(fields) - 1]))
    if not costs:
        return
    avg_cost = mean(costs)
    std = stdev(costs) if len(costs) >= 2 else 0
    avg_cost_s = f"{avg_cost:.2f}".ljust(10)
    std_s = f"{std:.2f}".ljust(10)
    print(f"{str(benchmark_file).ljust(width)}   {avg_cost_s} {std_s}")


if __name__ == "__main__":
    if not benchmarks_folder.exists():
        print(f"Benchmark folder not found at {benchmarks_folder}",
              file=sys.stderr)
        exit(1)

    longest = ("", 0)
    for root, dirs, files in os.walk(benchmarks_folder):
        for f in files:
            if len(f) > longest[1]:
                longest = (f, len(str(Path(root, f))))
    longest = longest[1]
    if not longest:
        exit()

    print(longest * " " + "    MEAN   |   STD")
    print(longest * "-" + "--------------------")
    for root, dirs, files in os.walk(benchmarks_folder):
        for f in sorted(files):
            print_benchmark_stats(Path(root, f), longest)