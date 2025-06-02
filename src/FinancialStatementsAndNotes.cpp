// =====================================================================================
//
//       Filename:  FinancialStatementsAndNotes.cpp
//
//    Description:  Module to manage download of SEC Financial Statement and
//    Notes data files.
//
//        Version:  1.0
//        Created:  05/04/2021 02:32:42 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================
//

//--------------------------------------------------------------------------------------
//       Class:  FinancialStatementsAndNotes_gen
//      Method:  FinancialStatementsAndNotes_gen
// Description:  constructor
//--------------------------------------------------------------------------------------

#include <iostream>
#include <system_error>

#include <boost/asio.hpp>
#include <boost/process.hpp>

namespace bp = boost::process;

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "Collector_Utils.h"
#include "FinancialStatementsAndNotes.h"
#include "HTTPS_Downloader.h"

FinancialStatementsAndNotes_gen::FinancialStatementsAndNotes_gen(
    date::year_month_day start_date, date::year_month_day end_date) {
  // when in quarterly mode, the 'month' value will actually be the 'quarter'
  // value between 1 and 4. maybe there's a better way to do this. Nonetheless,
  // it keeps the code conistent.

  date::year_month temp_s{start_date.year(), start_date.month()};
  if (temp_s > last_quarterly_) {
    monthly_mode_ = true;
    start_date_ = temp_s;
  } else {
    // convert to our faux quarterly value
    start_date_ = date::year_month{
        start_date.year(),
        date::month{
            start_date.month().operator unsigned int() / 3 +
            (start_date.month().operator unsigned int() % 3 == 0 ? 0 : 1)}};
  }

  date::year_month temp_e{end_date.year(), end_date.month()};
  if (temp_e > last_quarterly_) {
    end_date_ = temp_e;
  } else {
    // convert to our faux quarterly value
    end_date_ = date::year_month{
        end_date.year(),
        date::month{
            end_date.month().operator unsigned int() / 3 +
            (end_date.month().operator unsigned int() % 3 == 0 ? 0 : 1)}};
  }
  current_date_ = start_date_;

  format_current_value();

} // -----  end of method
  // FinancialStatementsAndNotes_gen::FinancialStatementsAndNotes_gen
  // (constructor)  -----

FinancialStatementsAndNotes_gen &FinancialStatementsAndNotes_gen::operator++() {
  // see if we're done

  if (current_date_ == date::year_month{}) {
    return *this;
  }

  if (!monthly_mode_) {
    // we need to do 'quarterly' arithmetic here.

    current_date_ += a_month;
    if (current_date_.month().operator unsigned int() > 4) {
      current_date_ = {current_date_.year() + a_year, date::January};
    }
    if (current_date_ > last_quarterly_qtr) {
      current_date_ = first_monthly_;
      monthly_mode_ = true;
    }
  } else {
    current_date_ += a_month;
  }

  if (current_date_ >= end_date_) {
    current_date_ = date::year_month();
    current_value_ = {"", ""};
    return *this;
  }

  format_current_value();

  return *this;
} // -----  end of method FinancialStatementsAndNotes_gen::operator++  -----

void FinancialStatementsAndNotes_gen::format_current_value() {
  if (!monthly_mode_) {
    current_value_.first =
        fmt::format("{}q{}_notes.zip", current_date_.year().operator int(),
                    current_date_.month().operator unsigned int());
    current_value_.second =
        fmt::format("{}_{}", current_date_.year().operator int(),
                    current_date_.month().operator unsigned int());
  } else {
    current_value_.first =
        fmt::format("{}_{:02}_notes.zip", current_date_.year().operator int(),
                    current_date_.month().operator unsigned int());
    current_value_.second =
        fmt::format("{}_{:02}", current_date_.year().operator int(),
                    current_date_.month().operator unsigned int());
  }
} // -----  end of method FinancialStatementsAndNotes_gen::format_current_value
  // -----

