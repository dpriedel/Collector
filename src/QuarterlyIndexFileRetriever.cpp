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

//--------------------------------------------------------------------------------------
//       Class:  QuarterlyIndexFileRetriever
//      Method:  QuarterlyIndexFileRetriever
// Description:  constructor
//--------------------------------------------------------------------------------------
QuarterlyIndexFileRetriever::QuarterlyIndexFileRetriever (HTTPS_Downloader& a_server, Poco::Logger& the_logger)
	: the_server_{a_server}, the_logger_{the_logger}
{
}  // -----  end of method QuarterlyIndexFileRetriever::QuarterlyIndexFileRetriever  (constructor)  -----


bg::date QuarterlyIndexFileRetriever::UseDate (const bg::date& day_in_quarter)
{
	input_date_ = bg::date();		//	don't know of a better way to clear date field.

	//	we can only work with past data.

	bg::date today{bg::day_clock::local_day()};
	poco_assert_msg(day_in_quarter < today, ("Date must be less than " + bg::to_simple_string(today)).c_str());

	return day_in_quarter;
}		// -----  end of method QuarterlyIndexFileRetriever::UseDate  -----

std::string QuarterlyIndexFileRetriever::MakeQuarterIndexPathName (const bg::date& day_in_quarter)
{
	input_date_ = UseDate(day_in_quarter);

	PathNameGenerator p_gen{day_in_quarter, day_in_quarter};

	remote_quarterly_index_file_name_ = *p_gen;

	return remote_quarterly_index_file_name_;

}		// -----  end of method QuarterlyIndexFileRetriever::MakeQuarterIndexPathName  -----


void QuarterlyIndexFileRetriever::RetrieveRemoteIndexFileTo (const fs::path& local_directory_name, bool replace_files)
{
	poco_assert_msg(! remote_quarterly_index_file_name_.empty(), "Must generate remote index file name before attempting download.");

	local_quarterly_index_file_directory_ = local_directory_name;
	this->MakeLocalIndexFilePath();
	fs::create_directories(local_quarterly_index_file_name_zip_.parent_path());

	if (! replace_files && fs::exists(local_quarterly_index_file_name_))
		return;

	// ftp_server_.OpenFTPConnection();
	// ftp_server_.ChangeWorkingDirectoryTo("edgar/full-index");
	//
	// ftp_server_.DownloadBinaryFile(remote_quarterly_index_file_name_, local_quarterly_index_file_name_zip_);
	//
	// ftp_server_.CloseFTPConnection();
	//
	// UnzipLocalIndexFile(local_quarterly_index_file_name_zip_);
	//
	// fs::remove(local_quarterly_index_file_name_zip_);
	//
	// poco_debug(the_logger_, "Q: Retrieved remote quarterly index file: " + remote_quarterly_index_file_name_ +
	// 	" to: " + local_quarterly_index_file_name_.string());
}		// -----  end of method QuarterlyIndexFileRetriever::RetrieveRemoteIndexFileTo  -----


void QuarterlyIndexFileRetriever::MakeLocalIndexFilePath (void)
{
	local_quarterly_index_file_name_zip_ = local_quarterly_index_file_directory_;
	local_quarterly_index_file_name_zip_ /= remote_quarterly_index_file_name_;

	local_quarterly_index_file_name_ = local_quarterly_index_file_name_zip_;
	local_quarterly_index_file_name_.replace_extension("idx");

}		// -----  end of method QuarterlyIndexFileRetriever::MakeLocalIndexFilePath  -----


void QuarterlyIndexFileRetriever::UnzipLocalIndexFile (const fs::path& local_zip_file_name)
{
	std::ifstream zipped_file(local_zip_file_name.string(), std::ios::in | std::ios::binary);
	Poco::Zip::Decompress expander(zipped_file, local_zip_file_name.parent_path().string(), true);

	expander.decompressAllFiles();

}		// -----  end of method QuarterlyIndexFileRetriever::UnzipLocalIndexFile  -----

const std::vector<std::string>& QuarterlyIndexFileRetriever::FindIndexFileNamesForDateRange(const bg::date& begin_date, const bg::date& end_date)
{
	start_date_ = this->UseDate(begin_date);
	end_date_ = this->UseDate(end_date);

	remote_quarterly_index_zip_file_name_list_ = this->GetRemoteIndexList();

	//	we need to keep a copy of these file names for the unzipped files which will end up locally.

	local_quarterly_index_file_name_list_.clear();

	//	NOTE: elem is passed by value since the replace algorith modifies in place

	std::transform(remote_quarterly_index_zip_file_name_list_.cbegin(), remote_quarterly_index_zip_file_name_list_.cend(),
			std::back_inserter(local_quarterly_index_file_name_list_),
				[](std::string elem) { boost::algorithm::replace_last(elem, ".zip", ".idx"); return elem; });

	poco_debug(the_logger_, "Q: Found " + std::to_string(remote_quarterly_index_zip_file_name_list_.size()) + " files for date range.");

	return remote_quarterly_index_zip_file_name_list_;
}		// -----  end of method DailyIndexFileRetriever::FindIndexFileNamesForDateRange  -----

