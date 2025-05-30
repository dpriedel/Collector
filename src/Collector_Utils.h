/*
 * =====================================================================================
 *
 *       Filename:  Collector_Utils.h
 *
 *    Description:  some shared routines
 *
 *        Version:  1.0
 *        Created:  04/22/2019 02:14:42 PM
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  David P. Riedel (dpr), driedel@cox.net
 *        License:  GNU General Public License v3
 *        Company:
 *
 * =====================================================================================
 */

/* This file is part of Collector. */

/* Collector is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or */
/* (at your option) any later version. */

/* Collector is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* GNU General Public License for more details. */

/* You should have received a copy of the GNU General Public License */
/* along with Collector.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _COLLECTOR_UTILS_INC_
#define _COLLECTOR_UTILS_INC_

#include <exception>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

#include <boost/assert.hpp>
#include <date/tz.h>
#include <fmt/chrono.h>
#include <fmt/format.h>

using namespace std::string_literals;

namespace Collector {
class AssertionException : public std::invalid_argument {
public:
  explicit AssertionException(const char *what);

  explicit AssertionException(const std::string &what);
};

class TimeOutException : public std::runtime_error {
public:
  explicit TimeOutException(const char *what);

  explicit TimeOutException(const std::string &what);
};

using sview = std::string_view;

}; // namespace Collector

namespace COL = Collector;

// some code to help with putting together error messages,
// we want to look for things which can be appended to a std::string
// let's try some concepts

template <typename T>
concept can_be_appended_to_string =
    requires(T t) { std::declval<std::string>().append(t); };

template <typename T>
concept has_string = requires(T t) { t.string(); };

// custom fmtlib formatter for filesytem paths

template <>
struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext>
  auto format(const std::filesystem::path &p, FormatContext &ctx) const {
    return fmt::format_to(ctx.out(), "{}", p.string());
  }
};

// custom fmtlib formatter for date year_month_day

template <>
struct fmt::formatter<date::year_month_day> : fmt::formatter<std::string> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext>
  auto format(const date::year_month_day &d, FormatContext &ctx) const {
    return fmt::format_to(ctx.out(), "{:%Y-%m-%d}", d);
  }
};

template <typename... Ts> inline std::string catenate(const Ts &...ts) {
  constexpr auto N = sizeof...(Ts);

  // first, construct our format string
  // TODO: make constexpr

  std::string f_string;
  for (int i = 0; i < N; ++i) {
    f_string.append("{}");
  }

  return fmt::vformat(f_string, fmt::make_format_args(ts...));
}

// function to split a string on a delimiter and return a vector of string-views

inline std::vector<COL::sview> split_string(COL::sview string_data,
                                            char delim) {
  std::vector<COL::sview> results;
  for (auto it = 0; it != COL::sview::npos; ++it) {
    auto pos = string_data.find(delim, it);
    if (pos != COL::sview::npos) {
      results.emplace_back(string_data.substr(it, pos - it));
    } else {
      results.emplace_back(string_data.substr(it));
      break;
    }
    it = pos;
  }
  return results;
}

// function to split a string on a delimiter and return a vector of strings

inline std::vector<std::string> split_string_to_strings(COL::sview string_data,
                                                        char delim) {
  std::vector<std::string> results;
  for (auto it = 0; it != std::string::npos; ++it) {
    auto pos = string_data.find(delim, it);
    if (pos != std::string::npos) {
      results.emplace_back(string_data.substr(it, pos - it));
    } else {
      results.emplace_back(string_data.substr(it));
      break;
    }
    it = pos;
  }
  return results;
}

// replace boost gregorian with new date library.

// utility to convert a date::year_month_day to a string
// based on code from "The C++ Standard Library 2nd Edition"
// by Nicolai Josuttis p. 158

inline std::string
LocalDateTimeAsString(std::chrono::system_clock::time_point a_date_time) {
  auto t = date::make_zoned(date::current_zone(), a_date_time);
  std::string ts = date::format("%a, %b %d, %Y at %I:%M:%S %p %Z", t);
  return ts;
}

date::year_month_day StringToDateYMD(const std::string &input_format,
                                     const std::string &the_date);

#endif /* ----- #IFNDEF COLLECTOR_UTILS_INC  ----- */
