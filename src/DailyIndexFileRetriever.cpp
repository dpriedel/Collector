// =====================================================================================
//
//       Filename:  DailyIndexFileRetriever.cpp
//
//    Description:  module to retrieve EDGAR daily index file for date
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



#include <algorithm>
#include <fstream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>

//namespace bgpt = boost::posix_time;


#include "DailyIndexFileRetriever.h"
#include "PathNameGenerator.h"

//--------------------------------------------------------------------------------------
//       Class:  DailyIndexFileRetriever
//      Method:  DailyIndexFileRetriever
// Description:  constructor
//--------------------------------------------------------------------------------------

DailyIndexFileRetriever::DailyIndexFileRetriever(HTTPS_Downloader& a_server, const fs::path& prefix, Poco::Logger& the_logger)
	: the_server_{a_server}, remote_directory_prefix_{prefix}, the_logger_{the_logger}
{

}  // -----  end of method DailyIndexFileRetriever::DailyIndexFileRetriever  (constructor)  -----


DailyIndexFileRetriever::~DailyIndexFileRetriever(void)
{

}		// -----  end of method IndexFileRetreiver::~DailyIndexFileRetriever  -----


bg::date DailyIndexFileRetriever::CheckDate (const bg::date& aDate)
{
	input_date_ = bg::date();		//	don't know of a better way to clear date field.

	//	we can only work with past data.

	bg::date today{bg::day_clock::local_day()};
	poco_assert_msg(aDate <= today, ("Date must be less than " + bg::to_simple_string(today)).c_str());

	return aDate;
}		// -----  end of method DailyIndexFileRetriever::CheckDate  -----

fs::path DailyIndexFileRetriever::MakeDailyIndexPathName (const bg::date& day_in_quarter)
{
	input_date_ = CheckDate(day_in_quarter);

	auto remote_index_file_directory = GeneratePath(remote_directory_prefix_, day_in_quarter);

	return remote_index_file_directory;

}		// -----  end of method QuarterlyIndexFileRetriever::MakeQuarterIndexPathName  -----

fs::path DailyIndexFileRetriever::FindRemoteIndexFileNameNearestDate(const bg::date& aDate)
{
	input_date_ = this->CheckDate(aDate);

	poco_debug(the_logger_, "D: Looking for Daily Index File nearest date: " + bg::to_simple_string(aDate));

	auto remote_diretory_name = MakeDailyIndexPathName(aDate);
	decltype(auto) directory_list = this->GetRemoteIndexList(remote_diretory_name);

	std::string looking_for = std::string{"form."} + bg::to_iso_string(input_date_) + ".idx";

	// index files may or may not be gzipped, so we need to exclude possible file name suffix from comparisons

	decltype(auto) pos = std::find_if(directory_list.crbegin(), directory_list.crend(),
        [&looking_for](const auto& x) {return x.compare(0, looking_for.size(), looking_for) <= 0;});
	poco_assert_msg(pos != directory_list.rend(), ("Can't find daily index file for date: " + bg::to_simple_string(input_date_)).c_str());

	actual_file_date_ = bg::from_undelimited_string((*pos).substr(5, 8));

	poco_debug(the_logger_, "D: Found Daily Index File for date: " + bg::to_simple_string(actual_file_date_));

	auto remote_daily_index_file_name = remote_diretory_name /= *pos;
	return remote_daily_index_file_name;
}		// -----  end of method DailyIndexFileRetriever::FindIndexFileDateNearest  -----


