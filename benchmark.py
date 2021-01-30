import subprocess
import sys
from datetime import datetime
from pathlib import Path

# ----------- PARAMETERS ------------

executable = Path("build/itc2007-cct")
runs_per_dataset = 10
seconds_per_run = 168

LS_OPTIONS = ["solver.methods=ls", "solver.multistart=true"]
HC_OPTIONS = ["solver.methods=hc",
              "solver.multistart=true",
              "hc.max_idle=80000",
              "hc.intensification_threshold=1.1",
              "hc.intensification_coeff=1.5"]

benchmarks = [
    ("comp01-ls", "datasets/comp01.ctt", "benchmarks/ls/comp01.out", LS_OPTIONS),
    ("comp02-ls", "datasets/comp02.ctt", "benchmarks/ls/comp02.out", LS_OPTIONS),
    ("comp03-ls", "datasets/comp03.ctt", "benchmarks/ls/comp03.out", LS_OPTIONS),
    ("comp04-ls", "datasets/comp04.ctt", "benchmarks/ls/comp04.out", LS_OPTIONS),
    ("comp05-ls", "datasets/comp05.ctt", "benchmarks/ls/comp05.out", LS_OPTIONS),
    ("comp06-ls", "datasets/comp06.ctt", "benchmarks/ls/comp06.out", LS_OPTIONS),
    ("comp07-ls", "datasets/comp07.ctt", "benchmarks/ls/comp07.out", LS_OPTIONS),

    ("comp01-hc", "datasets/comp01.ctt", "benchmarks/hc/comp01.out", HC_OPTIONS),
    ("comp02-hc", "datasets/comp02.ctt", "benchmarks/hc/comp02.out", HC_OPTIONS),
    ("comp03-hc", "datasets/comp03.ctt", "benchmarks/hc/comp03.out", HC_OPTIONS),
    ("comp04-hc", "datasets/comp04.ctt", "benchmarks/hc/comp04.out", HC_OPTIONS),
    ("comp05-hc", "datasets/comp05.ctt", "benchmarks/hc/comp05.out", HC_OPTIONS),
    ("comp06-hc", "datasets/comp06.ctt", "benchmarks/hc/comp06.out", HC_OPTIONS),
    ("comp07-hc", "datasets/comp07.ctt", "benchmarks/hc/comp07.out", HC_OPTIONS),
]

# -----------------------------------

if __name__ == "__main__":
    bench_filter = sys.argv[1:] if len(sys.argv) > 1 else None

    if not executable.exists():
        print(f"'{executable.absolute()}' not found: please build itc2007-cct",
              file=sys.stderr)
        exit(1)

    filtered_benchmarks = []
    for bench in benchmarks:
        bench_name = bench[0]
        if bench_filter and bench_name not in bench_filter:
            continue
        filtered_benchmarks.append(bench)

    print("=============== BENCHMARK ===============")
    print(f"# Datasets:          {len(filtered_benchmarks)}")
    print(f"Runs per benchmark:  {runs_per_dataset}")
    print(f"Seconds per run:     {seconds_per_run}")
    print(f"Estimated time:      {int(len(filtered_benchmarks) * runs_per_dataset * seconds_per_run / 60)} minutes")
    print("=========================================")

    for b_i, bench in enumerate(filtered_benchmarks):
        bench_name, dataset, output, options = bench

        dataset_path = Path(dataset)
        output_path = Path(output)

        if not dataset_path.exists():
            print(f"WARN: skipping benchmark, missing dataset at path '{dataset_path}'")
            continue

        print(f"[{b_i + 1}/{len(filtered_benchmarks)}] {dataset} {options}")

        output_folder = output_path.parent
        output_folder.mkdir(parents=True, exist_ok=True)

        with output_path.open("w") as out:
            out.write("# ----------------------------------------------------------\n")
            out.write("# ----------------------- BENCHMARK ------------------------\n")
            out.write("# ----------------------------------------------------------\n")
            out.write(f"# Dataset:          {dataset}\n")
            out.write(f"# Datetime:         {datetime.now().strftime('%H:%M:%S %m/%d/%Y')}\n")
            out.write(f"# Seconds per run:  {seconds_per_run}\n")
            out.write(f"# Options:\n")
            for opt in options:
                out.write(f"#   {opt}\n")
            out.write("# ---------------------------------------------------------\n")
            out.write("# seed  fingerprint  feasible  cost_rc  cost_mwd  cost_cc  cost_rs  cost\n")
            out.write("# ---------------------------------------------------------\n")

        for i in range(runs_per_dataset):
            print(f"  {i + 1}/{runs_per_dataset}... ", end="", flush=True)
            subprocess.run([
                str(executable),
                dataset,
                output,
                "-t", str(seconds_per_run),
                "-b",
                *(" ".join([f"-o {opt}" for opt in options]).split(" "))]
            )

        print("--------------------------------------")
