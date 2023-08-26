cmake_minimum_required(VERSION 3.24)
project(FinetimeMC VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS -pthread)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # used by clang-tidy
set(mathmore ON)

find_package(ROOT CONFIG REQUIRED)
find_package(range-v3 REQUIRED)
find_package(fmt REQUIRED)
find_package(cxxopts REQUIRED)

add_library(ROOTlib INTERFACE)
target_link_libraries(ROOTlib INTERFACE ROOT::Core ROOT::MathCore ROOT::MathMore
                                  ROOT::Hist ROOT::RIO ROOT::Gpad)

add_subdirectory(src)
