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

//--------------------------------------------------------------------------------------
//       Class:  DailyIndexFileRetriever
//      Method:  DailyIndexFileRetriever
// Description:  constructor
//--------------------------------------------------------------------------------------

DailyIndexFileRetriever::DailyIndexFileRetriever(const FTP_Server& ftp_server)
	: ftp_server_{ftp_server}
{

}  // -----  end of method DailyIndexFileRetriever::DailyIndexFileRetriever  (constructor)  -----


DailyIndexFileRetriever::~DailyIndexFileRetriever(void)
{

}		// -----  end of method IndexFileRetreiver::~DailyIndexFileRetriever  -----


bg::date DailyIndexFileRetriever::UseDate (const bg::date& aDate)
{
	input_date_ = bg::date();		//	don't know of a better way to clear date field.

	//	we can only work with past data.

	bg::date today{bg::day_clock::local_day()};
	poco_assert_msg(aDate <= today, ("Date must be less than " + bg::to_simple_string(today)).c_str());

	return aDate;
}		// -----  end of method DailyIndexFileRetriever::UseDate  -----


std::string DailyIndexFileRetriever::FindIndexFileNameNearestDate(const bg::date& aDate)
{
	input_date_ = this->UseDate(aDate);

	std::clog << "D: Looking for Daily Index File nearest date: " << aDate << '\n';

	decltype(auto) directory_list = this->GetRemoteIndexList();

	std::string looking_for = std::string{"form."} + bg::to_iso_string(input_date_) + ".idx";

	decltype(auto) pos = std::find_if(directory_list.crbegin(), directory_list.crend(),
        [&looking_for](const auto& x) {return x <= looking_for;});
	poco_assert_msg(pos != directory_list.rend(), ("Can't find daily index file for date: " + bg::to_simple_string(input_date_)).c_str());

	remote_daily_index_file_name_ = *pos;
	actual_file_date_ = bg::from_undelimited_string(remote_daily_index_file_name_.substr(5, 8));

	std::clog << "D: Found Daily Index File for date: " << actual_file_date_ << '\n';

	return remote_daily_index_file_name_;
}		// -----  end of method DailyIndexFileRetriever::FindIndexFileDateNearest  -----


const std::vector<std::string>& DailyIndexFileRetriever::FindIndexFileNamesForDateRange(const bg::date& begin_date, const bg::date& end_date)
{
	start_date_ = this->UseDate(begin_date);
	end_date_ = this->UseDate(end_date);

	std::clog << "D: Looking for Daily Index Files in date range from: " << start_date_  << " to: " << end_date_ << '\n';

	decltype(auto) directory_list = this->GetRemoteIndexList();

	decltype(auto) looking_for_start = std::string{"form."} + bg::to_iso_string(start_date_) + ".idx";
	decltype(auto) looking_for_end = std::string{"form."} + bg::to_iso_string(end_date_) + ".idx";

	remote_daily_index_file_name_list_.clear();
	std::copy_if(directory_list.crbegin(), directory_list.crend(), std::back_inserter(remote_daily_index_file_name_list_),
			[&looking_for_start, &looking_for_end](const std::string& elem)
			{ return (elem <= looking_for_end && elem >= looking_for_start); });

	poco_assert_msg(! remote_daily_index_file_name_list_.empty(), ("Can't find daily index files for date range: "
		   	+ bg::to_simple_string(start_date_) + bg::to_simple_string(end_date_)).c_str());

	actual_start_date_ = bg::from_undelimited_string(remote_daily_index_file_name_list_.back().substr(5, 8));
	actual_end_date_ = bg::from_undelimited_string(remote_daily_index_file_name_list_.front().substr(5, 8));

	std::clog << "D: Found " << remote_daily_index_file_name_list_.size() << " files for date range.\n";

	return remote_daily_index_file_name_list_;
}		// -----  end of method DailyIndexFileRetriever::FindIndexFileNamesForDateRange  -----


std::vector<std::string> DailyIndexFileRetriever::GetRemoteIndexList (void)
{
	ftp_server_.OpenFTPConnection();
	ftp_server_.ChangeWorkingDirectoryTo("edgar/daily-index");

	decltype(auto) directory_list = ftp_server_.ListWorkingDirectoryContents();

	ftp_server_.CloseFTPConnection();

	//	we need to do some cleanup of the directory listing to simplify our searches.

	decltype(auto) not_form = std::partition(directory_list.begin(), directory_list.end(),
			[](std::string& x) {return boost::algorithm::starts_with(x, "form");});
	directory_list.erase(not_form, directory_list.end());

	std::sort(directory_list.begin(), directory_list.end());

	return directory_list;
}		// -----  end of method DailyIndexFileRetriever::GetRemoteIndexList  -----

void DailyIndexFileRetriever::RetrieveRemoteIndexFileTo (const fs::path& local_directory_name, bool replace_files)
{
	poco_assert_msg(! remote_daily_index_file_name_.empty(), "Must locate remote index file before attempting download.");

	fs::create_directories(local_directory_name);
	local_daily_index_file_directory_ = local_directory_name;
	this->MakeLocalIndexFilePath();

	if (! replace_files && fs::exists(local_daily_index_file_name_))
		return;

	ftp_server_.OpenFTPConnection();
	ftp_server_.ChangeWorkingDirectoryTo("edgar/daily-index");

	ftp_server_.DownloadFile(remote_daily_index_file_name_, local_daily_index_file_name_);

	ftp_server_.CloseFTPConnection();

	std::clog << "D: Retrieved remote daily index file: " << remote_daily_index_file_name_ << " to: " << local_daily_index_file_name_ << '\n';

}		// -----  end of method DailyIndexFileRetriever::RetrieveIndexFile  -----


void DailyIndexFileRetriever::RetrieveIndexFilesForDateRangeTo (const fs::path& local_directory_name, bool replace_files)
{
	poco_assert_msg(! remote_daily_index_file_name_list_.empty(), "Must generate list of remote index files before attempting download.");

	fs::create_directories(local_directory_name);
	local_daily_index_file_directory_ = local_directory_name;

	ftp_server_.OpenFTPConnection();
	ftp_server_.ChangeWorkingDirectoryTo("edgar/daily-index");

	for (const auto& remote_file : remote_daily_index_file_name_list_)
	{
		decltype(auto) local_file_name = local_daily_index_file_directory_;
		local_file_name /= remote_file;
		if (replace_files || ! fs::exists(local_file_name))
		{
			ftp_server_.DownloadFile(remote_file, local_file_name);
			std::clog << "D: Retrieved remote daily index file: " << remote_file << " to: " << local_file_name << '\n';
		}
	}

	ftp_server_.CloseFTPConnection();
}		// -----  end of method DailyIndexFileRetriever::RetrieveIndexFilesForDateRangeTo  -----

void DailyIndexFileRetriever::MakeLocalIndexFilePath (void)
{
	local_daily_index_file_name_ = local_daily_index_file_directory_;
	local_daily_index_file_name_ /= remote_daily_index_file_name_;


}		// -----  end of method DailyIndexFileRetriever::MakeLocalIndexFileName  -----
