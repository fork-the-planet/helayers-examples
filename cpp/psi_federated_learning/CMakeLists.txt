#
# MIT License
#
# Copyright (c) 2020 International Business Machines
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.10)

project(psi_federated_learning VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-Werror -fopenmp -Wfatal-errors")

find_package(OpenFHE QUIET)
find_package(SEAL 3.6.6 EXACT REQUIRED)
find_package(Boost 1.72.0 EXACT REQUIRED COMPONENTS filesystem)
find_package(OpenSSL REQUIRED)
include_directories(
        ${OpenFHE_INCLUDE}
        ${OpenFHE_INCLUDE}/third-party/include
        ${OpenFHE_INCLUDE}/pke
        ${OpenFHE_INCLUDE}/binfhe
        ${OpenFHE_INCLUDE}/core)

add_executable(psi_federated_learning psi_federated_learning.cpp)
target_link_libraries(psi_federated_learning helayers_openfhe_ext helayers ${OpenFHE_LIBRARIES} SEAL::seal Boost::headers Boost::filesystem OpenSSL::Crypto)

add_executable(psi_multiple_parties psi_multiple_parties.cpp)
target_link_libraries(psi_multiple_parties helayers_openfhe_ext helayers ${OpenFHE_LIBRARIES} SEAL::seal Boost::headers Boost::filesystem OpenSSL::Crypto)
