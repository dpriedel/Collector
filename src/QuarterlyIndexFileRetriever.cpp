// =====================================================================================
//
//       Filename:  QuarterlyIndexFileRetriever.cpp
//
//    Description:  Implements a class which knows how to retrieve EDGAR quarterly index files
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


#include <fstream>

#include <boost/algorithm/string/replace.hpp>
#include <Poco/Zip/Decompress.h>
#include "QuarterlyIndexFileRetriever.h"
#include "PathNameGenerator.h"

//--------------------------------------------------------------------------------------
//       Class:  QuarterlyIndexFileRetriever
//      Method:  QuarterlyIndexFileRetriever
// Description:  constructor
//--------------------------------------------------------------------------------------
QuarterlyIndexFileRetriever::QuarterlyIndexFileRetriever (HTTPS_Downloader& a_server, const fs::path& prefix, Poco::Logger& the_logger)
	: the_server_{a_server}, remote_directory_prefix_{prefix}, the_logger_{the_logger}
{
}  // -----  end of method QuarterlyIndexFileRetriever::QuarterlyIndexFileRetriever  (constructor)  -----


bg::date QuarterlyIndexFileRetriever::CheckDate (const bg::date& day_in_quarter)
{
	input_date_ = bg::date();		//	don't know of a better way to clear date field.

	//	we can only work with past data.

	bg::date today{bg::day_clock::local_day()};
	poco_assert_msg(day_in_quarter < today, ("Date must be less than " + bg::to_simple_string(today)).c_str());

	return day_in_quarter;
}		// -----  end of method QuarterlyIndexFileRetriever::CheckDate  -----

fs::path QuarterlyIndexFileRetriever::MakeQuarterlyIndexPathName (const bg::date& day_in_quarter)
{
	input_date_ = CheckDate(day_in_quarter);

	auto remote_quarterly_index_file_name = GeneratePath(remote_directory_prefix_, day_in_quarter);
	remote_quarterly_index_file_name /= "form.zip";		// we know this.

	return remote_quarterly_index_file_name;

}		// -----  end of method QuarterlyIndexFileRetriever::MakeQuarterIndexPathName  -----


fs::path QuarterlyIndexFileRetriever::HierarchicalCopyRemoteIndexFileTo (const fs::path& remote_file_name, const fs::path& local_directory_name, bool replace_files)
{
	auto local_quarterly_index_file_name = this->MakeLocalIndexFilePath(local_directory_name, remote_file_name);
	auto local_quarterly_index_file_directory = local_quarterly_index_file_name.parent_path();
	fs::create_directories(local_quarterly_index_file_directory);

	if (! replace_files && fs::exists(local_quarterly_index_file_name))
	{
		poco_information(the_logger_, "Q: File exists and 'replace' is false: skipping download: " + local_quarterly_index_file_name.filename().string());
		return local_quarterly_index_file_name;
	}

	the_server_.DownloadFile(remote_file_name, local_quarterly_index_file_name);

	poco_information(the_logger_, "Q: Retrieved remote quarterly index file: " + remote_file_name.string() + " to: " + local_quarterly_index_file_name.string());

	return local_quarterly_index_file_name;
}		// -----  end of method QuarterlyIndexFileRetriever::CopyRemoteIndexFileTo  -----


fs::path QuarterlyIndexFileRetriever::MakeLocalIndexFilePath (const fs::path& local_prefix, const fs::path& remote_quarterly_index_file_name)
{
	// want keep the directory hierarchy the same as on the remote system
	// EXCCEPT for the remote system prefix (which is given to our ctor)

	// we will assume there is not trailing delimiter on the stored remote prefix.
	// (even though we have no edit to enforce that for now.)

	std::string remote_index_name = boost::algorithm::replace_first_copy(remote_quarterly_index_file_name.string(), remote_directory_prefix_.string(), "");
	auto local_quarterly_index_file_name = local_prefix;
	local_quarterly_index_file_name /= remote_index_name;
	local_quarterly_index_file_name.replace_extension("idx");

	return local_quarterly_index_file_name;
}		// -----  end of method QuarterlyIndexFileRetriever::MakeLocalIndexFilePath  -----


