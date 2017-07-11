// =====================================================================================
//
//       Filename:  FormFileRetriever.h
//
//    Description:  Header for class which finds entries for specified form type.
//
//        Version:  1.0
//        Created:  01/13/2014 10:10:18 AM
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


#ifndef FORMRETRIEVER_H_
#define FORMRETRIEVER_H_

#include <chrono>
#include <map>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include "Poco/Logger.h"

namespace fs = boost::filesystem;


#include "HTTPS_Downloader.h"

// =====================================================================================
//        Class:  FormFileRetriever
//  Description:
// =====================================================================================
class FormFileRetriever
{
	public:

		using FormsList = std::map<std::string, std::vector<std::string>>;

		// ====================  LIFECYCLE     =======================================

		FormFileRetriever (HTTPS_Downloader& a_server, Poco::Logger& the_logger, int pause=1);                   // constructor

		// ====================  ACCESSORS     =======================================

		// ====================  MUTATORS      =======================================

		// ====================  OPERATORS     =======================================

		FormsList FindFilesForForms(const std::vector<std::string>& the_forms,
				const fs::path& local_index_file_name, const std::map<std::string, std::string>& ticker_map={});

		FormsList FindFilesForForms(const std::vector<std::string>& the_forms, const fs::path& local_index_file_dir,
				const std::vector<fs::path>& local_index_file_list, const std::map<std::string, std::string>& ticker_map={});

		// NOTE: the retrieved files will be placed in a directory hierarchy as follows:
		// <form_directory>/<the_form>/<CIK number>/<file name>

		void RetrieveSpecifiedFiles(const FormsList& form_list,
				const fs::path& local_form_directory, bool replace_files=false);

	protected:

		void RetrieveSpecifiedFiles(const std::vector<std::string>& form_file_list, const std::string& the_form,
				const fs::path& local_form_directory, bool replace_files=false);

		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================

		static constexpr std::string::size_type k_index_CIK_offset = 74;

		HTTPS_Downloader& the_server_;
		fs::path local_form_directory_name_;

        Poco::Logger& the_logger_;
		std::chrono::seconds pause_;


}; // -----  end of class FormFileRetriever  -----

#endif /* FORMRETRIEVER_H_ */
