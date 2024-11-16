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


install(DIRECTORY DBusParseUtils/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/DBusParseUtils)
install(DIRECTORY DBusParse/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/DBusParse)
