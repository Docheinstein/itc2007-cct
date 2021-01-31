import subprocess
import sys
from datetime import datetime
from pathlib import Path

# ----------- PARAMETERS ------------

executable = Path("build/itc2007-cct")

LS_OPTIONS = ["solver.methods=ls",
              "solver.multistart=true"]

HC_OPTIONS = ["solver.methods=hc",
              "solver.multistart=true",
              "hc.max_idle=80000"]

TS_OPTIONS = ["solver.methods=ts",
              "ts.max_idle=-1",
              "ts.tabu_tenure=80",
              "ts.frequency_penalty_coeff=0"]

benchmarks = [
    ("comp01-ls", "datasets/comp01.ctt", "benchmarks/168/ls/comp01.out", LS_OPTIONS, 168, 10),
    ("comp02-ls", "datasets/comp02.ctt", "benchmarks/168/ls/comp02.out", LS_OPTIONS, 168, 10),
    ("comp03-ls", "datasets/comp03.ctt", "benchmarks/168/ls/comp03.out", LS_OPTIONS, 168, 10),
    ("comp04-ls", "datasets/comp04.ctt", "benchmarks/168/ls/comp04.out", LS_OPTIONS, 168, 10),
    ("comp05-ls", "datasets/comp05.ctt", "benchmarks/168/ls/comp05.out", LS_OPTIONS, 168, 10),
    ("comp06-ls", "datasets/comp06.ctt", "benchmarks/168/ls/comp06.out", LS_OPTIONS, 168, 10),
    ("comp07-ls", "datasets/comp07.ctt", "benchmarks/168/ls/comp07.out", LS_OPTIONS, 168, 10),

    ("comp01-hc", "datasets/comp01.ctt", "benchmarks/168/hc/comp01.out", HC_OPTIONS, 168, 10),
    ("comp02-hc", "datasets/comp02.ctt", "benchmarks/168/hc/comp02.out", HC_OPTIONS, 168, 10),
    ("comp03-hc", "datasets/comp03.ctt", "benchmarks/168/hc/comp03.out", HC_OPTIONS, 168, 10),
    ("comp04-hc", "datasets/comp04.ctt", "benchmarks/168/hc/comp04.out", HC_OPTIONS, 168, 10),
    ("comp05-hc", "datasets/comp05.ctt", "benchmarks/168/hc/comp05.out", HC_OPTIONS, 168, 10),
    ("comp06-hc", "datasets/comp06.ctt", "benchmarks/168/hc/comp06.out", HC_OPTIONS, 168, 10),
    ("comp07-hc", "datasets/comp07.ctt", "benchmarks/168/hc/comp07.out", HC_OPTIONS, 168, 10),

    ("comp01-ts", "datasets/comp01.ctt", "benchmarks/168/ts/comp01.out", TS_OPTIONS, 168, 10),
    ("comp02-ts", "datasets/comp02.ctt", "benchmarks/168/ts/comp02.out", TS_OPTIONS, 168, 10),
    ("comp03-ts", "datasets/comp03.ctt", "benchmarks/168/ts/comp03.out", TS_OPTIONS, 168, 10),
    ("comp04-ts", "datasets/comp04.ctt", "benchmarks/168/ts/comp04.out", TS_OPTIONS, 168, 10),
    ("comp05-ts", "datasets/comp05.ctt", "benchmarks/168/ts/comp05.out", TS_OPTIONS, 168, 10),
    ("comp06-ts", "datasets/comp06.ctt", "benchmarks/168/ts/comp06.out", TS_OPTIONS, 168, 10),
    ("comp07-ts", "datasets/comp07.ctt", "benchmarks/168/ts/comp07.out", TS_OPTIONS, 168, 10),
]

# -----------------------------------

if __name__ == "__main__":
    bench_filter = sys.argv[1:] if len(sys.argv) > 1 else None

    if not executable.exists():
        print(f"'{executable.absolute()}' not found: please build itc2007-cct",
              file=sys.stderr)
        exit(1)

    estimated_seconds = 0
    filtered_benchmarks = []
    for bench in benchmarks:
        bench_name = bench[0]
        if bench_filter and bench_name not in bench_filter:
            continue
        filtered_benchmarks.append(bench)
        t, runs = bench[4], bench[5]
        estimated_seconds += runs * t

    print("=============== BENCHMARK ===============")
    print(f"# Datasets:          {len(filtered_benchmarks)}")
    print(f"Estimated time:      {int(estimated_seconds / 60)} minutes")
    print("=========================================")

    for b_i, bench in enumerate(filtered_benchmarks):
        bench_name, dataset, output, options, t, runs = bench

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
            out.write(f"# Seconds per run:  {t}\n")
            out.write(f"# Options:\n")
            for opt in options:
                out.write(f"#   {opt}\n")
            out.write("# ---------------------------------------------------------\n")
            out.write("# seed  fingerprint  feasible  cost_rc  cost_mwd  cost_cc  cost_rs  cost\n")
            out.write("# ---------------------------------------------------------\n")

        for i in range(runs):
            print(f"  {i + 1}/{runs}... ", end="", flush=True)
            subprocess.run([
                str(executable),
                dataset,
                output,
                "-t", str(t),
                "-b",
                *(" ".join([f"-o {opt}" for opt in options]).split(" "))]
            )

        print("--------------------------------------")
