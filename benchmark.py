import subprocess
import sys
import os
from datetime import datetime
from pathlib import Path

"""
Run benchmarks.

Usage:
    python benchmark.py [BENCHMARK_NAME][,BENCHMARK_NAME]...
    
If no BENCHMARK_NAME is given, run all the benchmarks.
"""

# ----------- PARAMETERS ------------

executable = Path("build/itc2007-cct")

LS_OPTIONS = ["solver.methods=ls",
              "solver.multistart=true"]

HC_OPTIONS = ["solver.methods=hc",
              "solver.multistart=true",
              "hc.max_idle=120000",
              "hc.max_idle_near_best_coeff=3",
              "hc.near_best_ratio=1.02"]

TS_OPTIONS = ["solver.methods=ts",
              "ts.max_idle=-1",
              "ts.tabu_tenure=120",
              "ts.frequency_penalty_coeff=0"]

SA_OPTIONS = [
    "solver.methods=sa",
    "sa.initial_temperature=1.4",
    "sa.cooling_rate=0.965",
    "sa.min_temperature=0.12",
    "sa.temperature_length_coeff=0.125",
    "sa.min_temperature_near_best_coeff=0.68",
    "sa.near_best_ratio=1.05",
    "sa.reheat_coeff=1.015",
]

SA2_OPTIONS = [
    "solver.methods=sa",
    "sa.initial_temperature=1.4",
    "sa.cooling_rate=0.97",
    "sa.min_temperature=0.08",
    "sa.temperature_length_coeff=50"
]

SA3_OPTIONS = [
    "solver.methods=sa",
    "sa.initial_temperature=1.4",
    "sa.cooling_rate=0.965",
    "sa.min_temperature=0.12",
    "sa.temperature_length_coeff=50",
    "sa.min_temperature_near_best_coeff=0.67",
    "sa.near_best_ratio=1.05",
]

SALS_OPTIONS = [
    "solver.methods=sa,ls",
    "sa.initial_temperature=1.4",
    "sa.cooling_rate=0.965",
    "sa.min_temperature=0.12",
    "sa.temperature_length_coeff=0.125",
    "sa.min_temperature_near_best_coeff=0.68",
    "sa.near_best_ratio=1.05",
    "sa.reheat_coeff=1.015",
    "ls.max_distance_from_best_ratio=1.02",
]

HCSA_OPTIONS = ["solver.methods=hc,sa",
                "solver.restore_best_after_cycles=50",
                "hc.max_idle=30000",
                "hc.max_idle_near_best_coeff=3",
                "sa.initial_temperature=1.4",
                "sa.cooling_rate=0.965",
                "sa.min_temperature=0.095",
                "sa.temperature_length_coeff=50"]