const std::vector<fs::path> QuarterlyIndexFileRetriever::MakeIndexFileNamesForDateRange(const bg::date& begin_date, const bg::date& end_date)
{
	start_date_ = this->CheckDate(begin_date);
	end_date_ = this->CheckDate(end_date);

	std::vector<fs::path> results;

	DateRange range{start_date_, end_date_};

	for (auto quarter_begin = std::begin(range); quarter_begin <= std::end(range); ++quarter_begin)
	{
		auto remote_file_name = GeneratePath(remote_directory_prefix_, *quarter_begin);
		remote_file_name /= "form.zip";
		results.push_back(remote_file_name);
	}

	return results;
}		// -----  end of method DailyIndexFileRetriever::FindIndexFileNamesForDateRange  -----


std::vector<fs::path> QuarterlyIndexFileRetriever::HierarchicalCopyIndexFilesForDateRangeTo (const std::vector<fs::path>& remote_file_list, const fs::path& local_directory_name, bool replace_files)
{
	std::vector<fs::path> results;

	//	Remember...we are working with compressed directory files on the EDGAR server

	for (const auto& remote_file : remote_file_list)
	{
		auto local_file = HierarchicalCopyRemoteIndexFileTo(remote_file, local_directory_name, replace_files);
		results.push_back(local_file);
	}

	return results;
}		// -----  end of method QuarterlyIndexFileRetriever::CopyIndexFilesForDateRangeTo  -----

std::vector<fs::path> QuarterlyIndexFileRetriever::ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo (const std::vector<fs::path>& remote_file_list, const fs::path& local_directory_name, int max_at_a_time, bool replace_files)
{
	if (remote_file_list.size() < max_at_a_time)
		return HierarchicalCopyIndexFilesForDateRangeTo(remote_file_list, local_directory_name, replace_files);

	std::vector<fs::path> results;

	// we need to create a list of file name pairs -- remote file name, local file name.
	// we'll pass that list to the downloader and let it manage to process from there.

	// Also, here we will create the directory hierarchies for the to-be downloaded files.
	// Taking the easy way out so we don't have to worry about file system race conditions.

    int skipped_files_counter = 0;

	HTTPS_Downloader::remote_local_list concurrent_copy_list;

	for (const auto& remote_file_name : remote_file_list)
	{
		auto local_quarterly_index_file_name = this->MakeLocalIndexFilePath(local_directory_name, remote_file_name);

		if (! replace_files && fs::exists(local_quarterly_index_file_name))
		{
			results.push_back(local_quarterly_index_file_name);
			++skipped_files_counter;
		}
		else
        {
    		auto local_quarterly_index_file_directory = local_quarterly_index_file_name.parent_path();
    		fs::create_directories(local_quarterly_index_file_directory);
			concurrent_copy_list.push_back(std::make_pair(remote_file_name, local_quarterly_index_file_name));
        }
	}

	// now, we expect some magic to happen here...

	auto [success_counter, error_counter] = the_server_.DownloadFilesConcurrently(concurrent_copy_list, max_at_a_time);

	poco_information(the_logger_, "Q: Downloaded: " + std::to_string(success_counter) +
		" Skipped: " + std::to_string(skipped_files_counter) +
		" Errors: " + std::to_string(error_counter) + " quarterly index files.");

	// TODO: figure our error handling when some files do not get downloaded.
    // Let's try this for now.

    if (concurrent_copy_list.size() != success_counter)
        throw std::runtime_error("Download count = " + std::to_string(success_counter) + ". Should be: " + std::to_string(concurrent_copy_list.size()));

	for (const auto& e : concurrent_copy_list)
		results.push_back(e.second);

	return results;
}		// -----  end of method DailyIndexFileRetriever::ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo  -----
