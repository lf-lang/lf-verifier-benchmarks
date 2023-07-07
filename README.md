# LF Verifier Benchmarks
This repository contains benchmarks for Lingua Franca (LF) verifiers that can
process `@property` annotations.

## Prerequisites

### Manuel Setup
One can manually install the prerequisites by following these steps.

1. Install [Lingua Franca](https://www.lf-lang.org) with verifier;
```
git clone https://github.com/lf-lang/lingua-franca.git
git checkout verifier
./bin/build-lf-cli
```

2. Install [Z3](https://github.com/Z3Prover/z3) and
   [Uclid5](https://github.com/uclid-org/uclid);

Install Z3.

```
git clone https://github.com/uclid-org/uclid.git
cd uclid
git checkout 4fd5e566c5f87b052f92e9b23723a85e1c4d8c1c
./get-z3-linux.sh
```

Update the environment variables `PATH` and `LD_LIBRARY_PATH`. See
[here](https://github.com/uclid-org/uclid#installation-of-prerequisites-on-linux)
for more details.

```
export PATH="${PATH}:<path-to-uclid>/z3/bin"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:<path-to-uclid>/z3/bin"
```

Build Uclid5.

```
sbt update clean compile
sbt universal:packageBin
cd target/universal/
unzip uclid-0.9.5.zip
```
Then add `<path-to-uclid>/target/universal/uclid-0.9.5/bin/` to PATH.

### Docker (experimental)
To quickly set up a working environment, we recommend building a docker image
using the Dockerfile provided.
```
docker build -t <tag> -f docker/Dockerfile .
docker run -it <tag>
```
(FIXME: Need to allocate more memory to the docker container.)

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
