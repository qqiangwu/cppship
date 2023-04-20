[![Linux](https://github.com/qqiangwu/cppship/actions/workflows/ci-linux.yml/badge.svg?branch=main)](https://github.com/qqiangwu/cppship/actions/workflows/ci-linux.yml)
[![macOS](https://github.com/qqiangwu/cppship/actions/workflows/ci-macos.yml/badge.svg?branch=main)](https://github.com/qqiangwu/cppship/actions/workflows/ci-macos.yml)

# Intro
A cargo-like modern cpp build tools, aimed to combine all existing best practices, rather than re-inventing them from scratch.

+ dependency management: [conan2](https://conan.io/)
+ build: [cmake](https://cmake.org/)
+ tests: google test
+ benches: google bench

# Demo
![demo](https://user-images.githubusercontent.com/2892107/232242145-bcc4bb3f-21c0-41a6-919a-9b5f5b196246.gif)

# Install
## Predefined packages
### MacOS
```bash
$ brew install qqiangwu/tap/cppship
```

### Linux
Try [homebrew](https://brew.sh/), or build it yourself, see below.


## CMake
We can use cmake to build `cppship`

```bash
mkdir build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build -j8
cmake --build build -j8 --target install
```

## Cppship
We can also to use cppship to build `cppship`

```bash
# debug build
cppship build

# release build
cppship build -r

# test
cppship test

# install
cppship install
```

# Tutorial
Cppship will generate cmake projects based on your directory structure and cppship.toml.

See [examples](https://github.com/qqiangwu/cppship-examples)

## cppship.toml
```toml
[package]
# (required) your lib or binary name
name = "cppship"
# (required)
version = "0.0.1"
authors = []
# (optional) specifiy you cppstd
std = 20

[dependencies]
# conan dependencies
boost = { version = "1.81.0", components = ["headers"], options = { header_only = true } }
toml11 = "3.7.1"
fmt = "9.1.0"
range-v3 = "0.12.0"
bshoshany-thread-pool = "3.3.0"
ms-gsl = "4.0.0"
argparse = "2.9"
spdlog = "1.11.0"
```

## header-only lib
make sure your project have the following structure:

+ include: lib include dir
+ tests: test cpp files go here, optional, `lib` will be its dependency

## lib
make sure your project have the following structure:

+ include: lib include dir
+ lib: lib cpp files go here
+ tests: test cpp files go here, optional, `lib` will be its dependency

## binary
make sure your project have the following structure:

+ src: binary cpp files go here, currently only one binary is supported

## lib + binary
+ include: lib include dir
+ lib: lib cpp files go here
+ src: bin cpp files go here, `lib` will be its dependency
+ tests: test cpp files go here, optional, `lib` will be its dependency

## multiple binaries
+ src/main.cpp: primary binary named with your project name
+ src/bin/xxx.cpp: binary xxx
+ src/bin/yyy.cpp: binary yyy

## inner tests
+ lib/a_test.cpp: will be viewed as a test
+ lib/sub/b_test.cpp: will be viewed as a test

# Usage
## Init
```bash
# init a binary
cppship init demo
cd demo
cppship run
cppship test

# init a library
cppship init demolib --lib
cd demolib
cppship build
```

## build
```bash
# debug build
cppship build

# release build
cppship build -r

# dry-run, config cmake and generate compile_commands.json under build
cppship build -d
```

## run
```bash
cppship run
cppship run -- a b c extra_args
```

## test
```
cppship test
```

## install
```bash
cppship install
```

## clean
```bash
cppship clean
```

## format
We will use `clang-format` to format our code

```bash
# by default, only format files changed since last commit
# dry-run and warn
cppship fmt
# only fmt git cached files
cppship fmt --cached
# fix it
cppship fmt -f

# format all files
cppship fmt -a
cppship fmt -a -f
```

## lint
We will use `clang-tidy` to lint your code

```bash
# by default, only lint files changed since last commit
cppship lint
# lint git cached files
cppship lint --cached
# lint all
cppship lint -a
```