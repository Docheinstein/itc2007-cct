import subprocess
import sys
from pathlib import Path

"""
Start a race: run the solver for unlimited time in order to find the best.

Usage:
    python race.py INPUT OUTPUT
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

    if not dataset.exists():
        print(f"ERROR: '{dataset.absolute()}' not found")
        exit(1)

    print("=============== RACE ================")
    print(f"Input:          {dataset}")
    print(f"Estimated time: {output}")
    print("=====================================")

    subprocess.run([str(executable), str(dataset), str(output), "-r"])
