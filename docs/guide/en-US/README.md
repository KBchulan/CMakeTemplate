# CMake Template

This project is a CMake-powered template that takes care of the groundwork so you can stay focused on product development.

### Environment Requirements

Besides the usual toolchain, please install GoogleTest, cppcheck, and Google Benchmark:

```bash
sudo pacman -S gtest cppcheck benchmark
```

If you really do not want to install them, the CMake configuration can clone the dependencies during the build. Be aware that this makes configuration noticeably slower, so installing them ahead of time is recommended.

### Quick Start

Basic out-of-source build steps:

```bash
mkdir build && cd build

# Configure and build
cmake ..
ninja # or cmake --build .
```

Executables will be generated under `build/bin`. Optional follow-up targets include installation and packaging:

```bash
ninja install  # Installs headers, executables, and shared libraries into build/install for easy deployment
cpack          # Generates sh/tgz archives for distribution
```

On top of the default configuration, several toggles are available. Enable only what you need.

#### Formatting

Use the provided script to format the codebase according to `.clang-format`:

```bash
./scripts/format.sh
```

#### Static Analysis

Three static-analysis options are integrated. AddressSanitizer and UndefinedBehaviorSanitizer are enabled automatically once the options are toggled on; `cppcheck` still needs to be invoked explicitly:

```bash
mkdir build && cd build

# Configure and build
cmake .. -DENABLE_CPPCHECK=ON
ninja # or cmake --build .

# Run cppcheck
cmake --build . --target cppcheck
```

#### Unit Tests

Enable unit tests to compile and run sources under `tests/`:

```bash
mkdir build && cd build

# Configure and build
cmake .. -DBUILD_TESTING=ON
ninja # or cmake --build .

# Execute the tests
ctest # or ctest --output-on-failure
```

#### Code Coverage

Coverage reports rely on `lcov` and `genhtml`. Install the prerequisites first:

```bash
sudo pacman -S lcov
```

Then enable coverage (remember that testing must be on as well):

```bash
# Enable testing and coverage
cmake .. -DBUILD_TESTING=ON -DENABLE_COVERAGE=ON
ninja

# Generate coverage data
ninja coverage
```

Open `build/coverage/index.html` in your browser to inspect the report.

#### Benchmarking

To build Google Benchmark targets, turn on the option and build as usual. The resulting binaries are placed in `build/bin`:

```bash
mkdir build && cd build

# Configure and build
cmake .. -DBUILD_BENCHMARK=ON
ninja # or cmake --build .
```

**Note:** Logger benchmarks inevitably print log output, which can interfere with the benchmark summary. Pipe the output through `grep` if you only care about the `BM_` lines:

```bash
./build/bin/bench_logger | grep "BM_"
```

#### Profiling

The `perf` + FlameGraph pipeline is wrapped into `scripts/profile.sh`. It works for regular programs and long-running services alike:

```bash
mkdir build && cd build

# Enable profiling support
cmake .. -DENABLE_PROFILING=ON
ninja # or cmake --build .

# Start profiling (follow the prompts)
./scripts/profile.sh
```

### Project Layout

An overview of what already ships with this template:

```bash
CMakeTemplate/
|-- .github/           # GitHub Actions workflows
|-- benchmark/         # Benchmark sources
|-- docs/              # Project documentation
|-- include/           # Public headers
|-- scripts/           # Helper scripts
|-- src/               # Main project sources
|-- tests/             # GoogleTest unit tests
|-- .clang-format      # Google-style formatting rules
|-- .clangd            # clangd configuration
|-- .gitignore         # Git ignore rules
|-- CMakeLists.txt     # Top-level build script managing targets and options
|-- Config.cmake.in    # Template for exported package configuration
|-- CTestConfig.cmake  # CDash configuration
|-- LICENSE            # Open-source license
--- README.md          # Project overview (Chinese)
```

Included components: a lock-free queue (100M+ ops in benchmarks), a logging module (260M+ ops), and an ID generator (50M+ ops).

Three utility scripts are also provided: `format.sh` (format the entire codebase), `profile.sh` (scan for executables and collect performance data), and `workflow.sh` (run all CI workflows locally).

Other artifacts are intentionally minimal—most names speak for themselves. When you add new functionality, refer to the examples in each directory for guidance.
