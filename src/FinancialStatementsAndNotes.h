// =====================================================================================
//
//       Filename:  FinancialStatementsAndNotes.h
//
//    Description:  Module to manage download of SEC Financial Statement and
//    Notes data files.
//
//        Version:  1.0
//        Created:  05/04/2021 02:30:37 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

// =====================================================================================
//        Class:  FinancialStatementsAndNotes_itor
//  Description:  generator for financial statement and notes file and directory
//  names.
// =====================================================================================

#ifndef FINANCIALSTATEMENTSANDNOTES_INC
#define FINANCIALSTATEMENTSANDNOTES_INC

#include <chrono>
#include <filesystem>
#include <string>
#include <utility>

#include <date/date.h>

namespace fs = std::filesystem;

using namespace date::literals;

class FinancialStatementsAndNotes_gen {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type =
      std::pair<std::string, std::string>; // file name, directory name
  using difference_type = ptrdiff_t;
  using const_pointer = value_type const *;
  using const_reference = value_type const &;

public:
  // ====================  LIFECYCLE     =======================================

  FinancialStatementsAndNotes_gen() = default; // constructor
  FinancialStatementsAndNotes_gen(date::year_month_day start_date,
                                  date::year_month_day end_date); // constructor

  // ====================  ACCESSORS     =======================================

  std::string next_file_name() const;

  // ====================  MUTATORS      =======================================

  FinancialStatementsAndNotes_gen &operator++();
  FinancialStatementsAndNotes_gen operator++(int) {
    FinancialStatementsAndNotes_gen retval = *this;
    ++(*this);
    return retval;
  }

  // ====================  OPERATORS     =======================================

  const_reference operator*() const { return current_value_; }
  const_pointer operator->() const { return &current_value_; }

  bool operator==(const FinancialStatementsAndNotes_gen &rhs) const {
    return current_date_ == rhs.current_date_;
  }
  bool operator!=(const FinancialStatementsAndNotes_gen &rhs) const {
    return !(*this == rhs);
  }

protected:
  // ====================  METHODS       =======================================

  // ====================  DATA MEMBERS  =======================================

private:
  // ====================  METHODS       =======================================

  void format_current_value(void);

  // ====================  DATA MEMBERS  =======================================

  inline static constexpr date::months a_month{1};
  inline static constexpr date::years a_year{1};

  inline static constexpr date::year_month first_quarterly_ =
      2009_y / date::January;
  inline static constexpr date::year_month last_quarterly_ =
      2023_y / date::December;
  inline static constexpr date::year_month last_quarterly_qtr =
      2023_y / date::October; // using months for quarters
  inline static constexpr date::year_month first_monthly_ =
      2024_y / date::January;

  date::year_month start_date_;
  date::year_month end_date_;

  date::year_month current_date_;

  value_type current_value_;
  bool monthly_mode_ = false;

}; // -----  end of class FinancialStatementsAndNotes_gen  -----

// =====================================================================================
//        Class:  FinancialStatementsAndNotes
//  Description:  generator for financial statement and notes file names
// =====================================================================================
class FinancialStatementsAndNotes {
public:
  using const_iterator = FinancialStatementsAndNotes_gen;

public:
  // ====================  LIFECYCLE     =======================================

  FinancialStatementsAndNotes(date::year_month_day start_date,
                              date::year_month_day end_date); // constructor

  // ====================  ACCESSORS     =======================================

  const_iterator begin() const {
    return FinancialStatementsAndNotes_gen{start_date_, end_date_};
  }
  const_iterator end() const { return FinancialStatementsAndNotes_gen(); }

  // ====================  MUTATORS      =======================================

  void download_files(const std::string &server_name, const std::string &port,
                      const fs::path &download_destination, bool replace_files);

  // ====================  OPERATORS     =======================================

protected:
  // ====================  METHODS       =======================================

  // ====================  DATA MEMBERS  =======================================

private:
  // ====================  METHODS       =======================================

  // ====================  DATA MEMBERS  =======================================

  inline static const fs::path source_directory =
      "/files/dera/data/financial-statement-and-notes-data-sets";

  date::year_month_day start_date_;
  date::year_month_day end_date_;

}; // -----  end of class FinancialStatementsAndNotes  -----

#endif // ----- #ifndef FinancialStatementsAndNotes_INC  -----
