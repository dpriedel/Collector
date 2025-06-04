// =====================================================================================
//
//       Filename:  DailyIndexFileRetriever.h
//
//    Description:  module to retrieve SEC daily index file for date
//    				nearest specified date
//
//        Version:  1.0
//        Created:  01/03/2014 10:25:52 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:
//
// =====================================================================================

#ifndef DAILYINDEXFILERETRIEVER_H_
#define DAILYINDEXFILERETRIEVER_H_

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
#include <string>
#include <vector>

namespace fs = std::filesystem;

// =====================================================================================
//        Class:  DailyIndexFileRetriever
//  Description:
// =====================================================================================

class DailyIndexFileRetriever {
public:
  // ====================  LIFECYCLE     =======================================
  DailyIndexFileRetriever() = delete;
  DailyIndexFileRetriever(const std::string &host, const std::string &port,
                          const fs::path &prefix);
  DailyIndexFileRetriever(const DailyIndexFileRetriever &rhs) = delete;
  DailyIndexFileRetriever(DailyIndexFileRetriever &&rhs) = delete;

  ~DailyIndexFileRetriever() = default;

  // ====================  ACCESSORS     =======================================

  [[nodiscard]] std::chrono::year_month_day GetActualIndexFileDate() const {
    return actual_file_date_;
  }
  [[nodiscard]] std::pair<std::chrono::year_month_day,
                          std::chrono::year_month_day>
  GetActualDateRange() const {
    return std::pair(actual_start_date_, actual_end_date_);
  }

  // ====================  MUTATORS      =======================================

  //	If there is no file for the specified date, this function will return
  // the 	immediately prior file.

  DailyIndexFileRetriever &
  operator=(const DailyIndexFileRetriever &rhs) = delete;
  DailyIndexFileRetriever &operator=(DailyIndexFileRetriever &&rhs) = delete;

  fs::path
  FindRemoteIndexFileNameNearestDate(std::chrono::year_month_day aDate);

  //	returns the local path name of the downloaded file.

  fs::path CopyRemoteIndexFileTo(const fs::path &remote_daily_index_file_name,
                                 const fs::path &local_directory_name,
                                 bool replace_files = false);
  fs::path HierarchicalCopyRemoteIndexFileTo(
      const fs::path &remote_daily_index_file_name,
      const fs::path &local_directory_prefix, bool replace_files = false);

  //	This method treats the date range as a closed interval.

  std::vector<fs::path>
  FindRemoteIndexFileNamesForDateRange(std::chrono::year_month_day start_date,
                                       std::chrono::year_month_day end_date);
  std::vector<fs::path>
  MakeIndexFileNamesForDateRange(std::chrono::year_month_day start_date,
                                 std::chrono::year_month_day end_date);

  //	returns the local path name of the downloaded file.

  std::vector<fs::path>
  CopyIndexFilesForDateRangeTo(const std::vector<fs::path> &remote_file_list,
                               const fs::path &local_directory_name,
                               bool replace_files = false);
  std::vector<fs::path> ConcurrentlyCopyIndexFilesForDateRangeTo(
      const std::vector<fs::path> &remote_file_list,
      const fs::path &local_directory_name, int max_at_a_time,
      bool replace_files = false);

  std::vector<fs::path> HierarchicalCopyIndexFilesForDateRangeTo(
      const std::vector<fs::path> &remote_file_list,
      const fs::path &local_directory_prefix, bool replace_files = false);
  std::vector<fs::path> ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo(
      const std::vector<fs::path> &remote_file_list,
      const fs::path &local_directory_prefix, int max_at_a_time,
      bool replace_files = false);

  // daily files are now organized in a directory hierarchy the same as
  // quarterly index files.

  fs::path MakeDailyIndexPathName(std::chrono::year_month_day day_in_quarter);

  // ====================  OPERATORS     =======================================

protected:
  std::chrono::year_month_day CheckDate(std::chrono::year_month_day aDate);
  fs::path MakeLocalIndexFilePath(const fs::path &local_prefix,
                                  const fs::path &remote_daily_index_file_name);
  std::vector<std::string> GetRemoteIndexList(const fs::path &remote_directory);

  // ====================  DATA MEMBERS  =======================================

private:
  auto AddToCopyList(const fs::path &local_directory_name, bool replace_files);
  auto AddToConcurrentCopyList(const fs::path &local_directory_prefix,
                               bool replace_files);

  // ====================  DATA MEMBERS  =======================================

  fs::path remote_directory_prefix_; // top-level directory path
  std::chrono::year_month_day input_date_;
  std::chrono::year_month_day actual_file_date_;
  std::chrono::year_month_day start_date_;
  std::chrono::year_month_day end_date_;
  std::chrono::year_month_day actual_start_date_;
  std::chrono::year_month_day actual_end_date_;

  std::string host_;
  std::string port_;

}; // -----  end of class DailyIndexFileRetriever  -----

#endif /* DAILYINDEXFILERETRIEVER_H_ */
