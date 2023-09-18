# LF Verifier Benchmarks
This repository contains benchmarks for Lingua Franca (LF) verifiers that can
process `@property` annotations.

## Prerequisites

### Docker (experimental)
To quickly set up a working environment, we recommend building a docker image
using the Dockerfile provided.
```
docker build -t <tag> -f docker/Dockerfile .
docker run -it <tag>
```

### Manuel Setup
One can manually install the prerequisites by following these steps.

1. Install [Lingua Franca](https://www.lf-lang.org);
```
git clone https://github.com/lf-lang/lingua-franca.git
cd lingua-franca
./gradlew assemble
```

2. Install [Z3](https://github.com/Z3Prover/z3) and
   [Uclid5](https://github.com/uclid-org/uclid);

   a. Clone the Uclid5 repo:

   ```
   git clone https://github.com/uclid-org/uclid.git
   cd uclid
   git checkout 4fd5e566c5f87b052f92e9b23723a85e1c4d8c1c
   ```

   b. Make sure prerequisites are met.
   Please find the instructions for Linux [here](https://github.com/uclid-org/uclid#installation-of-prerequisites-on-linux), and the instructions for macOS [here](https://github.com/uclid-org/uclid#installation-of-prerequisites-on-mac).

   c. Install Z3 using a utility script in the Uclid5 repo.

   ```
   ./get-z3-linux.sh
   ```

   There is a corresponding utility script for macOS.

   ```
   ./get-z3-macos.sh
   ```

   d. Run the setup scripts.

   ```
   ./setup-z3-linux.sh
   ```

   There is a corresponding version for macOS.

   ```
   ./setup-z3-macos.sh
   ```

   e. Build Uclid5.

   ```
   sbt update clean compile
   sbt universal:packageBin
   cd target/universal/
   unzip uclid-0.9.5.zip
   ```

   f. Then add `<path-to-uclid>/target/universal/uclid-0.9.5/bin/` to PATH.

## Getting Started

### Checking a single program
All the benchmark programs are under the `benchmarks/src/` directory.
```
cd lf-verifier-benchmarks/benchmarks/src
```
To verify the properties of a program, simply run `lfc`.
```
lfc --verify <file.lf>
```

### Checking a set of programs
To check all programs under a specific directory (e.g. `benchmark/`), use the
`run-benchmark` script.
```
cd lf-verifier-benchmarks/
./scripts/run-benchmarks benchmarks
```
The timing results will be located under a `results/` directory.

### Ploting
To generate a stem plot for the timing results.
1. `cd lf-verifier-benchmarks/plotting`
1. Open `3d_stem_plot.py` and update the timing data (x = LF LOC, y = CT, z =
   Time (seconds)).
2. `python3 3d_stem_plot.py`
