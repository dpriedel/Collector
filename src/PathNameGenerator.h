// =====================================================================================
//
//       Filename:  PathNameGenerator.h
//
//    Description:  Header for class which knows how to generate EDGAR index file path names
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

#ifndef PATHNAMEGENERATOR_H_
#define PATHNAMEGENERATOR_H_

	/* This file is part of CollectEDGARData. */

	/* CollectEDGARData is free software: you can redistribute it and/or modify */
	/* it under the terms of the GNU General Public License as published by */
	/* the Free Software Foundation, either version 3 of the License, or */
	/* (at your option) any later version. */

	/* CollectEDGARData is distributed in the hope that it will be useful, */
	/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
	/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
	/* GNU General Public License for more details. */

	/* You should have received a copy of the GNU General Public License */
	/* along with CollectEDGARData.  If not, see <http://www.gnu.org/licenses/>. */


#include <string>
#include <experimental/filesystem>

#include <boost/date_time/gregorian/gregorian.hpp>

namespace bg = boost::gregorian;
namespace fs = std::experimental::filesystem;

// NOTE: this revised iterator approach with range support now
// still implements the closed interval type iterators used previously.

class QuarterlyIterator
{
public:

	explicit QuarterlyIterator(const bg::date& start_date);

	const bg::date& operator*() const { return working_date_; }
	QuarterlyIterator& operator++();
	bool operator!=(const QuarterlyIterator& other) const { return working_date_ <= other.working_date_; }

private:

	inline static bg::months a_quarter{3};
	bg::date start_date_;
	bg::date working_date_;
	bg::greg_year start_year_;
	bg::greg_year working_year_;
	bg::greg_month start_month_;
	bg::greg_month working_month_;

};

// some traits for our iterator class

namespace std
{
	template <>
	struct iterator_traits<QuarterlyIterator>{
	using iterator_category = std::forward_iterator_tag;
	using value_type = bg::date;
	};
}

class DateRange
{
public:

	explicit DateRange(const bg::date& start_date, const bg::date& end_date);

	QuarterlyIterator begin() const { return QuarterlyIterator{start_date_}; }
	QuarterlyIterator end() const { return QuarterlyIterator{end_date_}; }

private:

	bg::date start_date_;
	bg::date end_date_;

};

fs::path GeneratePath(const fs::path& prefix, const bg::date& quarter_begin);


#endif /* PATHNAMEGENERATOR_H_ */