std::vector<std::string> QuarterlyIndexFileRetriever::GetRemoteIndexList (void)
{
	std::vector<std::string> results;

	PathNameGenerator p_gen{start_date_, end_date_};
	PathNameGenerator p_end;

	for (; p_gen != p_end; ++p_gen)
	{
		results.push_back(*p_gen);
	}

	return results;
}		// -----  end of method QuarterlyIndexFileRetriever::GetRemoteIndexList  -----

void QuarterlyIndexFileRetriever::RetrieveIndexFilesForDateRangeTo (const fs::path& local_directory_name, bool replace_files)
{
	poco_assert_msg(! remote_quarterly_index_zip_file_name_list_.empty(), "Must generate list of remote index files before attempting download.");

	//	Remember...we are working with compressed directory files on the EDGAR server

	local_quarterly_index_file_directory_ = local_directory_name;

	// ftp_server_.OpenFTPConnection();
	// ftp_server_.ChangeWorkingDirectoryTo("edgar/full-index");
	//
	// for (const auto& remote_file : remote_quarterly_index_zip_file_name_list_)
	// {
	// 	decltype(auto) local_quarterly_index_file_name_zip = local_quarterly_index_file_directory_;
	// 	local_quarterly_index_file_name_zip /= remote_file;
	//
	// 	fs::create_directories(local_quarterly_index_file_name_zip.parent_path());
	//
	// 	fs::path local_file_name{local_quarterly_index_file_name_zip};
	// 	local_file_name.replace_extension("idx");
	//
	// 	if (replace_files || ! fs::exists(local_file_name))
	// 	{
	// 		ftp_server_.DownloadBinaryFile(remote_file, local_quarterly_index_file_name_zip);
	// 		UnzipLocalIndexFile(local_quarterly_index_file_name_zip);
	// 		fs::remove(local_quarterly_index_file_name_zip);
	//
	// 		poco_debug(the_logger_, "Q: Retrieved remote quarterly index file: " + remote_file + " to: " + local_file_name.string());
	// 	}
	// }
	//
	// ftp_server_.CloseFTPConnection();
}		// -----  end of method QuarterlyIndexFileRetriever::RetrieveIndexFilesForDateRangeTo  -----


//--------------------------------------------------------------------------------------
//       Class:  QuarterlyIndexFileRetriever::PathNameGenerator
//      Method:  QuarterlyIndexFileRetriever::PathNameGenerator
// Description:  constructor
//--------------------------------------------------------------------------------------

//	NOTE:	The range validity edits on greg_year and greg_month don't allow you to use
//			meaningful default values.

QuarterlyIndexFileRetriever::PathNameGenerator::PathNameGenerator (void)
	: start_year_{2000}, end_year_{2000}, active_year_{2000}, start_month_{1}, end_month_{1}, active_month_{1}
{

}  // -----  end of method QuarterlyIndexFileRetriever::PathNameGenerator  (constructor)  -----


//--------------------------------------------------------------------------------------
//       Class:  QuarterlyIndexFileRetriever::PathNameGenerator
//      Method:  QuarterlyIndexFileRetriever::PathNameGenerator
// Description:  constructor
//--------------------------------------------------------------------------------------
QuarterlyIndexFileRetriever::PathNameGenerator::PathNameGenerator (const bg::date& start_date, const bg::date& end_date)
	: start_date_{start_date}, end_date_{end_date}, active_date_{start_date},
		start_year_{start_date.year()}, end_year_{end_date.year()}, active_year_{start_date.year()},
	    start_month_{start_date.month()}, end_month_{end_date.month()}, active_month_{start_date.month()}
{
	fs::path EDGAR_path{std::to_string(active_year_)};
	EDGAR_path /= "QTR" + std::to_string(active_month_ / 3 + (active_month_ % 3 == 0 ? 0 : 1));
	EDGAR_path /= "form.zip";

	path_ = EDGAR_path.string();

}  // -----  end of method QuarterlyIndexFileRetriever::PathNameGenerator::QuarterlyIndexFileRetriever::PathNameGenerator  (constructor)  -----


void QuarterlyIndexFileRetriever::PathNameGenerator::increment (void)
{
	bg::months a_quarter{3};
	active_date_ += a_quarter;

	if (active_date_ > end_date_)
	{
		//	we need to become equal to and 'end' iterator

		active_year_ = start_year_;
		active_month_ = start_month_;
		path_.clear();
	}
	else
	{
		active_year_ = active_date_.year();
		active_month_ = active_date_.month();

		fs::path EDGAR_path{std::to_string(active_year_)};
		EDGAR_path /= "QTR" + std::to_string(active_month_ / 3 + (active_month_ % 3 == 0 ? 0 : 1));
		EDGAR_path /= "form.zip";

		path_ = EDGAR_path.string();
	}
}		// -----  end of method PathNameGenerator::increment  -----
