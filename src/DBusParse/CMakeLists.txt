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


add_library(DBusParse SHARED)

target_sources(
        DBusParse PRIVATE
        ../../include/DBusParse/dbus.hpp
        dbus.cpp
        ../../include/DBusParse/dbus_auth.hpp
        dbus_auth.cpp
        dbus_parse.cpp
        ../../include/DBusParse/dbus_print.hpp
        dbus_print.cpp
        ../../include/DBusParse/dbus_random.hpp
        dbus_random.cpp
        ../../include/DBusParse/dbus_serialize.hpp
        dbus_serialize.cpp
        ../../include/DBusParse/dbus_utils.hpp
        dbus_utils.cpp)

target_include_directories(
        DBusParse PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/DBusParseUtils>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/DBusParse>)

write_basic_package_version_file(DBusParseConfigVersion.cmake COMPATIBILITY ExactVersion)

install(TARGETS DBusParse EXPORT DBusParseConfig
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT DBusParseConfig DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/DBusParse")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/DBusParseConfigVersion.cmake DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/DBusParse")
