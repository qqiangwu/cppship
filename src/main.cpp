#include "cppship/cppship.h"

#include <iostream>

int main(int argc, char** argv) { return cppship::run(std::span<char*>(argv, argc)); }