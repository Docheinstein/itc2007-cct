import subprocess
import sys
from datetime import datetime
from pathlib import Path

# ----------- PARAMETERS ------------
from random import randint, uniform

executable = Path("build/itc2007-cct")

TS_OPTIONS = ["solver.methods=sa",
              "solver.restore_best_after_cycles=30",
              "sa.max_idle=-1",
              "sa.min_temperature=0.10"]

while True:
    cr = uniform(0.95, 0.9995)
    it = uniform(1.3, 1.7)
    tl = uniform(1, 1.5)
    options = TS_OPTIONS + [f"sa.cooling_rate={cr}", f"sa.initial_temperature={it}", f"sa.temperature_length_coeff={tl}"]
    print(f"it = {it} | cr = {cr} | tl = {tl}")
    subprocess.run([
        str(executable),
        "datasets/comp03.ctt",
        "-t", "120",
        "-b",
        *(" ".join([f"-o {opt}" for opt in options]).split(" "))]
    )

