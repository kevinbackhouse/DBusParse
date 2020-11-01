# Copyright 2020 Kevin Backhouse.
#
# This file is part of DBusParse.
#
# DBusParse is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# DBusParse is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with DBusParse.  If not, see <https://www.gnu.org/licenses/>.


cmake_minimum_required(VERSION 3.10)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

enable_testing()

# set the project name
project(DBusParse VERSION 1.0.0 DESCRIPTION "C++ Library for parsing D-Bus messages")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(USE_SANITIZERS "Enable ASAN and UBSAN" OFF)

add_compile_options(-Wall -Wextra -pedantic -Werror)

if (USE_SANITIZERS)
  set(SANITIZER_FLAGS "-fsanitize=address,undefined")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tests)
