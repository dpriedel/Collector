// =====================================================================================
//
//       Filename:  PathNameGenerator.h
//
//    Description:  Implements class which knows how to generate SEC index file path names
//
//        Version:  1.0
//        Created:  06/29/2017 09:14:10 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:
//
// =====================================================================================

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

#include "PathNameGenerator.h"

DateRange::DateRange(const bg::date& start_date, const bg::date& end_date)
	: start_date_{start_date}, end_date_{end_date}

{
	auto working_date1 = bg::date(start_date.year(), (start_date.month() / 3 + (start_date.month() % 3 == 0 ? 0 : 1)) * 3 - 2, 1);
	auto working_date2 = bg::date(end_date.year(), (end_date.month() / 3 + (end_date.month() % 3 == 0 ? 0 : 1)) * 3 - 2, 1);

	end_date_ = *(++QuarterlyIterator{end_date});
}

QuarterlyIterator::QuarterlyIterator(const bg::date& start_date)
	: start_date_{start_date}, start_year_{start_date.year()}, working_year_{start_date.year()},
	    start_month_{start_date.month()}, working_month_{start_date.month()}

{
	// compute the first day of the quarter and use that as the base of calculations

	working_date_ = bg::date(start_date.year(), (start_date.month() / 3 + (start_date.month() % 3 == 0 ? 0 : 1)) * 3 - 2, 1);
	working_month_ = working_date_.month();
}

QuarterlyIterator& QuarterlyIterator::operator++()

{
	working_date_ += a_quarter;

	return *this;
}

fs::path GeneratePath(const fs::path& prefix, const bg::date& quarter_begin)

{
	auto working_year = quarter_begin.year();
	auto working_month = quarter_begin.month();

	auto SEC_path = prefix;
	SEC_path /= std::to_string(working_year);
	SEC_path /= "QTR" + std::to_string(working_month / 3 + (working_month % 3 == 0 ? 0 : 1));

	return SEC_path;
}
