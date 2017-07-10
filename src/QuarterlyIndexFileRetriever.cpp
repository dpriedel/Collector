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

fs::path QuarterlyIndexFileRetriever::MakeQuarterIndexPathName (const bg::date& day_in_quarter)
{
	input_date_ = CheckDate(day_in_quarter);

	PathNameGenerator p_gen{remote_directory_prefix_, day_in_quarter, day_in_quarter};

	remote_quarterly_index_file_name_ = *p_gen;
	remote_quarterly_index_file_name_ /= "form.zip";		// we know this.

	return remote_quarterly_index_file_name_;

}		// -----  end of method QuarterlyIndexFileRetriever::MakeQuarterIndexPathName  -----


void QuarterlyIndexFileRetriever::HierarchicalCopyRemoteIndexFileTo (const fs::path& local_directory_name, bool replace_files)
{
	poco_assert_msg(! remote_quarterly_index_file_name_.empty(), "Must generate remote index file name before attempting download.");

	this->MakeLocalIndexFilePath(local_directory_name);
	local_quarterly_index_file_directory_ = local_quarterly_index_file_name_.parent_path();
	fs::create_directories(local_quarterly_index_file_directory_);

	if (! replace_files && fs::exists(local_quarterly_index_file_name_))
	{
		poco_debug(the_logger_, "Q: File exists and 'replace' is false: skipping download: " + local_quarterly_index_file_name_.leaf().string());
		return;
	}

	the_server_.DownloadFile(remote_quarterly_index_file_name_, local_quarterly_index_file_name_);

	poco_debug(the_logger_, "Q: Retrieved remote quarterly index file: " + remote_quarterly_index_file_name_.string() +
		" to: " + local_quarterly_index_file_name_.string());
}		// -----  end of method QuarterlyIndexFileRetriever::CopyRemoteIndexFileTo  -----


void QuarterlyIndexFileRetriever::MakeLocalIndexFilePath (const fs::path& local_prefix)
{
	// want keep the directory hierarchy the same as on the remote system
	// EXCCEPT for the remote system prefix (which is given to our ctor)

	// we will assume there is not trailing delimiter on the stored remote prefix.
	// (even though we have no edit to enforce that for now.)

	std::string remote_index_name = boost::algorithm::replace_first_copy(remote_quarterly_index_file_name_.string(), remote_directory_prefix_.string(), "");
	local_quarterly_index_file_name_= local_prefix;
	local_quarterly_index_file_name_ /= remote_index_name;
	local_quarterly_index_file_name_.replace_extension("idx");

}		// -----  end of method QuarterlyIndexFileRetriever::MakeLocalIndexFilePath  -----


const std::vector<fs::path>& QuarterlyIndexFileRetriever::MakeIndexFileNamesForDateRange(const bg::date& begin_date, const bg::date& end_date)
{
	start_date_ = this->CheckDate(begin_date);
	end_date_ = this->CheckDate(end_date);

	remote_quarterly_index_zip_file_name_list_ = this->GetRemoteIndexList();

	//	we need to keep a copy of these file names for the unzipped files which will end up locally.
	// (mainly for testing purposes)

	local_quarterly_index_file_name_list_.clear();

	//	NOTE: elem is passed by value since the replace algorith modifies in place

	std::transform(remote_quarterly_index_zip_file_name_list_.cbegin(), remote_quarterly_index_zip_file_name_list_.cend(),
			std::back_inserter(local_quarterly_index_file_name_list_),
				[](fs::path elem) { elem.replace_extension(".idx"); return elem; });

	poco_debug(the_logger_, "Q: Found " + std::to_string(remote_quarterly_index_zip_file_name_list_.size()) + " files for date range.");

	return remote_quarterly_index_zip_file_name_list_;
}		// -----  end of method DailyIndexFileRetriever::FindIndexFileNamesForDateRange  -----

std::vector<fs::path> QuarterlyIndexFileRetriever::GetRemoteIndexList (void)
{
	std::vector<fs::path> results;

	PathNameGenerator p_gen{remote_directory_prefix_, start_date_, end_date_};
	PathNameGenerator p_end;

	for (; p_gen != p_end; ++p_gen)
	{
		auto remote_file_name = *p_gen;
		remote_file_name /= "form.zip";
		results.push_back(remote_file_name);
	}

	return results;
}		// -----  end of method QuarterlyIndexFileRetriever::GetRemoteIndexList  -----

void QuarterlyIndexFileRetriever::HierarchicalCopyIndexFilesForDateRangeTo (const fs::path& local_directory_name, bool replace_files)
{
	poco_assert_msg(! remote_quarterly_index_zip_file_name_list_.empty(), "Must generate list of remote index files before attempting download.");

	//	Remember...we are working with compressed directory files on the EDGAR server

	for (const auto& remote_file : remote_quarterly_index_zip_file_name_list_)
	{
		remote_quarterly_index_file_name_ = remote_file;
		HierarchicalCopyRemoteIndexFileTo(local_directory_name, replace_files);
	}
}		// -----  end of method QuarterlyIndexFileRetriever::CopyIndexFilesForDateRangeTo  -----
