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

#ifndef  _COLLECTOR_UTILS_INC_
#define  _COLLECTOR_UTILS_INC_

#include <exception>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

#include <boost/assert.hpp>


#include "date/tz.h"

using namespace std::string_literals;

namespace Collector
{
    class AssertionException : public std::invalid_argument
    {
    public:

        explicit AssertionException(const char* what);

        explicit AssertionException(const std::string& what);
    };

    class TimeOutException : public std::runtime_error
    {
    public:

        explicit TimeOutException(const char* what);

        explicit TimeOutException(const std::string& what);
    };

    using sview = std::string_view;

};	/* -----  end of namespace Collector  ----- */

namespace COL = Collector;

// some code to help with putting together error messages,
// we want to look for things which can be appended to a std::string
// let's try some concepts

template <typename T>
concept can_be_appended_to_string = requires(T t)
{
    std::declval<std::string>().append(t);
};

template <typename T>
concept has_string = requires(T t)
{
    t.string();
};

// suport for concatenation of string-like things
// let's use some concepts

//template<can_be_appended_to_string T>
//void append_to_string(std::string& s, const T& t)
//{
//    s +=t;
//}
//
//template<has_string T>
//void append_to_string(std::string& s, const T& t)
//{
//    s +=t.to_string();
//}
//
//template<typename T> requires(std::is_arithmetic_v<T>)
//void append_to_string(std::string& s, const T& t)
//{
//    s+= std::to_string(t);
//}
//
template<typename T>
void append_to_string(std::string& s, const T& t)
{
    // look for things which are 'string like' so we can just append them.

    if constexpr(can_be_appended_to_string<T>)
    {
        s.append(t);
    }
    else if constexpr(std::is_same_v<T, char>)
    {
        s += t;
    }
    else if constexpr(std::is_arithmetic_v<T>)
    {
        // it's a number so convert it.

        s.append(std::to_string(t));
    }
    else if constexpr(has_string<T>)
    {
        // it can look like a string

        s.append(t.string());
    }
    else
    {
        // we don't know what to do with it.

        throw std::invalid_argument("wrong type for 'catenate' function: "s + typeid(t).name());
    }
}

// now, a function to concatenate a bunch of string-like things.

template<typename... Ts>
std::string catenate(Ts&&... ts)
{
    // let's use fold a expression
    // (comma operator is cool...)

    std::string x;
    ( ... , append_to_string(x, std::forward<Ts>(ts)) );
    return x;
}

// function to split a string on a delimiter and return a vector of string-views

inline std::vector<COL::sview> split_string(COL::sview string_data, char delim)
{
    std::vector<COL::sview> results;
	for (auto it = 0; it != COL::sview::npos; ++it)
	{
		auto pos = string_data.find(delim, it);
        if (pos != COL::sview::npos)
        {
    		results.emplace_back(string_data.substr(it, pos - it));
        }
        else
        {
    		results.emplace_back(string_data.substr(it));
            break;
        }
		it = pos;
	}
    return results;
}

// function to split a string on a delimiter and return a vector of strings

inline std::vector<std::string> split_string_to_strings(COL::sview string_data, char delim)
{
    std::vector<std::string> results;
	for (auto it = 0; it != std::string::npos; ++it)
	{
		auto pos = string_data.find(delim, it);
        if (pos != std::string::npos)
        {
    		results.emplace_back(string_data.substr(it, pos - it));
        }
        else
        {
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

inline std::string LocalDateTimeAsString(std::chrono::system_clock::time_point a_date_time)
{
    auto t = date::make_zoned(date::current_zone(), a_date_time);
    std::string ts = date::format("%a, %b %d, %Y at %I:%M:%S %p %Z", t);
    return ts;
}

date::year_month_day StringToDateYMD(const std::string& input_format, std::string the_date);

#endif   /* ----- #IFNDEF COLLECTOR_UTILS_INC  ----- */
