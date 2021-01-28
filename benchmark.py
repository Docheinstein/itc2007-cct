import subprocess
import sys
from datetime import datetime
from pathlib import Path

# ----------- PARAMETERS ------------

executable = Path("build/itc2007-cct")
runs_per_dataset = 10
seconds_per_run = 168


def benchmark_arg(name, method):
    return f"datasets/{name}.ctt", f"configs/{method}.conf", f"benchmarks/{method}/{name}.out"


benchmark_args = [
    # benchmark_arg("comp01", "ls"),
    # benchmark_arg("comp02", "ls"),
    # benchmark_arg("comp03", "ls"),
    # benchmark_arg("comp04", "ls"),
    benchmark_arg("comp05", "ls"),
    # benchmark_arg("comp06", "ls"),
    # benchmark_arg("comp07", "ls")
]

# -----------------------------------

if not executable.exists():
    print(f"'{executable.absolute()}' not found: please build itc2007-cct",
          file=sys.stderr)
    exit(1)

print("=============== BENCHMARK ===============")
print(f"# Benchmarks:        {len(benchmark_args)}")
print(f"Runs per benchmark:  {runs_per_dataset}")
print(f"Seconds per run:     {seconds_per_run}")
print(f"Estimated time:      {int(len(benchmark_args) * runs_per_dataset * seconds_per_run / 60)} minutes")
print("=========================================")

for b_i, bench_arg in enumerate(benchmark_args):
    dataset, config, output = bench_arg

    dataset_path = Path(dataset)
    config_path = Path(config)
    output_path = Path(output)

    if not dataset_path.exists():
        print(f"WARN: skipping benchmark, missing dataset at path '{dataset_path}'")
        continue

    if not config_path.exists():
        print(f"WARN: skipping benchmark, missing config at path '{config_path}'")
        continue

    print(f"[{b_i + 1}/{len(benchmark_args)}] {dataset} -c {config}")

    output_folder = output_path.parent
    output_folder.mkdir(parents=True, exist_ok=True)

    with output_path.open("w") as out:
        out.write("# ----------------------------------------------------------\n")
        out.write("# ----------------------- BENCHMARK ------------------------\n")
        out.write("# ----------------------------------------------------------\n")
        out.write(f"# Dataset:          {dataset}\n")
        out.write(f"# Datetime:         {datetime.now().strftime('%H:%M:%S %m/%d/%Y')}\n")
        out.write(f"# Seconds per run:  {seconds_per_run}\n")
        out.write("# ----------------------------------------------------------\n")
        with config_path.open() as cfg:
            for line in cfg.readlines():
                line = line.strip()
                if line:
                    print(f"# {line}")
                    out.write(f"# {line}\n")
        out.write("# ---------------------------------------------------------\n")
        out.write("# seed  feasible  cost_rc  cost_mwd  cost_cc  cost_rs  cost\n")
        out.write("# ---------------------------------------------------------\n")

    for i in range(runs_per_dataset):
        print(f"  {i + 1}/{runs_per_dataset}... ", end="", flush=True)
        subprocess.run([
            str(executable),
            dataset,
            output,
            "-t", str(seconds_per_run),
            "-c", config,
            "-b"])

    print("--------------------------------------")
