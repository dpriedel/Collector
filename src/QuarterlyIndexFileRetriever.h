// =====================================================================================
//
//       Filename:  QuarterlyIndexFileRetriever.h
//
//    Description:  Header for class which knows how to retrieve EDGAR quarterly index files
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
//        Class:  QuarterlyIndexFileRetriever
//  Description:
// =====================================================================================
class QuarterlyIndexFileRetriever
{
	public:
		// ====================  LIFECYCLE     =======================================
		QuarterlyIndexFileRetriever ()=delete;
		QuarterlyIndexFileRetriever (HTTPS_Downloader& a_server, const fs::path& prefix, Poco::Logger& the_logger);                // constructor

		// ====================  ACCESSORS     =======================================

		const fs::path& GetLocalIndexFilePath(void) const { return local_quarterly_index_file_name_; }
		const std::vector<fs::path>& GetfRemoteIndexFileNamesForDateRange(void) const { return remote_quarterly_index_zip_file_name_list_; }

		//	this method provides file names ending in .idx, not .zip as above
		const std::vector<fs::path>& GetLocalIndexFileNamesForDateRange(void) const { return local_quarterly_index_file_name_list_; }

		// ====================  MUTATORS      =======================================

		fs::path MakeQuarterIndexPathName(const bg::date& day_in_quarter);
		void RetrieveRemoteIndexFileTo(const fs::path& local_directory_name, bool replace_files=false);

		//	This method treats the date range as a closed interval.

		const std::vector<fs::path>& FindIndexFileNamesForDateRange(const bg::date& start_date, const bg::date& end_date);
		void RetrieveIndexFilesForDateRangeTo(const fs::path& local_directory_name, bool replace_files=false);

		// ====================  OPERATORS     =======================================

	protected:

		bg::date CheckDate(const bg::date& aDate);
		void MakeLocalIndexFilePath(const fs::path& local_prefix);
		std::vector<fs::path> GetRemoteIndexList(void);

		// ====================  DATA MEMBERS  =======================================

	private:

		// ====================  DATA MEMBERS  =======================================

		HTTPS_Downloader& the_server_;
		fs::path remote_quarterly_index_file_name_;
		std::vector<fs::path> remote_quarterly_index_zip_file_name_list_;
		std::vector<fs::path> local_quarterly_index_file_name_list_;
        fs::path remote_directory_prefix_;                // top-level directory path
		fs::path local_quarterly_index_file_name_;
		fs::path local_quarterly_index_file_name_zip_;
		fs::path local_quarterly_index_file_directory_;
		bg::date input_date_;
		bg::date start_date_;
		bg::date end_date_;
        Poco::Logger& the_logger_;

}; // -----  end of class QuarterlyIndexFileRetriever  -----

#endif /* QUARTERLYINDEXFILERETRIEVER_H_ */
