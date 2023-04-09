# Intro
A cargo-like modern cpp build tools, aimed to combine all existing best practices, rather than re-inventing them from scratch.

+ dependency management: [conan](https://conan.io/)
+ build: ninja
+ tests: google test
+ benches: google bench

# Build
```bash
mkdir build && cd build
conan install .. --profile debug
cmake .. -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cd ..
cmake --build build -j8
cmake --build build -j8 --target install
```

# Usage
## build
```bash
cppship build
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