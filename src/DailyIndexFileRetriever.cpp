// =====================================================================================
//
//       Filename:  DailyIndexFileRetriever.cpp
//
//    Description:  module to retrieve SEC daily index file for date
//    				nearest specified date
//
//        Version:  1.0
//        Created:  01/06/2014 10:25:52 AM
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

#include <algorithm>

#include <format>

#include <boost/algorithm/string/replace.hpp>
#include <spdlog/spdlog.h>

#include "Collector_Utils.h"
#include "DailyIndexFileRetriever.h"
#include "HTTPS_Downloader.h"
#include "PathNameGenerator.h"

//--------------------------------------------------------------------------------------
//       Class:  DailyIndexFileRetriever
//      Method:  DailyIndexFileRetriever
// Description:  constructor
//--------------------------------------------------------------------------------------

DailyIndexFileRetriever::DailyIndexFileRetriever(const std::string &host,
                                                 const std::string &port,
                                                 const fs::path &prefix)
    : host_{host}, port_{port}, remote_directory_prefix_{prefix}
{

} // -----  end of method DailyIndexFileRetriever::DailyIndexFileRetriever
  // (constructor)  -----

std::chrono::year_month_day DailyIndexFileRetriever::CheckDate(std::chrono::year_month_day aDate)
{
    input_date_ = {};

    //	we can only work with past data.

    std::chrono::year_month_day today{floor<std::chrono::days>(std::chrono::system_clock::now())};
    BOOST_ASSERT_MSG(
        aDate <= today,
        catenate(std::format("{:%F}", aDate), ": Date must be less than ", std::format("{:%F}", today)).c_str());

    return aDate;
} // -----  end of method DailyIndexFileRetriever::CheckDate  -----

fs::path DailyIndexFileRetriever::MakeDailyIndexPathName(std::chrono::year_month_day day_in_quarter)
{
    input_date_ = CheckDate(day_in_quarter);

    auto remote_index_file_directory = GeneratePath(remote_directory_prefix_, day_in_quarter);

    return remote_index_file_directory;

} // -----  end of method QuarterlyIndexFileRetriever::MakeQuarterIndexPathName
  // -----

fs::path DailyIndexFileRetriever::FindRemoteIndexFileNameNearestDate(std::chrono::year_month_day aDate)
{
    input_date_ = this->CheckDate(aDate);

    spdlog::debug(catenate("D: Looking for Daily Index File nearest date: ", std::format("{:%F}", aDate)));

    auto remote_diretory_name = MakeDailyIndexPathName(aDate);
    decltype(auto) directory_list = this->GetRemoteIndexList(remote_diretory_name);

    std::string looking_for = catenate("master.", std::format("{:%Y%m%d}", input_date_), ".idx");

    // index files may or may not be gzipped, so we need to exclude possible file
    // name suffix from comparisons

    auto pos = std::find_if(directory_list.crbegin(), directory_list.crend(), [&looking_for](const auto &x) {
        return x.compare(0, looking_for.size(), looking_for) <= 0;
    });
    BOOST_ASSERT_MSG(pos != directory_list.rend(),
                     catenate("Can't find daily index file for date: ", std::format("{:%F}", input_date_)).c_str());

    actual_file_date_ = StringToDateYMD("%Y%m%d", (*pos).substr(7, 8));

    spdlog::debug(catenate("D: Found Daily Index File for date: ", std::format("{:%F}", actual_file_date_)));

    auto remote_daily_index_file_name = remote_diretory_name /= *pos;
    return remote_daily_index_file_name;
} // -----  end of method DailyIndexFileRetriever::FindIndexFileDateNearest
  // -----

