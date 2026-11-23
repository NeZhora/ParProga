import argparse
import csv
import math
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def read_rows(path):
    with path.open(encoding="utf-8-sig", newline="") as stream:
        for row in csv.DictReader(stream):
            value = float(row["seconds"])
            if not math.isfinite(value) or value <= 0:
                print(f"{path}: skipping invalid measurement {row}", file=sys.stderr)
                continue
            yield row, value


def save_plot(series, xlabel, ylabel, target, *, log_x=False, log_y=False):
    if not series:
        raise ValueError(f"No measurements for {target.name}")
    fig, ax = plt.subplots(figsize=(9, 5), layout="constrained")
    ticks = set()
    markers = ("o", "s", "^", "D", "v", "P", "X", "<", ">", "*", "h")
    for index, (label, values) in enumerate(sorted(series.items())):
        xs, ys = zip(*sorted(values.items()))
        ax.plot(xs, ys, marker=markers[index % len(markers)], label=label)
        ticks.update(xs)
    if log_x:
        ax.set_xscale("log", base=2)
    if log_y:
        ax.set_yscale("log")
    ax.set_xticks(sorted(ticks), labels=sorted(ticks))
    ax.set(xlabel=xlabel, ylabel=ylabel)
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.savefig(target, dpi=160)
    plt.close(fig)


def plot_matmul(path, out):
    times = defaultdict(dict)
    for row, value in read_rows(path):
        key = (row["method"], int(row["threads"]))
        n = int(row["n"])
        if n in times[key]:
            raise ValueError(f"Duplicate measurement for {key}, N={n}")
        times[key][n] = value

    time_series = {}
    speedups, efficiencies = {}, {}
    for (method, workers), values in times.items():
        label = f"{method}, p={workers}" if method.endswith("_omp") else method
        time_series[label] = values
        if method.endswith("_omp"):
            base = times.get((method.removesuffix("_omp"), 1), {})
            speedup = {n: base[n] / t for n, t in values.items() if n in base}
            if speedup:
                speedups[label] = speedup
                efficiencies[label] = {n: s / workers for n, s in speedup.items()}

    save_plot(time_series, "Размер матриц N", "Время, с", out / "matrix-time.png",
              log_x=True, log_y=True)
    save_plot(speedups, "Размер матриц N", "Ускорение", out / "matrix-speedup.png", log_x=True)
    save_plot(efficiencies, "Размер матриц N", "Эффективность",
              out / "matrix-efficiency.png", log_x=True)


def plot_loops(paths, out):
    speedups, efficiencies = {}, {}
    for path in paths:
        baselines, parallel = {}, defaultdict(dict)
        for row, value in read_rows(path):
            key = (row["variant"], int(row["rows"]), int(row["cols"]))
            workers = int(row["workers"])
            if row["backend"] == "seq":
                if key in baselines:
                    raise ValueError(f"Duplicate baseline for {key} in {path}")
                baselines[key] = value
            else:
                series = parallel[key + (row["backend"],)]
                if workers in series:
                    raise ValueError(f"Duplicate measurement for {key}, p={workers} in {path}")
                series[workers] = value

        for (variant, rows, cols, backend), values in sorted(parallel.items()):
            name = f"{variant} {backend}, {rows}x{cols}"
            if name in speedups:
                raise ValueError(f"Duplicate benchmark series: {name}")
            base = baselines[(variant, rows, cols)]
            speedup = {p: base / t for p, t in values.items()}
            speedups[name] = speedup
            efficiencies[name] = {p: s / p for p, s in speedup.items()}
            best = max(speedup, key=speedup.get)
            print(f"{name}: max S={speedup[best]:.3f} at p={best}, "
                  f"E={efficiencies[name][best]:.3f}")

    save_plot(speedups, "Число исполнителей p", "Ускорение", out / "loops-speedup.png")
    save_plot(efficiencies, "Число исполнителей p", "Эффективность",
              out / "loops-efficiency.png")


def main():
    parser = argparse.ArgumentParser(description="Plot performance from SLURM benchmark output")
    parser.add_argument("lab", choices=("matrix", "loops"))
    parser.add_argument("files", nargs="+", type=Path)
    parser.add_argument("--out", type=Path, default=Path("plots"))
    args = parser.parse_args()
    if args.lab == "matrix" and len(args.files) != 1:
        parser.error("matrix requires one benchmark file")

    try:
        args.out.mkdir(parents=True, exist_ok=True)
        if args.lab == "matrix":
            plot_matmul(args.files[0], args.out)
        else:
            plot_loops(args.files, args.out)
    except (OSError, KeyError, ValueError) as error:
        parser.exit(1, f"Cannot plot benchmark: {error}\n")


if __name__ == "__main__":
    main()
