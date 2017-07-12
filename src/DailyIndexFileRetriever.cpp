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

	PathNameGenerator p_gen{remote_directory_prefix_, day_in_quarter, day_in_quarter};

	remote_index_file_directory_ = *p_gen;

	return remote_index_file_directory_;

}		// -----  end of method QuarterlyIndexFileRetriever::MakeQuarterIndexPathName  -----

fs::path DailyIndexFileRetriever::FindRemoteIndexFileNameNearestDate(const bg::date& aDate)
{
	input_date_ = this->CheckDate(aDate);

	poco_debug(the_logger_, "D: Looking for Daily Index File nearest date: " + bg::to_simple_string(aDate));

	decltype(auto) directory_list = this->GetRemoteIndexList(aDate);

	std::string looking_for = std::string{"form."} + bg::to_iso_string(input_date_) + ".idx";

	// index files may or may not be gzipped, so we need to exclude possible file name suffix from comparisons

	decltype(auto) pos = std::find_if(directory_list.crbegin(), directory_list.crend(),
        [&looking_for](const auto& x) {return x.compare(0, looking_for.size(), looking_for) <= 0;});
	poco_assert_msg(pos != directory_list.rend(), ("Can't find daily index file for date: " + bg::to_simple_string(input_date_)).c_str());

	actual_file_date_ = bg::from_undelimited_string((*pos).substr(5, 8));

	poco_debug(the_logger_, "D: Found Daily Index File for date: " + bg::to_simple_string(actual_file_date_));

	remote_daily_index_file_name_ = remote_index_file_directory_ /= *pos;
	return remote_daily_index_file_name_;
}		// -----  end of method DailyIndexFileRetriever::FindIndexFileDateNearest  -----


const std::vector<fs::path>& DailyIndexFileRetriever::FindRemoteIndexFileNamesForDateRange(const bg::date& begin_date, const bg::date& end_date)
{
	start_date_ = this->CheckDate(begin_date);
	end_date_ = this->CheckDate(end_date);

	poco_debug(the_logger_, "D: Looking for Daily Index Files in date range from: " + bg::to_simple_string(start_date_)  + " to: " + bg::to_simple_string(end_date_));

	decltype(auto) directory_list = this->GetRemoteIndexList(begin_date);

	decltype(auto) looking_for_start = std::string{"form."} + bg::to_iso_string(start_date_) + ".idx";
	decltype(auto) looking_for_end = std::string{"form."} + bg::to_iso_string(end_date_) + ".idx";

	std::vector<std::string> found_files;

	// index files may or may not be gzipped, so we need to exclude possible file name suffix from comparisons

	std::copy_if(directory_list.crbegin(), directory_list.crend(), std::back_inserter(found_files),
			[&looking_for_start, &looking_for_end](const std::string& elem)
			{ return (elem.compare(0, looking_for_end.size(), looking_for_end) <= 0
			 	&& elem.compare(0, looking_for_start.size(), looking_for_start) >= 0); });

	poco_assert_msg(! found_files.empty(), ("Can't find daily index files for date range: "
		   	+ bg::to_simple_string(start_date_) + " " + bg::to_simple_string(end_date_)).c_str());

	actual_start_date_ = bg::from_undelimited_string(found_files.back().substr(5, 8));
	actual_end_date_ = bg::from_undelimited_string(found_files.front().substr(5, 8));

	poco_debug(the_logger_, "D: Found " + std::to_string(found_files.size()) + " files for date range.");

	// last thing...prepend directory name

	remote_daily_index_file_name_list_.clear();
	std::transform(found_files.cbegin(), found_files.cend(), std::back_inserter(remote_daily_index_file_name_list_),
		[this](const auto& e){ auto fpath{remote_index_file_directory_}; return fpath /= e; });

	return remote_daily_index_file_name_list_;
}		// -----  end of method DailyIndexFileRetriever::FindIndexFileNamesForDateRange  -----


std::vector<std::string> DailyIndexFileRetriever::GetRemoteIndexList (const bg::date& day_in_quarter)
{
	auto remote_directory = this->MakeDailyIndexPathName(day_in_quarter);

	decltype(auto) directory_list = the_server_.ListDirectoryContents(remote_directory);

	//	we need to do some cleanup of the directory listing to simplify our searches.

	decltype(auto) not_form = std::partition(directory_list.begin(), directory_list.end(),
			[](std::string& x) {return boost::algorithm::starts_with(x, "form");});
	directory_list.erase(not_form, directory_list.end());

	std::sort(directory_list.begin(), directory_list.end());

	return directory_list;
}		// -----  end of method DailyIndexFileRetriever::GetRemoteIndexList  -----