const std::vector<fs::path>& DailyIndexFileRetriever::FindRemoteIndexFileNamesForDateRange(const bg::date& begin_date, const bg::date& end_date)
{
	start_date_ = this->CheckDate(begin_date);
	end_date_ = this->CheckDate(end_date);

	poco_debug(the_logger_, "D: Looking for Daily Index Files in date range from: " + bg::to_simple_string(start_date_)  + " to: " + bg::to_simple_string(end_date_));

	remote_daily_index_file_name_list_.clear();

	decltype(auto) looking_for_start = std::string{"form."} + bg::to_iso_string(start_date_) + ".idx";
	decltype(auto) looking_for_end = std::string{"form."} + bg::to_iso_string(end_date_) + ".idx";

	auto remote_directory_list = MakeIndexFileNamesForDateRange(begin_date, end_date);

	for (const auto& remote_directory : remote_directory_list)
	{
		decltype(auto) directory_list = this->GetRemoteIndexList(remote_directory);

		std::vector<std::string> found_files;

		// index files may or may not be gzipped, so we need to exclude possible file name suffix from comparisons

		std::copy_if(directory_list.crbegin(), directory_list.crend(), std::back_inserter(found_files),
				[&looking_for_start, &looking_for_end](const std::string& elem)
				{ return (elem.compare(0, looking_for_end.size(), looking_for_end) <= 0
				 	&& elem.compare(0, looking_for_start.size(), looking_for_start) >= 0); });

		// last thing...prepend directory name

		std::transform(found_files.cbegin(), found_files.cend(), std::back_inserter(remote_daily_index_file_name_list_),
			[remote_directory](const auto& e){ auto fpath{remote_directory}; return fpath /= e; });
	}
	poco_assert_msg(! remote_daily_index_file_name_list_.empty(), ("Can't find daily index files for date range: "
		   	+ bg::to_simple_string(start_date_) + " " + bg::to_simple_string(end_date_)).c_str());

	actual_start_date_ = bg::from_undelimited_string(remote_daily_index_file_name_list_.back().filename().string().substr(5, 8));
	actual_end_date_ = bg::from_undelimited_string(remote_daily_index_file_name_list_.front().filename().string().substr(5, 8));

	poco_debug(the_logger_, "D: Found " + std::to_string(remote_daily_index_file_name_list_.size()) + " files for date range.");

	return remote_daily_index_file_name_list_;
}		// -----  end of method DailyIndexFileRetriever::FindIndexFileNamesForDateRange  -----

const std::vector<fs::path> DailyIndexFileRetriever::MakeIndexFileNamesForDateRange(const bg::date& begin_date, const bg::date& end_date)
{
	start_date_ = this->CheckDate(begin_date);
	end_date_ = this->CheckDate(end_date);

	std::vector<fs::path> results;
	DateRange range{start_date_, end_date_};

	for (auto quarter_begin = std::begin(range); quarter_begin <= std::end(range); ++quarter_begin)
	{
		auto remote_file_name = GeneratePath(remote_directory_prefix_, *quarter_begin);
		results.push_back(std::move(remote_file_name));
	}
	return results;
}		// -----  end of method DailyIndexFileRetriever::FindIndexFileNamesForDateRange  -----


std::vector<std::string> DailyIndexFileRetriever::GetRemoteIndexList (const fs::path& remote_directory)
{
	decltype(auto) directory_list = the_server_.ListDirectoryContents(remote_directory);

	//	we need to do some cleanup of the directory listing to simplify our searches.

	decltype(auto) not_form = std::partition(directory_list.begin(), directory_list.end(),
			[](std::string& x) {return boost::algorithm::starts_with(x, "form");});
	directory_list.erase(not_form, directory_list.end());

	std::sort(directory_list.begin(), directory_list.end());

	return directory_list;
}		// -----  end of method DailyIndexFileRetriever::GetRemoteIndexList  -----

fs::path DailyIndexFileRetriever::CopyRemoteIndexFileTo (const fs::path& remote_daily_index_file_name, const fs::path& local_directory_name, bool replace_files)
{
	auto local_daily_index_file_name = local_directory_name;
	local_daily_index_file_name /= remote_daily_index_file_name.filename();

	if (local_daily_index_file_name.extension() == ".gz")
		local_daily_index_file_name.replace_extension("");

	if (! replace_files && fs::exists(local_daily_index_file_name))
	{
		poco_information(the_logger_, "D: File exists and 'replace' is false: skipping download: " + local_daily_index_file_name.filename().string());
		return local_daily_index_file_name;
	}

	fs::create_directories(local_directory_name);

	the_server_.DownloadFile(remote_daily_index_file_name, local_daily_index_file_name);

	poco_information(the_logger_, "D: Retrieved remote daily index file: " + remote_daily_index_file_name.string() + " to: " + local_daily_index_file_name.string());

	return local_daily_index_file_name;

}		// -----  end of method DailyIndexFileRetriever::RetrieveIndexFile  -----


