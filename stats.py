import os
import sys
from pathlib import Path
from statistics import stdev, mean

benchmarks_folder = Path("benchmarks")


def print_benchmark_stats(benchmark_file):
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
    std = stdev(costs)
    print(f"{benchmark_file}: avg_cost = {avg_cost:.2f} | std = {std:.2f}")


if __name__ == "__main__":
    if not benchmarks_folder.exists():
        print(f"Benchmark folder not found at {benchmarks_folder}",
              file=sys.stderr)
        exit(1)

    for root, dirs, files in os.walk(benchmarks_folder):
        for f in sorted(files):
            print_benchmark_stats(Path(root, f))