std::vector<fs::path> DailyIndexFileRetriever::FindRemoteIndexFileNamesForDateRange(
    std::chrono::year_month_day begin_date, std::chrono::year_month_day end_date)
{
    start_date_ = this->CheckDate(begin_date);
    end_date_ = this->CheckDate(end_date);

    spdlog::debug(catenate("D: Looking for Daily Index Files in date range from: ", std::format("{:%F}", start_date_),
                           " to: ", std::format("{:%F}", end_date_)));

    auto looking_for_start = std::string{"master."} + std::format("{:%Y%m%d}", start_date_) + ".idx";
    auto looking_for_end = std::string{"master."} + std::format("{:%Y%m%d}", end_date_) + ".idx";

    auto remote_directory_list = MakeIndexFileNamesForDateRange(begin_date, end_date);

    // index files may or may not be gzipped, so we need to exclude possible file
    // name suffix from comparisons so this function takes that into account.

    auto is_between{[&looking_for_start, &looking_for_end](const std::string &elem) {
        return (elem.compare(0, looking_for_end.size(), looking_for_end) <= 0 &&
                elem.compare(0, looking_for_start.size(), looking_for_start) >= 0);
    }};

    std::vector<fs::path> remote_daily_index_file_name_list;

    for (const auto &remote_directory : remote_directory_list)
    {
        decltype(auto) directory_list = this->GetRemoteIndexList(remote_directory);

        std::vector<std::string> found_files;

        std::copy_if(directory_list.crbegin(), directory_list.crend(), std::back_inserter(found_files), is_between);

        // last thing...prepend directory name

        std::transform(found_files.cbegin(), found_files.cend(), std::back_inserter(remote_daily_index_file_name_list),
                       [remote_directory](const auto &e) {
                           auto fpath{remote_directory};
                           return std::move(fpath /= e);
                       });
    }
    BOOST_ASSERT_MSG(!remote_daily_index_file_name_list.empty(),
                     catenate("Can't find daily index files for date range: ", std::format("{:%F}", start_date_), " ",
                              std::format("{:%F}", end_date_))
                         .c_str());

    actual_start_date_ =
        StringToDateYMD("%Y%m%d", remote_daily_index_file_name_list.back().filename().string().substr(7, 8));
    actual_end_date_ =
        StringToDateYMD("%Y%m%d", remote_daily_index_file_name_list.front().filename().string().substr(7, 8));

    spdlog::debug(catenate("D: Found ", remote_daily_index_file_name_list.size(), " files for date range."));

    return remote_daily_index_file_name_list;
} // -----  end of method
  // DailyIndexFileRetriever::FindIndexFileNamesForDateRange  -----

std::vector<fs::path> DailyIndexFileRetriever::MakeIndexFileNamesForDateRange(std::chrono::year_month_day begin_date,
                                                                              std::chrono::year_month_day end_date)
{
    start_date_ = this->CheckDate(begin_date);
    end_date_ = this->CheckDate(end_date);

    std::vector<fs::path> results;
    DateRange range{start_date_, end_date_};

    std::transform(std::begin(range), std::end(range), std::back_inserter(results), [this](const auto &qtr_begin) {
        auto remote_file_name = GeneratePath(this->remote_directory_prefix_, qtr_begin);
        return (std::move(remote_file_name));
    });
    return results;
} // -----  end of method
  // DailyIndexFileRetriever::FindIndexFileNamesForDateRange  -----

std::vector<std::string> DailyIndexFileRetriever::GetRemoteIndexList(const fs::path &remote_directory)
{
    HTTPS_Downloader the_server(host_, port_);
    auto directory_list = the_server.ListDirectoryContents(remote_directory);

    //	we need to do some cleanup of the directory listing to simplify our
    // searches.

    auto not_form = std::partition(directory_list.begin(), directory_list.end(),
                                   [](std::string &x) { return x.starts_with("master"); });
    directory_list.erase(not_form, directory_list.end());

    std::sort(directory_list.begin(), directory_list.end());

    return directory_list;
} // -----  end of method DailyIndexFileRetriever::GetRemoteIndexList  -----

fs::path DailyIndexFileRetriever::CopyRemoteIndexFileTo(const fs::path &remote_daily_index_file_name,
                                                        const fs::path &local_directory_name, bool replace_files)
{
    auto local_daily_index_file_name = local_directory_name;
    local_daily_index_file_name /= remote_daily_index_file_name.filename();

    if (local_daily_index_file_name.extension() == ".gz")
    {
        local_daily_index_file_name.replace_extension("");
    }

    if (!replace_files && fs::exists(local_daily_index_file_name))
    {
        spdlog::info(catenate("D: File exists and 'replace' is false: skipping download: ",
                              local_daily_index_file_name.filename().string()));
        return local_daily_index_file_name;
    }

    fs::create_directories(local_directory_name);

    HTTPS_Downloader the_server(host_, port_);
    the_server.DownloadFile(remote_daily_index_file_name, local_daily_index_file_name);

    spdlog::info(catenate("D: Retrieved remote daily index file: ", remote_daily_index_file_name.string(),
                          " to: ", local_daily_index_file_name.string()));

    return local_daily_index_file_name;

} // -----  end of method DailyIndexFileRetriever::RetrieveIndexFile  -----

