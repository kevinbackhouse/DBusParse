// Copyright 2020 Kevin Backhouse.
//
// This file is part of DBusParse.
//
// DBusParse is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DBusParse is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DBusParse.  If not, see <https://www.gnu.org/licenses/>.


#pragma once

#include <string>

// Exception class. Caught in main().
class Error : public std::exception {
  std::string msg_;

public:
  Error() = delete;  // No default constructor.
  explicit Error(const char* msg) : msg_(msg) {}
  explicit Error(std::string&& msg) : msg_(std::move(msg)) {}

  const char* what() const noexcept override {
    return msg_.c_str();
  }
};

// Exception class for system errors that include an errno. Caught in
// main().
class ErrorWithErrno : public Error {
  const int err_;

public:
  ErrorWithErrno() = delete;  // No default constructor.
  explicit ErrorWithErrno(const char* msg) : Error(msg), err_(errno) {}
  explicit ErrorWithErrno(std::string&& msg) : Error(std::move(msg)), err_(errno) {}

  int getErrno() const { return err_; }
};
