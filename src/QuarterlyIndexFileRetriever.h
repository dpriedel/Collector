// =====================================================================================
//
//       Filename:  QuarterlyIndexFileRetriever.h
//
//    Description:  Header for class which knows how to retrieve SEC quarterly
//    index files
//
//        Version:  1.0
//        Created:  01/30/2014 11:14:10 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:
//
// =====================================================================================

#ifndef QUARTERLYINDEXFILERETRIEVER_H_
#define QUARTERLYINDEXFILERETRIEVER_H_

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
//        Class:  QuarterlyIndexFileRetriever
//  Description:
// =====================================================================================
class QuarterlyIndexFileRetriever
{
public:
    // ====================  LIFECYCLE     =======================================
    QuarterlyIndexFileRetriever() = delete;
    QuarterlyIndexFileRetriever(const std::string &host, const std::string &port,
                                const fs::path &prefix); // constructor

    ~QuarterlyIndexFileRetriever() = default;

    QuarterlyIndexFileRetriever(const QuarterlyIndexFileRetriever &rhs) = delete;
    QuarterlyIndexFileRetriever(QuarterlyIndexFileRetriever &&rhs) = delete;
    //
    // ====================  ACCESSORS     =======================================

    // ====================  MUTATORS      =======================================

    QuarterlyIndexFileRetriever &operator=(const QuarterlyIndexFileRetriever &rhs) = delete;
    QuarterlyIndexFileRetriever &operator=(QuarterlyIndexFileRetriever &&rhs) = delete;

    fs::path MakeQuarterlyIndexPathName(std::chrono::year_month_day day_in_quarter);
    fs::path HierarchicalCopyRemoteIndexFileTo(const fs::path &remote_file_name,
                                               const fs::path &local_directory_name,
                                               bool replace_files = false);

    //	This method treats the date range as a closed interval.

    std::vector<fs::path> MakeIndexFileNamesForDateRange(std::chrono::year_month_day start_date,
                                                         std::chrono::year_month_day end_date);
    std::vector<fs::path> HierarchicalCopyIndexFilesForDateRangeTo(const std::vector<fs::path> &remote_file_list,
                                                                   const fs::path &local_directory_name,
                                                                   bool replace_files = false);
    std::vector<fs::path> ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo(
        const std::vector<fs::path> &remote_file_list, const fs::path &local_directory_name, int max_at_a_time,
        bool replace_files = false);

    // ====================  OPERATORS     =======================================

protected:
    std::chrono::year_month_day CheckDate(std::chrono::year_month_day aDate);
    fs::path MakeLocalIndexFilePath(const fs::path &local_prefix, const fs::path &remote_quarterly_index_file_name);

    // ====================  DATA MEMBERS  =======================================

private:
    auto AddToCopyList(const fs::path &local_directory_name, bool replace_files);

    // ====================  DATA MEMBERS  =======================================

    fs::path remote_directory_prefix_; // top-level directory path
    std::chrono::year_month_day input_date_;
    std::chrono::year_month_day start_date_;
    std::chrono::year_month_day end_date_;

    std::string host_;
    std::string port_;

}; // -----  end of class QuarterlyIndexFileRetriever  -----

#endif /* QUARTERLYINDEXFILERETRIEVER_H_ */