fs::path DailyIndexFileRetriever::HierarchicalCopyRemoteIndexFileTo (const fs::path& remote_daily_index_file_name, const fs::path& local_directory_prefix, bool replace_files)
{
	auto local_daily_index_file_name = MakeLocalIndexFilePath(local_directory_prefix, remote_daily_index_file_name);

	if (local_daily_index_file_name.extension() == ".gz")
		local_daily_index_file_name.replace_extension("");

	auto local_daily_index_file_directory = local_daily_index_file_name.parent_path();

	if (! replace_files && fs::exists(local_daily_index_file_name))
	{
		poco_information(the_logger_, "D: File exists and 'replace' is false: skipping download: " + local_daily_index_file_name.filename().string());
		return local_daily_index_file_name;
	}

	fs::create_directories(local_daily_index_file_directory);

	the_server_.DownloadFile(remote_daily_index_file_name, local_daily_index_file_name);

	poco_information(the_logger_, "D: Retrieved remote daily index file: " + remote_daily_index_file_name.string() + " to: " + local_daily_index_file_name.string());

	return local_daily_index_file_name;

}		// -----  end of method DailyIndexFileRetriever::RetrieveIndexFile  -----


std::vector<fs::path> DailyIndexFileRetriever::CopyIndexFilesForDateRangeTo (const std::vector<fs::path>& remote_file_list, const fs::path& local_directory_name, bool replace_files)
{
	std::vector<fs::path> results;

	for (const auto& remote_daily_index_file_name : remote_file_list)
	{
		auto local_file = CopyRemoteIndexFileTo(remote_daily_index_file_name, local_directory_name, replace_files);
		results.push_back(std::move(local_file));
	}
	return results;
}		// -----  end of method DailyIndexFileRetriever::CopyIndexFilesForDateRangeTo  -----

std::vector<fs::path> DailyIndexFileRetriever::ConcurrentlyCopyIndexFilesForDateRangeTo (const std::vector<fs::path>& remote_file_list, const fs::path& local_directory_name, int max_at_a_time, bool replace_files)
{
	if (remote_file_list.size() < max_at_a_time)
		return CopyIndexFilesForDateRangeTo(remote_file_list, local_directory_name, replace_files);

	std::vector<fs::path> results;

	fs::create_directories(local_directory_name);

    int skipped_files_counter = 0;

	// we need to create a list of file name pairs -- remote file name, local file name.
	// we'll pass that list to the downloader and let it manage to process from there.

	HTTPS_Downloader::remote_local_list concurrent_copy_list;

	for (const auto& remote_file_name : remote_file_list)
	{
		auto local_daily_index_file_name = local_directory_name;
		local_daily_index_file_name /= remote_file_name.filename();

		if (local_daily_index_file_name.extension() == ".gz")
			local_daily_index_file_name.replace_extension("");

		if (! replace_files && fs::exists(local_daily_index_file_name))
		{
			results.push_back(std::move(local_daily_index_file_name));
			++skipped_files_counter;
		}
		else
			concurrent_copy_list.emplace_back(std::pair(remote_file_name, local_daily_index_file_name));

	}

	// now, we expect some magic to happen here...

	auto [success_counter, error_counter] = the_server_.DownloadFilesConcurrently(concurrent_copy_list, max_at_a_time);

	poco_information(the_logger_, "D: Downloaded: " + std::to_string(success_counter) +
		" Skipped: " + std::to_string(skipped_files_counter) +
		" Errors: " + std::to_string(error_counter) + " daily index files.");

	// TODO: figure our error handling when some files do not get downloaded.
    // Let's try this for now.

    if (concurrent_copy_list.size() != success_counter)
        throw std::runtime_error("Download count = " + std::to_string(success_counter) + ". Should be: " + std::to_string(concurrent_copy_list.size()));

	for (auto& [remote_file, local_file] : concurrent_copy_list)
		results.push_back(std::move(local_file));

	return results;
}		// -----  end of method DailyIndexFileRetriever::CopyIndexFilesForDateRangeTo  -----

