# Copyright 2020-2024 Kevin Backhouse.
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


add_library(DBusParseUtils SHARED)

target_sources(
        DBusParseUtils PRIVATE
        ../../include/DBusParseUtils/endianness.hpp
        ../../include/DBusParseUtils/error.hpp
        parse.cpp
        ../../include/DBusParseUtils/parse.hpp
        utils.cpp
        ../../include/DBusParseUtils/utils.hpp)

target_include_directories(DBusParseUtils PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/DBusParseUtils>
)

write_basic_package_version_file(DBusParseUtilsConfigVersion.cmake COMPATIBILITY ExactVersion)

install(TARGETS DBusParseUtils EXPORT DBusParseUtilsConfig
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT DBusParseUtilsConfig DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/DBusParseUtils")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/DBusParseUtilsConfigVersion.cmake
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/DBusParseUtils")
