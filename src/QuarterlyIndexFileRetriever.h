// =====================================================================================
//
//       Filename:  QuarterlyIndexFileRetriever.h
//
//    Description:  Header for class which knows how to retrieve SEC quarterly index files
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


#include <string>
#include <vector>
#include <filesystem>

// #include <boost/filesystem.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include "Poco/Logger.h"

namespace bg = boost::gregorian;
namespace fs = std::filesystem;

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

		// ====================  MUTATORS      =======================================

		fs::path MakeQuarterlyIndexPathName(const bg::date& day_in_quarter);
		fs::path HierarchicalCopyRemoteIndexFileTo(const fs::path& remote_file_name, const fs::path& local_directory_name, bool replace_files=false);

		//	This method treats the date range as a closed interval.

		const std::vector<fs::path> MakeIndexFileNamesForDateRange(const bg::date& start_date, const bg::date& end_date);
		std::vector<fs::path> HierarchicalCopyIndexFilesForDateRangeTo(const std::vector<fs::path>& remote_file_list, const fs::path& local_directory_name, bool replace_files=false);
		std::vector<fs::path> ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo(const std::vector<fs::path>& remote_file_list, const fs::path& local_directory_name, int max_at_a_time, bool replace_files=false);

		// ====================  OPERATORS     =======================================

	protected:

		bg::date CheckDate(const bg::date& aDate);
		fs::path MakeLocalIndexFilePath(const fs::path& local_prefix, const fs::path& remote_quarterly_index_file_name);

		// ====================  DATA MEMBERS  =======================================

	private:

		auto AddToCopyList(const fs::path& local_directory_name, bool replace_files);

		// ====================  DATA MEMBERS  =======================================

		HTTPS_Downloader& the_server_;
        fs::path remote_directory_prefix_;                // top-level directory path
		bg::date input_date_;
		bg::date start_date_;
		bg::date end_date_;
        Poco::Logger& the_logger_;

}; // -----  end of class QuarterlyIndexFileRetriever  -----

#endif /* QUARTERLYINDEXFILERETRIEVER_H_ */
