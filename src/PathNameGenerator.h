// =====================================================================================
//
//       Filename:  PathNameGenerator.h
//
//    Description:  Header for class which knows how to generate SEC index file
//    path names
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

#include <chrono>
#include <filesystem>
#include <iterator>

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
class QuarterlyIterator {
public:
  // Type aliases required for a custom iterator
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::chrono::year_month_day;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type *;
  using reference = value_type &;

  /* ====================  LIFECYCLE     =======================================
   */

  QuarterlyIterator() = default; /* constructor */
  explicit QuarterlyIterator(std::chrono::year_month_day start_date);

  QuarterlyIterator(const QuarterlyIterator &rhs) = default;
  QuarterlyIterator(QuarterlyIterator &&rhs) = default;

  ~QuarterlyIterator() = default;

  /* ====================  ACCESSORS     =======================================
   */

  std::chrono::year_month_day operator*() const { return working_date_; }

  /* ====================  MUTATORS      =======================================
   */

  QuarterlyIterator &operator++();

  /* ====================  OPERATORS     =======================================
   */

  QuarterlyIterator &operator=(const QuarterlyIterator &rhs) = default;
  QuarterlyIterator &operator=(QuarterlyIterator &&rhs) = default;

  bool operator!=(const QuarterlyIterator &other) const {
    return working_date_ != other.working_date_;
  }
  bool operator<=(const QuarterlyIterator &other) const {
    return working_date_ <= other.working_date_;
  }

protected:
  /* ====================  METHODS       =======================================
   */

  /* ====================  DATA MEMBERS  =======================================
   */

private:
  /* ====================  METHODS       =======================================
   */

  /* ====================  DATA MEMBERS  =======================================
   */

  inline static constexpr std::chrono::months a_quarter{3};

  std::chrono::year_month_day start_date_;
  std::chrono::year_month_day working_date_;
  std::chrono::year start_year_;
  std::chrono::year working_year_;
  std::chrono::month start_month_;
  std::chrono::month working_month_;

}; /* -----  end of class QuarterlyIterator  ----- */

inline bool operator<(const QuarterlyIterator &lhs,
                      const QuarterlyIterator &rhs) {
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
class DateRange {
public:
  /* ====================  LIFECYCLE     =======================================
   */

  DateRange(std::chrono::year_month_day start_date,
            std::chrono::year_month_day end_date);

  /* ====================  ACCESSORS     =======================================
   */

  [[nodiscard]] QuarterlyIterator begin() const {
    return QuarterlyIterator{start_date_};
  }
  [[nodiscard]] QuarterlyIterator end() const {
    return QuarterlyIterator{end_date_};
  }
  /* ====================  MUTATORS      =======================================
   */

  /* ====================  OPERATORS     =======================================
   */

protected:
  /* ====================  METHODS       =======================================
   */

  /* ====================  DATA MEMBERS  =======================================
   */

private:
  /* ====================  METHODS       =======================================
   */

  /* ====================  DATA MEMBERS  =======================================
   */

  std::chrono::year_month_day start_date_;
  std::chrono::year_month_day end_date_;

}; /* -----  end of class DateRange  ----- */

fs::path GeneratePath(const fs::path &prefix,
                      std::chrono::year_month_day quarter_begin);

#endif /* PATHNAMEGENERATOR_H_ */