//--------------------------------------------------------------------------------------
//       Class:  FinancialStatementsAndNotes
//      Method:  FinancialStatementsAndNotes
// Description:  constructor
//--------------------------------------------------------------------------------------
FinancialStatementsAndNotes::FinancialStatementsAndNotes(
    date::year_month_day start_date, date::year_month_day end_date)
    : start_date_{start_date}, end_date_{end_date} {
} // -----  end of method
  // FinancialStatementsAndNotes::FinancialStatementsAndNotes
  // (constructor)  -----

// files/dera/data/financial-statement-and-notes-data-sets/
void FinancialStatementsAndNotes::download_files(
    const std::string &server_name, const std::string &port,
    const fs::path &download_destination_zips,
    const fs::path &download_destination_files, bool replace_files) {
  int downloaded_files_counter = 0;
  int skipped_files_counter = 0;
  int error_counter = 0;

  HTTPS_Downloader fin_statement_downloader{server_name, port};

  if (!fs::exists(download_destination_zips)) {
    fs::create_directories(download_destination_zips);
  }

  if (!fs::exists(download_destination_files)) {
    fs::create_directories(download_destination_files);
  }

  // since this class looks like a range (has 'begin' and 'end') we can do this.

  for (const auto &[file, directory] : *this) {
    fs::path source_file = source_directory / file;
    fs::path destination_zip_directory = download_destination_zips / directory;
    if (!fs::exists(destination_zip_directory)) {
      fs::create_directories(destination_zip_directory);
    }
    BOOST_ASSERT_MSG(
        fs::exists(destination_zip_directory),
        fmt::format("Unable to create/find destination_zip_directory: {}",
                    destination_zip_directory)
            .c_str());

    fs::path destination_file_directory =
        download_destination_files / directory;
    if (!fs::exists(destination_file_directory)) {
      fs::create_directories(destination_file_directory);
    }
    BOOST_ASSERT_MSG(
        fs::exists(destination_file_directory),
        fmt::format("Unable to create/find destination_file_directory: {}",
                    destination_file_directory)
            .c_str());

    fs::path destination_zip_file = destination_zip_directory / file;
    if (fs::exists(destination_zip_file) && replace_files == false) {
      ++skipped_files_counter;
      spdlog::info(fmt::format(
          "N: File: {} exists and 'replace' is false. Skipping download.",
          destination_zip_file));
      continue;
    }

    namespace asio = boost::asio;
    using boost::process::process;
    try {
      fin_statement_downloader.DownloadFile(source_file, destination_zip_file);

      std::cout << "unzipping to: " << destination_file_directory.c_str()
                << '\n';
      asio::io_context ctx;
      process proc(ctx.get_executor(), // <1>
                   "/usr/bin/7z",      // <2>
                   {"x"s, "-o"s + destination_file_directory.string(),
                    destination_zip_file.string()} // <3>
      );                                           // <4>
      auto result = proc.wait();
      if (result != 0) {
        throw std::runtime_error(
            std::format("Unzipping file: {} failed with return code: {}",
                        destination_zip_file.c_str(), result));
      }

      // bp::system(
      //     fmt::format("7z x -o{} {}", destination_directory,
      //     destination_file), bp::std_out > bp::null, bp::std_err > bp::null);
      ++downloaded_files_counter;
    } catch (std::system_error &e) {
      ++error_counter;
      spdlog::error(e.what());
      auto ec = e.code();
      spdlog::error(catenate("Category: ", ec.category().name(), ". Value: ",
                             ec.value(), ". Message: ", ec.message()));
    }
  }

  spdlog::info(fmt::format(
      "N: Downloaded: {}. Skipped: {}. Errors: {}. out of {} possible files.",
      downloaded_files_counter, skipped_files_counter, error_counter,
      std::ranges::distance(*this)));

} // -----  end of method FinancialStatementsAndNotes::download_files  -----
