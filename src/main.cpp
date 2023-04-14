#include "cppship/cppship.h"

#include <iostream>

int main(int argc, const char** argv) { return cppship::run(std::span<const char*>(argv, argc)); }