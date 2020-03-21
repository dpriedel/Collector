// =====================================================================================
//
//       Filename:  PathNameGenerator.h
//
//    Description:  Header for class which knows how to generate SEC index file path names
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


#include <filesystem>
#include <iterator>
#include <string>

//#include <boost/date_time/gregorian/gregorian.hpp>

//namespace bg = boost::gregorian;
namespace fs = std::filesystem;

#include "Collector_Utils.h"

// NOTE: this revised iterator approach with range support now
// still implements the closed interval type iterators used previously.

/*
 * =====================================================================================
 *        Class:  QuarterlyIterator
 *  Description:  really a generator for dates in a range.
 * =====================================================================================
 */
class QuarterlyIterator: public std::iterator<
                         std::forward_iterator_tag,     // iterator_category
                         date::year_month_day           // value_type
                         >
{
    public:
        /* ====================  LIFECYCLE     ======================================= */
        
        QuarterlyIterator () = default;                        /* constructor */
        explicit QuarterlyIterator(const date::year_month_day& start_date);

        QuarterlyIterator(const QuarterlyIterator& rhs) = default;
        QuarterlyIterator(QuarterlyIterator&& rhs) = default;

        ~QuarterlyIterator() = default;

        /* ====================  ACCESSORS     ======================================= */

        const date::year_month_day& operator*() const { return working_date_; }

        /* ====================  MUTATORS      ======================================= */

        QuarterlyIterator& operator++();

        /* ====================  OPERATORS     ======================================= */

        QuarterlyIterator& operator=(const QuarterlyIterator& rhs) = default;
        QuarterlyIterator& operator=(QuarterlyIterator&& rhs) = default;

        bool operator!=(const QuarterlyIterator& other) const { return working_date_ != other.working_date_; }
        bool operator<=(const QuarterlyIterator& other) const { return working_date_ <= other.working_date_; }

    protected:
        /* ====================  METHODS       ======================================= */

        /* ====================  DATA MEMBERS  ======================================= */

    private:
        /* ====================  METHODS       ======================================= */

        /* ====================  DATA MEMBERS  ======================================= */

        inline static date::months a_quarter{3};

        date::year_month_day start_date_;
        date::year_month_day working_date_;
        date::year start_year_;
        date::year working_year_;
        date::month start_month_;
        date::month working_month_;

}; /* -----  end of class QuarterlyIterator  ----- */

inline bool operator < (const QuarterlyIterator& lhs, const QuarterlyIterator& rhs)
{
	return *lhs < *rhs;
}

// we want to have a half open range of at least 1 quarter
// so internally, we add a quarter to the end date
// so the standard library algorithms will work as expected.
// This means, the supplied dates will BOTH be included in the range.

/*
 * =====================================================================================
 *        Class:  DateRange
 *  Description:  provide a half-open interval at least 1 quarter.
 * =====================================================================================
 */
class DateRange
{
    public:
        /* ====================  LIFECYCLE     ======================================= */
	
        DateRange(const date::year_month_day& start_date, const date::year_month_day& end_date);

        /* ====================  ACCESSORS     ======================================= */

        [[nodiscard]] QuarterlyIterator begin() const { return QuarterlyIterator{start_date_}; }
        [[nodiscard]] QuarterlyIterator end() const { return QuarterlyIterator{end_date_}; }
        /* ====================  MUTATORS      ======================================= */

        /* ====================  OPERATORS     ======================================= */

    protected:
        /* ====================  METHODS       ======================================= */

        /* ====================  DATA MEMBERS  ======================================= */

    private:
        /* ====================  METHODS       ======================================= */

        /* ====================  DATA MEMBERS  ======================================= */

        date::year_month_day start_date_;
        date::year_month_day end_date_;

}; /* -----  end of class DateRange  ----- */


fs::path GeneratePath(const fs::path& prefix, const date::year_month_day& quarter_begin);


#endif /* PATHNAMEGENERATOR_H_ */
