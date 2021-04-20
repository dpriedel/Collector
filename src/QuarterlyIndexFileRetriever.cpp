// =====================================================================================
//
//       Filename:  QuarterlyIndexFileRetriever.cpp
//
//    Description:  Implements a class which knows how to retrieve SEC quarterly index files
//
//        Version:  1.0
//        Created:  01/30/2014 11:23:18 AM
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


#include <fstream>

#include <boost/algorithm/string/replace.hpp>

#include "spdlog/spdlog.h"

#include "Collector_Utils.h"
#include "HTTPS_Downloader.h"
#include "PathNameGenerator.h"
#include "QuarterlyIndexFileRetriever.h"

//--------------------------------------------------------------------------------------
//       Class:  QuarterlyIndexFileRetriever
//      Method:  QuarterlyIndexFileRetriever
// Description:  constructor
//--------------------------------------------------------------------------------------
QuarterlyIndexFileRetriever::QuarterlyIndexFileRetriever (const std::string& host, const std::string& port, const fs::path& prefix)
	: host_{host}, port_{port}, remote_directory_prefix_{prefix}
{
}  // -----  end of method QuarterlyIndexFileRetriever::QuarterlyIndexFileRetriever  (constructor)  -----


date::year_month_day QuarterlyIndexFileRetriever::CheckDate (date::year_month_day day_in_quarter)
{
	input_date_ = {};

	//	we can only work with past data.
    
    auto today = date::year_month_day{floor<date::days>(std::chrono::system_clock::now())};
	BOOST_ASSERT_MSG(day_in_quarter < today, catenate("Date must be less than ", date::format("%F", today)).c_str());

	return day_in_quarter;
}		// -----  end of method QuarterlyIndexFileRetriever::CheckDate  -----

fs::path QuarterlyIndexFileRetriever::MakeQuarterlyIndexPathName (date::year_month_day day_in_quarter)
{
	input_date_ = CheckDate(day_in_quarter);

	auto remote_quarterly_index_file_name = GeneratePath(remote_directory_prefix_, day_in_quarter);
	remote_quarterly_index_file_name /= "form.zip";		// we know this.

	return remote_quarterly_index_file_name;

}		// -----  end of method QuarterlyIndexFileRetriever::MakeQuarterIndexPathName  -----


fs::path QuarterlyIndexFileRetriever::HierarchicalCopyRemoteIndexFileTo (const fs::path& remote_file_name, const fs::path& local_directory_name,
    bool replace_files)
{
	auto local_quarterly_index_file_name = this->MakeLocalIndexFilePath(local_directory_name, remote_file_name);

	if (! replace_files && fs::exists(local_quarterly_index_file_name))
	{
        spdlog::info(catenate("Q: File exists and 'replace' is false: skipping download: ",
                    local_quarterly_index_file_name.filename().string()));
		return local_quarterly_index_file_name;
	}

	auto local_quarterly_index_file_directory = local_quarterly_index_file_name.parent_path();
	fs::create_directories(local_quarterly_index_file_directory);

    HTTPS_Downloader the_server(host_, port_);
	the_server.DownloadFile(remote_file_name, local_quarterly_index_file_name);

    spdlog::info(catenate("Q: Retrieved remote quarterly index file: ", remote_file_name.string(), " to: ",
        local_quarterly_index_file_name.string()));

	return local_quarterly_index_file_name;
}		// -----  end of method QuarterlyIndexFileRetriever::CopyRemoteIndexFileTo  -----


fs::path QuarterlyIndexFileRetriever::MakeLocalIndexFilePath (const fs::path& local_prefix, const fs::path& remote_quarterly_index_file_name)
{
	// want keep the directory hierarchy the same as on the remote system
	// EXCCEPT for the remote system prefix (which is given to our ctor)

	// we will assume there is not trailing delimiter on the stored remote prefix.
	// (even though we have no edit to enforce that for now.)

    //  there seems to be a change in behaviour.  appending a path starting with '/' actually does an assignment, not an append.

	std::string remote_index_name = boost::algorithm::replace_first_copy(remote_quarterly_index_file_name.string(), remote_directory_prefix_.string(), "");
    if (remote_index_name[0] == '/')
    {
        remote_index_name.erase(0, 1);
    }
	auto local_quarterly_index_file_name = local_prefix;
	local_quarterly_index_file_name /= remote_index_name;
	local_quarterly_index_file_name.replace_extension("idx");

	return local_quarterly_index_file_name;
}		// -----  end of method QuarterlyIndexFileRetriever::MakeLocalIndexFilePath  -----