#   <benchmark_name>  <<input>  <output>  <options>  <seconds>  <runs>
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

    ("comp01-sa", "datasets/comp01.ctt", "benchmarks/168/sa/comp01.out", SA_OPTIONS, 168, 10),
    ("comp02-sa", "datasets/comp02.ctt", "benchmarks/168/sa/comp02.out", SA_OPTIONS, 168, 10),
    ("comp03-sa", "datasets/comp03.ctt", "benchmarks/168/sa/comp03.out", SA_OPTIONS, 168, 10),
    ("comp04-sa", "datasets/comp04.ctt", "benchmarks/168/sa/comp04.out", SA_OPTIONS, 168, 10),
    ("comp05-sa", "datasets/comp05.ctt", "benchmarks/168/sa/comp05.out", SA_OPTIONS, 168, 10),
    ("comp06-sa", "datasets/comp06.ctt", "benchmarks/168/sa/comp06.out", SA_OPTIONS, 168, 10),
    ("comp07-sa", "datasets/comp07.ctt", "benchmarks/168/sa/comp07.out", SA_OPTIONS, 168, 10),

    ("comp01-sa2", "datasets/comp01.ctt", "benchmarks/168/sa2/comp01.out", SA2_OPTIONS, 168, 10),
    ("comp02-sa2", "datasets/comp02.ctt", "benchmarks/168/sa2/comp02.out", SA2_OPTIONS, 168, 10),
    ("comp03-sa2", "datasets/comp03.ctt", "benchmarks/168/sa2/comp03.out", SA2_OPTIONS, 168, 10),
    ("comp04-sa2", "datasets/comp04.ctt", "benchmarks/168/sa2/comp04.out", SA2_OPTIONS, 168, 10),
    ("comp05-sa2", "datasets/comp05.ctt", "benchmarks/168/sa2/comp05.out", SA2_OPTIONS, 168, 10),
    ("comp06-sa2", "datasets/comp06.ctt", "benchmarks/168/sa2/comp06.out", SA2_OPTIONS, 168, 10),
    ("comp07-sa2", "datasets/comp07.ctt", "benchmarks/168/sa2/comp07.out", SA2_OPTIONS, 168, 10),

    ("comp01-sa3", "datasets/comp01.ctt", "benchmarks/168/sa3/comp01.out", SA3_OPTIONS, 168, 10),
    ("comp02-sa3", "datasets/comp02.ctt", "benchmarks/168/sa3/comp02.out", SA3_OPTIONS, 168, 10),
    ("comp03-sa3", "datasets/comp03.ctt", "benchmarks/168/sa3/comp03.out", SA3_OPTIONS, 168, 10),
    ("comp04-sa3", "datasets/comp04.ctt", "benchmarks/168/sa3/comp04.out", SA3_OPTIONS, 168, 10),
    ("comp05-sa3", "datasets/comp05.ctt", "benchmarks/168/sa3/comp05.out", SA3_OPTIONS, 168, 10),
    ("comp06-sa3", "datasets/comp06.ctt", "benchmarks/168/sa3/comp06.out", SA3_OPTIONS, 168, 10),
    ("comp07-sa3", "datasets/comp07.ctt", "benchmarks/168/sa3/comp07.out", SA3_OPTIONS, 168, 10),

    ("comp01-hcsa", "datasets/comp01.ctt", "benchmarks/168/hcsa/comp01.out", HCSA_OPTIONS, 168, 10),
    ("comp02-hcsa", "datasets/comp02.ctt", "benchmarks/168/hcsa/comp02.out", HCSA_OPTIONS, 168, 10),
    ("comp03-hcsa", "datasets/comp03.ctt", "benchmarks/168/hcsa/comp03.out", HCSA_OPTIONS, 168, 10),
    ("comp04-hcsa", "datasets/comp04.ctt", "benchmarks/168/hcsa/comp04.out", HCSA_OPTIONS, 168, 10),
    ("comp05-hcsa", "datasets/comp05.ctt", "benchmarks/168/hcsa/comp05.out", HCSA_OPTIONS, 168, 10),
    ("comp06-hcsa", "datasets/comp06.ctt", "benchmarks/168/hcsa/comp06.out", HCSA_OPTIONS, 168, 10),
    ("comp07-hcsa", "datasets/comp07.ctt", "benchmarks/168/hcsa/comp07.out", HCSA_OPTIONS, 168, 10),

    ("comp01-sals", "datasets/comp01.ctt", "benchmarks/168/sals/comp01.out", SALS_OPTIONS, 168, 10),
    ("comp02-sals", "datasets/comp02.ctt", "benchmarks/168/sals/comp02.out", SALS_OPTIONS, 168, 10),
    ("comp03-sals", "datasets/comp03.ctt", "benchmarks/168/sals/comp03.out", SALS_OPTIONS, 168, 10),
    ("comp04-sals", "datasets/comp04.ctt", "benchmarks/168/sals/comp04.out", SALS_OPTIONS, 168, 10),
    ("comp05-sals", "datasets/comp05.ctt", "benchmarks/168/sals/comp05.out", SALS_OPTIONS, 168, 10),
    ("comp06-sals", "datasets/comp06.ctt", "benchmarks/168/sals/comp06.out", SALS_OPTIONS, 168, 10),
    ("comp07-sals", "datasets/comp07.ctt", "benchmarks/168/sals/comp07.out", SALS_OPTIONS, 168, 10),
]

# -----------------------------------

if __name__ == "__main__":
    bench_filter = sys.argv[1:] if len(sys.argv) > 1 else None

    if not executable.exists():
        print(f"ERROR: '{executable.absolute()}' not found: please build itc2007-cct",
              file=sys.stderr)
        exit(1)

    estimated_seconds = 0
    filtered_benchmarks = []
    for bf in bench_filter:
        for b in benchmarks:
            b_name = b[0]
            if bf == b_name:
                filtered_benchmarks.append(b)
                t, runs = b[4], b[5]
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
            out.write(f"# Datetime:         {datetime.now().strftime('%H:%M:%S %d/%m/%Y')}\n")
            out.write(f"# Seconds per run:  {t}\n")
            out.write(f"# Options:\n")
            for opt in options:
                out.write(f"#   {opt}\n")
            out.write("# ---------------------------------------------------------\n")
            out.write("# seed  fingerprint  time  cycles  moves  feasible  cost_rc  cost_mwd  cost_cc  cost_rs  cost\n")
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