std::vector<fs::path> DailyIndexFileRetriever::HierarchicalCopyIndexFilesForDateRangeTo (const std::vector<fs::path>& remote_file_list, const fs::path& local_directory_prefix, bool replace_files)
{
	std::vector<fs::path> results;

	for (const auto& remote_daily_index_file_name : remote_file_list)
	{
		auto local_file = HierarchicalCopyRemoteIndexFileTo(remote_daily_index_file_name, local_directory_prefix, replace_files);
		results.push_back(std::move(local_file));
	}
	return results;
}		// -----  end of method DailyIndexFileRetriever::CopyIndexFilesForDateRangeTo  -----

std::vector<fs::path> DailyIndexFileRetriever::ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo (const std::vector<fs::path>& remote_file_list, const fs::path& local_directory_prefix, int max_at_a_time, bool replace_files)
{
	if (remote_file_list.size() < max_at_a_time)
		return HierarchicalCopyIndexFilesForDateRangeTo(remote_file_list, local_directory_prefix, replace_files);

	std::vector<fs::path> results;

	// we need to create a list of file name pairs -- remote file name, local file name.
	// we'll pass that list to the downloader and let it manage to process from there.

	// Also, here we will create the directory hierarchies for the to-be downloaded files.
	// Taking the easy way out so we don't have to worry about file system race conditions.

    int skipped_files_counter = 0;

	HTTPS_Downloader::remote_local_list concurrent_copy_list;

	for (const auto& remote_file_name : remote_file_list)
	{
		auto local_daily_index_file_name = MakeLocalIndexFilePath(local_directory_prefix, remote_file_name);

		if (local_daily_index_file_name.extension() == ".gz")
			local_daily_index_file_name.replace_extension("");

		if (! replace_files && fs::exists(local_daily_index_file_name))
		{
			results.push_back(std::move(local_daily_index_file_name));
			++skipped_files_counter;
		}
		else
        {
		    auto local_daily_index_file_directory = local_daily_index_file_name.parent_path();
            fs::create_directories(local_daily_index_file_directory);
    		concurrent_copy_list.emplace_back(std::pair(remote_file_name, local_daily_index_file_name));
        }
	}

	// now, we expect some magic to happen here...

	auto [success_counter, error_counter] = the_server_.DownloadFilesConcurrently(concurrent_copy_list, max_at_a_time);

	poco_information(the_logger_, "D: Downloaded: " + std::to_string(success_counter) +
		" Skipped: " + std::to_string(skipped_files_counter) +
		" Errors: " + std::to_string(error_counter) + " daily index files.");

	// TODO: figure our error handling when some files do not get downloaded.
    // Let's try this for now.

    if (concurrent_copy_list.size() != success_counter)
        throw std::runtime_error("Download count = " + std::to_string(success_counter) + ". Should be: " + std::to_string(concurrent_copy_list.size()));

	for (auto& [remote_file, local_file] : concurrent_copy_list)
		results.push_back(std::move(local_file));

	return results;
}		// -----  end of method DailyIndexFileRetriever::ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo  -----

fs::path DailyIndexFileRetriever::MakeLocalIndexFilePath (const fs::path& local_prefix, const fs::path& remote_daily_index_file_name)
{
	// want keep the directory hierarchy the same as on the remote system
	// EXCCEPT for the remote system prefix (which is given to our ctor)

	// we will assume there is not trailing delimiter on the stored remote prefix.
	// (even though we have no edit to enforce that for now.)

	std::string remote_index_name = boost::algorithm::replace_first_copy(remote_daily_index_file_name.string(), remote_directory_prefix_.string(), "");
	auto local_daily_index_file_name = local_prefix;
	local_daily_index_file_name /= remote_index_name;
	return local_daily_index_file_name;

}		// -----  end of method DailyIndexFileRetriever::MakeLocalIndexFileName  -----
