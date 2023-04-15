[![Linux](https://github.com/qqiangwu/cppship/actions/workflows/ci-linux.yml/badge.svg?branch=main)](https://github.com/qqiangwu/cppship/actions/workflows/ci-linux.yml)
[![macOS](https://github.com/qqiangwu/cppship/actions/workflows/ci-macos.yml/badge.svg?branch=main)](https://github.com/qqiangwu/cppship/actions/workflows/ci-macos.yml)

# Intro
A cargo-like modern cpp build tools, aimed to combine all existing best practices, rather than re-inventing them from scratch.

+ dependency management: [conan2](https://conan.io/)
+ build: [cmake](https://cmake.org/)
+ tests: google test
+ benches: google bench

# Demo
![demo](https://user-images.githubusercontent.com/2892107/232225706-a5f8ae7f-b8c3-49bd-8f74-2bf2be79740b.gif)

# Build
```bash
mkdir build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build build -j8
cmake --build build -j8 --target install
```

# Usage
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
cppship fmt # dry-run and warn
cppship fmt -f # fix it

# format all files
cppship fmt -a
cppship fmt -a -f
```

## lint
We will use `clang-tidy` to lint your code

```bash
# by default, only lint files changed since last commit
cppship lint
cppship lint -a # lint all
```