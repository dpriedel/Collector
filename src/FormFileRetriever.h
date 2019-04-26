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


#ifndef FORMRETRIEVER_H_
#define FORMRETRIEVER_H_

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

#include "HTTPS_Downloader.h"
#include "TickerConverter.h"

// =====================================================================================
//        Class:  FormFileRetriever
//  Description:
// =====================================================================================
class FormFileRetriever
{
	public:

		using FormsAndFilesList = std::map<std::string, std::vector<fs::path>>;

		// ====================  LIFECYCLE     =======================================

		FormFileRetriever (const std::string& host, const std::string& port);            // constructor

		FormFileRetriever() = delete;
		FormFileRetriever(const FormFileRetriever& rhs) = delete;
		FormFileRetriever(FormFileRetriever&& rhs) = delete;

		// ====================  ACCESSORS     =======================================

		// ====================  MUTATORS      =======================================

        FormFileRetriever& operator=(const FormFileRetriever& rhs) = delete;
        FormFileRetriever& operator=(FormFileRetriever&& rhs) = delete;

		// ====================  OPERATORS     =======================================

		FormsAndFilesList FindFilesForForms(const std::vector<std::string>& the_form_types,
				const fs::path& local_index_file_name, const TickerConverter::TickerCIKMap& ticker_map={});

		FormsAndFilesList FindFilesForForms(const std::vector<std::string>& the_form_types,
				const std::vector<fs::path>& local_index_files, const TickerConverter::TickerCIKMap& ticker_map={});

		// NOTE: the retrieved files will be placed in a directory hierarchy as follows:
		// <form_directory>/<the_form_type>/<CIK number>/<file name>

		void RetrieveSpecifiedFiles(const FormsAndFilesList& form_list,
				const fs::path& local_form_directory, bool replace_files=false);

		void ConcurrentlyRetrieveSpecifiedFiles(const FormsAndFilesList& form_list,
				const fs::path& local_form_directory, int max_at_a_time, bool replace_files=false);

	protected:

		void RetrieveSpecifiedFiles(const std::vector<fs::path>& remote_file_names, const std::string& form_type,
				const fs::path& local_form_directory, bool replace_files=false);

		void ConcurrentlyRetrieveSpecifiedFiles(const std::vector<fs::path>& remote_file_names, const std::string& form_type,
				const fs::path& local_form_directory, int max_at_a_time, bool replace_files=false);

		// ====================  DATA MEMBERS  =======================================

	private:

		fs::path MakeLocalDirNameFromRemoteFileName(const fs::path& local_form_directory_name, const fs::path& remote_file_name, const std::string& form_name);

		auto ExtractFileName(int& found_a_form);

		auto AddToCopyList(const std::string& form_name, const fs::path& local_form_directory, bool replace_files);

		// ====================  DATA MEMBERS  =======================================

		static constexpr std::string::size_type k_index_CIK_offset = 74;

		HTTPS_Downloader the_server_;

}; // -----  end of class FormFileRetriever  -----

#endif /* FORMRETRIEVER_H_ */
