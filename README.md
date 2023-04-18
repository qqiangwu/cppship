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