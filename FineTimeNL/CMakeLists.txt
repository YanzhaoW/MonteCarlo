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

add_executable(main main.cxx)
target_link_libraries(main PUBLIC ROOT::Core ROOT::MathCore ROOT::MathMore
                                  ROOT::Hist ROOT::RIO ROOT::Gpad fmt::fmt range-v3::range-v3)
