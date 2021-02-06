import subprocess
import sys
from pathlib import Path

"""
Start a race.
Run the solver for unlimited time in order 
to find the best possible solution.

Usage:
    python race.py INPUT OUTPUT [LOG]

e.g.
    python race.py datasets/comp02.ctt results/races/comp02.ctt.sol 
    python race.py datasets/comp02.ctt results/races/comp02.ctt.sol results/races/comp02.ctt.sol.log
"""

executable = Path("build/itc2007-cct")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"ERROR: usage is: python race.py INPUT OUTPUT",
              file=sys.stderr)
        exit(1)

    if not executable.exists():
        print(f"ERROR: '{executable.absolute()}' not found: please build itc2007-cct",
              file=sys.stderr)
        exit(1)

    dataset = Path(sys.argv[1])
    output = Path(sys.argv[2])
    output_log = Path(sys.argv[3]) if len(sys.argv) >= 4 else ""

    if not dataset.exists():
        print(f"ERROR: '{dataset.absolute()}' not found")
        exit(1)

    print("=============== RACE ================")
    print(f"Input:          {dataset}")
    print(f"Output:         {output}")
    print(f"Log:            {output_log}")
    print("=====================================")

    subprocess.run([str(executable), str(dataset), str(output), "-qr", f"-b{output_log}"])