fs::path DailyIndexFileRetriever::CopyRemoteIndexFileTo (const fs::path& local_directory_name, bool replace_files)
{
	poco_assert_msg(! remote_daily_index_file_name_.empty(), "Must locate remote index file before attempting download.");

	local_daily_index_file_name_= local_directory_name;
	local_daily_index_file_name_ /= remote_daily_index_file_name_.leaf();

	if (local_daily_index_file_name_.extension() == ".gz")
		local_daily_index_file_name_.replace_extension("");

	if (! replace_files && fs::exists(local_daily_index_file_name_))
	{
		poco_debug(the_logger_, "D: File exists and 'replace' is false: skipping download: " + local_daily_index_file_name_.leaf().string());
		return local_daily_index_file_name_;
	}

	fs::create_directories(local_directory_name);

	the_server_.DownloadFile(remote_daily_index_file_name_, local_daily_index_file_name_);

	poco_debug(the_logger_, "D: Retrieved remote daily index file: " + remote_daily_index_file_name_.string() + " to: " + local_daily_index_file_name_.string());

	return local_daily_index_file_name_;

}		// -----  end of method DailyIndexFileRetriever::RetrieveIndexFile  -----


fs::path DailyIndexFileRetriever::HierarchicalCopyRemoteIndexFileTo (const fs::path& local_directory_prefix, bool replace_files)
{
	poco_assert_msg(! remote_daily_index_file_name_.empty(), "Must locate remote index file before attempting download.");

	this->MakeLocalIndexFilePath(local_directory_prefix);

	if (local_daily_index_file_name_.extension() == ".gz")
		local_daily_index_file_name_.replace_extension("");

	local_daily_index_file_directory_ = local_daily_index_file_name_.parent_path();
	fs::create_directories(local_daily_index_file_directory_);

	if (! replace_files && fs::exists(local_daily_index_file_name_))
	{
		poco_debug(the_logger_, "D: File exists and 'replace' is false: skipping download: " + local_daily_index_file_name_.leaf().string());
		return local_daily_index_file_name_;
	}

	the_server_.DownloadFile(remote_daily_index_file_name_, local_daily_index_file_name_);

	poco_debug(the_logger_, "D: Retrieved remote daily index file: " + remote_daily_index_file_name_.string() + " to: " + local_daily_index_file_name_.string());

	return local_daily_index_file_name_;

}		// -----  end of method DailyIndexFileRetriever::RetrieveIndexFile  -----


std::vector<fs::path> DailyIndexFileRetriever::CopyIndexFilesForDateRangeTo (const fs::path& local_directory_name, bool replace_files)
{
	poco_assert_msg(! remote_daily_index_file_name_list_.empty(), "Must generate list of remote index files before attempting download.");

	std::vector<fs::path> results;

	for (const auto& remote_index_name : remote_daily_index_file_name_list_)
	{
		remote_daily_index_file_name_ = remote_index_name;
		auto local_file = CopyRemoteIndexFileTo(local_directory_name, replace_files);
		results.push_back(local_file);
	}
	return results;
}		// -----  end of method DailyIndexFileRetriever::CopyIndexFilesForDateRangeTo  -----

std::vector<fs::path> DailyIndexFileRetriever::HierarchicalCopyIndexFilesForDateRangeTo (const fs::path& local_directory_name, bool replace_files)
{
	poco_assert_msg(! remote_daily_index_file_name_list_.empty(), "Must generate list of remote index files before attempting download.");

	std::vector<fs::path> results;

	for (const auto& remote_index_name : remote_daily_index_file_name_list_)
	{
		remote_daily_index_file_name_ = remote_index_name;
		auto local_file = HierarchicalCopyRemoteIndexFileTo(local_directory_name, replace_files);
		results.push_back(local_file);
	}
	return results;
}		// -----  end of method DailyIndexFileRetriever::CopyIndexFilesForDateRangeTo  -----

void DailyIndexFileRetriever::MakeLocalIndexFilePath (const fs::path& local_prefix)
{
	// want keep the directory hierarchy the same as on the remote system
	// EXCCEPT for the remote system prefix (which is given to our ctor)

	// we will assume there is not trailing delimiter on the stored remote prefix.
	// (even though we have no edit to enforce that for now.)

	std::string remote_index_name = boost::algorithm::replace_first_copy(remote_daily_index_file_name_.string(), remote_directory_prefix_.string(), "");
	local_daily_index_file_name_= local_prefix;
	local_daily_index_file_name_ /= remote_index_name;

}		// -----  end of method DailyIndexFileRetriever::MakeLocalIndexFileName  -----
