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

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include "Poco/Logger.h"

namespace bg = boost::gregorian;

#include "FTP_Connection.h"

// =====================================================================================
//        Class:  QuarterlyIndexFileRetriever
//  Description:
// =====================================================================================
class QuarterlyIndexFileRetriever
{
	public:
		// ====================  LIFECYCLE     =======================================
		QuarterlyIndexFileRetriever ()=delete;
		QuarterlyIndexFileRetriever (const FTP_Server& ftp_server, Poco::Logger& the_logger);                // constructor

		// ====================  ACCESSORS     =======================================

		const fs::path& GetLocalIndexFilePath(void) const { return local_quarterly_index_file_name_; }
		const std::vector<std::string>& GetfRemoteIndexFileNamesForDateRange(void) const { return remote_quarterly_index_zip_file_name_list_; }

		//	this method provides file names ending in .idx, not .zip as above
		const std::vector<std::string>& GetLocalIndexFileNamesForDateRange(void) const { return local_quarterly_index_file_name_list_; }

		// ====================  MUTATORS      =======================================

		std::string MakeQuarterIndexPathName(const bg::date& day_in_quarter);
		void RetrieveRemoteIndexFileTo(const fs::path& local_directory_name, bool replace_files=false);

		//	This method treats the date range as a closed interval.

		const std::vector<std::string>& FindIndexFileNamesForDateRange(const bg::date& start_date, const bg::date& end_date);
		void RetrieveIndexFilesForDateRangeTo(const fs::path& local_directory_name, bool replace_files=false);

		// ====================  OPERATORS     =======================================

	protected:

		bg::date UseDate(const bg::date& aDate);
		void MakeLocalIndexFilePath(void);
		void UnzipLocalIndexFile(const fs::path& local_zip_file_name);
		std::vector<std::string> GetRemoteIndexList(void);

		// ====================  DATA MEMBERS  =======================================

	private:

		//	a Quarterly Index path name generator
		//	we use an iterator but we are actually acting as a generator.

		class PathNameGenerator : public boost::iterator_facade
		<
			PathNameGenerator,
			std::string const,
			boost::forward_traversal_tag
		>
		{
			public:

				PathNameGenerator(void);
				PathNameGenerator(const bg::date& start_date, const bg::date& end_date);

			private:

				friend class boost::iterator_core_access;

				void increment();

				bool equal(PathNameGenerator const& other) const
				{
					return this->path_ == other.path_;
				}

				std::string const& dereference() const { return path_; }

				bg::date start_date_;
				bg::date end_date_;
				bg::date active_date_;

				bg::greg_year start_year_;
				bg::greg_year end_year_;
				bg::greg_year active_year_;
				bg::greg_month start_month_;
				bg::greg_month end_month_;
				bg::greg_month active_month_;

				std::string path_;
		};

		// ====================  DATA MEMBERS  =======================================

		FTP_Server ftp_server_;
		std::string remote_quarterly_index_file_name_;
		std::vector<std::string> remote_quarterly_index_zip_file_name_list_;
		std::vector<std::string> local_quarterly_index_file_name_list_;
		fs::path local_quarterly_index_file_name_;
		fs::path local_quarterly_index_file_name_zip_;
		fs::path local_quarterly_index_file_directory_;
		bg::date input_date_;
		bg::date start_date_;
		bg::date end_date_;
        Poco::Logger& the_logger_;

}; // -----  end of class QuarterlyIndexFileRetriever  -----

#endif /* QUARTERLYINDEXFILERETRIEVER_H_ */
