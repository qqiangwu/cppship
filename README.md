# Build
```bash
mkdir build && cd build
conan install .. --profile debug
cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cd ..
cmake --build build -j8
cmake --build build -j8 --target install
```

# Usage
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