fs::path DailyIndexFileRetriever::HierarchicalCopyRemoteIndexFileTo(const fs::path &remote_daily_index_file_name,
                                                                    const fs::path &local_directory_prefix,
                                                                    bool replace_files)
{
    auto local_daily_index_file_name = MakeLocalIndexFilePath(local_directory_prefix, remote_daily_index_file_name);

    if (local_daily_index_file_name.extension() == ".gz")
    {
        local_daily_index_file_name.replace_extension("");
    }

    if (!replace_files && fs::exists(local_daily_index_file_name))
    {
        spdlog::info(catenate("D: File exists and 'replace' is false: skipping download: ",
                              local_daily_index_file_name.filename().string()));
        return local_daily_index_file_name;
    }

    auto local_daily_index_file_directory = local_daily_index_file_name.parent_path();
    fs::create_directories(local_daily_index_file_directory);

    HTTPS_Downloader the_server(host_, port_);
    the_server.DownloadFile(remote_daily_index_file_name, local_daily_index_file_name);

    spdlog::info(catenate("D: Retrieved remote daily index file: ", remote_daily_index_file_name.string(),
                          " to: ", local_daily_index_file_name.string()));

    return local_daily_index_file_name;

} // -----  end of method DailyIndexFileRetriever::RetrieveIndexFile  -----

std::vector<fs::path> DailyIndexFileRetriever::CopyIndexFilesForDateRangeTo(
    const std::vector<fs::path> &remote_file_list, const fs::path &local_directory_name, bool replace_files)
{
    std::vector<fs::path> results;

    for (const auto &remote_daily_index_file_name : remote_file_list)
    {
        auto local_file = CopyRemoteIndexFileTo(remote_daily_index_file_name, local_directory_name, replace_files);
        results.push_back(std::move(local_file));
    }
    return results;
} // -----  end of method DailyIndexFileRetriever::CopyIndexFilesForDateRangeTo
  // -----

auto DailyIndexFileRetriever::AddToCopyList(const fs::path &local_directory_name, bool replace_files)
{
    //	construct our lambda function here so it doesn't clutter up our code
    // below.

    return [local_directory_name, replace_files](const auto &remote_file_name) {
        auto local_daily_index_file_name = local_directory_name;
        local_daily_index_file_name /= remote_file_name.filename();

        if (local_daily_index_file_name.extension() == ".gz")
        {
            local_daily_index_file_name.replace_extension("");
        }

        if (!replace_files && fs::exists(local_daily_index_file_name))
        {
            // we use an empty remote file name to indicate no copy needed as the
            // local file already exists.

            return HTTPS_Downloader::copy_file_names({}, local_daily_index_file_name);
        }
        return HTTPS_Downloader::copy_file_names(remote_file_name, std::move(local_daily_index_file_name));
    };
}

std::vector<fs::path> DailyIndexFileRetriever::ConcurrentlyCopyIndexFilesForDateRangeTo(
    const std::vector<fs::path> &remote_file_list, const fs::path &local_directory_name, int max_at_a_time,
    bool replace_files)
{
    if (remote_file_list.size() < max_at_a_time)
    {
        return CopyIndexFilesForDateRangeTo(remote_file_list, local_directory_name, replace_files);
    }

    fs::create_directories(local_directory_name);

    // we need to create a list of file name pairs -- remote file name, local file
    // name. we'll pass that list to the downloader and let it manage to process
    // from there.

    HTTPS_Downloader::remote_local_list concurrent_copy_list;

    std::transform(std::begin(remote_file_list), std::end(remote_file_list), std::back_inserter(concurrent_copy_list),
                   AddToCopyList(local_directory_name, replace_files));

    // now, we expect some magic to happen here...

    HTTPS_Downloader the_server(host_, port_);
    auto [success_counter, error_counter] = the_server.DownloadFilesConcurrently(concurrent_copy_list, max_at_a_time);

    int skipped_files_counter = std::count_if(std::begin(concurrent_copy_list), std::end(concurrent_copy_list),
                                              [](const auto &e) { return !e.first; });

    spdlog::info(catenate("D: Downloaded: ", success_counter, " Skipped: ", skipped_files_counter,
                          " Errors: ", error_counter, " daily index files."));

    // TODO: figure our error handling when some files do not get downloaded.
    // Let's try this for now.

    if (concurrent_copy_list.size() != success_counter + skipped_files_counter)
    {
        throw std::runtime_error(
            catenate("Download count = ", success_counter, ". Should be: ", concurrent_copy_list.size()));
    }

    std::vector<fs::path> results;

    for (auto &[remote_file, local_file] : concurrent_copy_list)
    {
        results.push_back(std::move(local_file));
    }

    return results;
} // -----  end of method DailyIndexFileRetriever::CopyIndexFilesForDateRangeTo
  // -----

