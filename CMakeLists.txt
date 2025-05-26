cmake_minimum_required(VERSION 3.10)
project(PseudoFinancialModels)

set(CMAKE_CXX_STANDARD 20)
set(EIGEN3_INCLUDE_DIR "/usr/include/eigen3")

include_directories(${EIGEN3_INCLUDE_DIR})

add_executable(factor_model app/models/factor_model.cpp)
add_executable(heston_model app/models/heston_model.cpp)