std::vector<fs::path> QuarterlyIndexFileRetriever::MakeIndexFileNamesForDateRange(date::year_month_day begin_date, date::year_month_day end_date)
{
	start_date_ = this->CheckDate(begin_date);
	end_date_ = this->CheckDate(end_date);

	std::vector<fs::path> results;

	DateRange range{start_date_, end_date_};

    std::transform(
        std::begin(range),
        std::end(range),
        std::back_inserter(results),
        [this] (const auto& qtr_begin)
            {
                auto remote_file_name = GeneratePath(this->remote_directory_prefix_, qtr_begin);
                return (std::move(remote_file_name /= "form.zip"));
            }
		);

	return results;
}		// -----  end of method DailyIndexFileRetriever::FindIndexFileNamesForDateRange  -----

std::vector<fs::path> QuarterlyIndexFileRetriever::HierarchicalCopyIndexFilesForDateRangeTo (const std::vector<fs::path>& remote_file_list,
    const fs::path& local_directory_name, bool replace_files)
{
	std::vector<fs::path> results;

	//	Remember...we are working with compressed directory files on the SEC server

    results.reserve(remote_file_list.size());
	for (const auto& remote_file : remote_file_list)
	{
		results.push_back(HierarchicalCopyRemoteIndexFileTo(remote_file, local_directory_name, replace_files));
	}

	return results;
}		// -----  end of method QuarterlyIndexFileRetriever::CopyIndexFilesForDateRangeTo  -----

auto QuarterlyIndexFileRetriever::AddToCopyList(const fs::path& local_directory_name, bool replace_files)
{
	//	construct our lambda function here so it doesn't clutter up our code below.

	return [this, local_directory_name, replace_files](const auto& remote_file_name)
	{
		auto local_quarterly_index_file_name = this->MakeLocalIndexFilePath(local_directory_name, remote_file_name);

		if (! replace_files && fs::exists(local_quarterly_index_file_name))
		{
			// we use an empty remote file name to indicate no copy needed as the local file already exists.

			return HTTPS_Downloader::copy_file_names({}, local_quarterly_index_file_name);
		}
        auto local_quarterly_index_file_directory = local_quarterly_index_file_name.parent_path();
        fs::create_directories(local_quarterly_index_file_directory);
        return HTTPS_Downloader::copy_file_names(remote_file_name, std::move(local_quarterly_index_file_name));
	};
}

std::vector<fs::path> QuarterlyIndexFileRetriever::ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo (const std::vector<fs::path>& remote_file_list,
    const fs::path& local_directory_name, int max_at_a_time, bool replace_files)
{
	if (remote_file_list.size() < max_at_a_time)
    {
		return HierarchicalCopyIndexFilesForDateRangeTo(remote_file_list, local_directory_name, replace_files);
    }

	// we need to create a list of file name pairs -- remote file name, local file name.
	// we'll pass that list to the downloader and let it manage to process from there.

	// Also, here we will create the directory hierarchies for the to-be downloaded files.
	// Taking the easy way out so we don't have to worry about file system race conditions.

	HTTPS_Downloader::remote_local_list concurrent_copy_list;

	std::transform(
		std::begin(remote_file_list),
		std::end(remote_file_list),
		std::back_inserter(concurrent_copy_list),
		AddToCopyList(local_directory_name, replace_files)
		);

	// now, we expect some magic to happen here...

    HTTPS_Downloader the_server(host_, port_);
	auto [success_counter, error_counter] = the_server.DownloadFilesConcurrently(concurrent_copy_list, max_at_a_time);

    // if the first file name in the pair is empty, there was no download done.

    int skipped_files_counter = std::count_if(std::begin(concurrent_copy_list), std::end(concurrent_copy_list),
		[] (const auto& e) { return ! e.first; });

    spdlog::info(catenate("Q: Downloaded: ", success_counter, " Skipped: ", skipped_files_counter, " Errors: ",
                error_counter, " quarterly index files."));

	// TODO: figure our error handling when some files do not get downloaded.
    // Let's try this for now.

    if (concurrent_copy_list.size() != success_counter + skipped_files_counter)
    {
        throw std::runtime_error(catenate("Download count = ", success_counter, ". Should be: ",
                    concurrent_copy_list.size()));
    }

	std::vector<fs::path> results;

	for (auto& [remote_name, local_name] : concurrent_copy_list)
    {
		results.push_back(std::move(local_name));
    }

	return results;
}		// -----  end of method DailyIndexFileRetriever::ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo  -----