std::vector<fs::path> DailyIndexFileRetriever::HierarchicalCopyIndexFilesForDateRangeTo(
    const std::vector<fs::path> &remote_file_list, const fs::path &local_directory_prefix, bool replace_files)
{
    std::vector<fs::path> results;

    for (const auto &remote_daily_index_file_name : remote_file_list)
    {
        auto local_file =
            HierarchicalCopyRemoteIndexFileTo(remote_daily_index_file_name, local_directory_prefix, replace_files);
        results.push_back(std::move(local_file));
    }
    return results;
} // -----  end of method DailyIndexFileRetriever::CopyIndexFilesForDateRangeTo
  // -----

auto DailyIndexFileRetriever::AddToConcurrentCopyList(const fs::path &local_directory_prefix, bool replace_files)
{
    //	construct our lambda function here so it doesn't clutter up our code
    // below.

    return [this, local_directory_prefix, replace_files](const auto &remote_file_name) {
        auto local_daily_index_file_name = this->MakeLocalIndexFilePath(local_directory_prefix, remote_file_name);
        local_daily_index_file_name /= remote_file_name.filename();

        if (local_daily_index_file_name.extension() == ".gz")
        {
            local_daily_index_file_name.replace_extension("");
        }

        if (!replace_files && fs::exists(local_daily_index_file_name))
        {
            // we use an empty remote file name to indicate no copy needed as the
            // local file already exists.

            return HTTPS_Downloader::copy_file_names({}, local_daily_index_file_name);
        }

        auto local_daily_index_file_directory = local_daily_index_file_name.parent_path();
        fs::create_directories(local_daily_index_file_directory);
        return HTTPS_Downloader::copy_file_names(remote_file_name, std::move(local_daily_index_file_name));
    };
}

std::vector<fs::path> DailyIndexFileRetriever::ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo(
    const std::vector<fs::path> &remote_file_list, const fs::path &local_directory_prefix, int max_at_a_time,
    bool replace_files)
{
    if (remote_file_list.size() < max_at_a_time)
    {
        return HierarchicalCopyIndexFilesForDateRangeTo(remote_file_list, local_directory_prefix, replace_files);
    }

    // we need to create a list of file name pairs -- remote file name, local file
    // name. we'll pass that list to the downloader and let it manage to process
    // from there.

    // Also, here we will create the directory hierarchies for the to-be
    // downloaded files. Taking the easy way out so we don't have to worry about
    // file system race conditions.

    HTTPS_Downloader::remote_local_list concurrent_copy_list;

    std::transform(std::begin(remote_file_list), std::end(remote_file_list), std::back_inserter(concurrent_copy_list),
                   AddToConcurrentCopyList(local_directory_prefix, replace_files));

    // now, we expect some magic to happen here...

    HTTPS_Downloader the_server(host_, port_);
    auto [success_counter, error_counter] = the_server.DownloadFilesConcurrently(concurrent_copy_list, max_at_a_time);

    // if the first file name in the pair is empty, there was no download done.

    int skipped_files_counter = std::count_if(std::begin(concurrent_copy_list), std::end(concurrent_copy_list),
                                              [](const auto &e) { return !e.first; });

    spdlog::info(catenate("D: Downloaded: ", success_counter, " Skipped: ", skipped_files_counter,
                          " Errors: ", error_counter, " daily index files."));

    // TODO: figure our error handling when some files do not get downloaded.
    // Let's try this for now.

    if (concurrent_copy_list.size() != success_counter)
    {
        throw std::runtime_error(
            catenate("Download count = ", success_counter, ". Should be: ", concurrent_copy_list.size()));
    }

    std::vector<fs::path> results;

    for (auto &[remote_file, local_file] : concurrent_copy_list)
    {
        results.push_back(std::move(local_file));
    }

    return results;
} // -----  end of method
  // DailyIndexFileRetriever::ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo
  // -----

fs::path DailyIndexFileRetriever::MakeLocalIndexFilePath(const fs::path &local_prefix,
                                                         const fs::path &remote_daily_index_file_name)
{
    // want keep the directory hierarchy the same as on the remote system
    // EXCCEPT for the remote system prefix (which is given to our ctor)

    // we will assume there is not trailing delimiter on the stored remote prefix.
    // (even though we have no edit to enforce that for now.)

    std::string remote_index_name = boost::algorithm::replace_first_copy(remote_daily_index_file_name.string(),
                                                                         remote_directory_prefix_.string(), "");

    //  there seems to be a change in behaviour.  appending a path starting with
    //  '/' actually does an assignment, not an append.

    if (remote_index_name[0] == '/')
    {
        remote_index_name.erase(0, 1);
    }
    auto local_daily_index_file_name = local_prefix;
    local_daily_index_file_name /= remote_index_name;
    return local_daily_index_file_name;

} // -----  end of method DailyIndexFileRetriever::MakeLocalIndexFileName  -----
