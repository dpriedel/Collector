// =====================================================================================
//
//       Filename:  DailyIndexFileRetriever.h
//
//    Description:  module to retrieve EDGAR daily index file for date
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


#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include "Poco/Logger.h"

namespace bg = boost::gregorian;
namespace fs = boost::filesystem;

#include "HTTPS_Downloader.h"

// =====================================================================================
//        Class:  DailyIndexFileRetriever
//  Description:
// =====================================================================================

class DailyIndexFileRetriever
{
	public:
		// ====================  LIFECYCLE     =======================================
		DailyIndexFileRetriever(void) = delete;
		DailyIndexFileRetriever (HTTPS_Downloader& a_server, const fs::path& prefix, Poco::Logger& the_logger);

		~DailyIndexFileRetriever(void);

		// ====================  ACCESSORS     =======================================

		const std::string& GetRemoteIndexFileName (void) const { return remote_daily_index_file_name_; }
		const fs::path& GetLocalIndexFilePath(void) const { return local_daily_index_file_name_; }
		const bg::date& GetActualIndexFileDate(void) const { return actual_file_date_; }
		const std::vector<std::string>& GetfRemoteIndexFileNamesForDateRange(void) const { return remote_daily_index_file_name_list_; }
		std::pair<bg::date, bg::date> GetActualDateRange(void) const { return std::make_pair(actual_start_date_, actual_end_date_); }

		// ====================  MUTATORS      =======================================

		//	If there is no file for the specified date, this function will return the
		//	immediately prior file.

		std::string FindIndexFileNameNearestDate(const bg::date& aDate);
		void RetrieveRemoteIndexFileTo(const fs::path& local_directory_name, bool replace_files=false);

		//	This method treats the date range as a closed interval.

		const std::vector<std::string>& FindIndexFileNamesForDateRange(const bg::date& start_date, const bg::date& end_date);
		void RetrieveIndexFilesForDateRangeTo(const fs::path& local_directory_name, bool replace_files=false);

		// daily files are now organized in a directory hierarchy the same as quarterly index files.

		fs::path MakeDailyIndexPathName(const bg::date& day_in_quarter);

		// ====================  OPERATORS     =======================================

	protected:

		bg::date UseDate(const bg::date& aDate);
		void MakeLocalIndexFilePath(void);
		std::vector<std::string> GetRemoteIndexList(const bg::date& day_in_quarter);

		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================

		HTTPS_Downloader& the_server_;
		std::string remote_daily_index_file_name_;
		std::vector<std::string> remote_daily_index_file_name_list_;
        fs::path remote_file_directory_;                // top-level directory path
        fs::path remote_index_file_directory_;
		fs::path local_daily_index_file_directory_;
		fs::path local_daily_index_file_name_;
		bg::date input_date_;
		bg::date actual_file_date_;
		bg::date start_date_;
		bg::date end_date_;
		bg::date actual_start_date_;
		bg::date actual_end_date_;

        Poco::Logger& the_logger_;

}; // -----  end of class DailyIndexFileRetriever  -----





#endif /* DAILYINDEXFILERETRIEVER_H_ */
