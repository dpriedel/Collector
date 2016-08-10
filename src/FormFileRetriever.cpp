// =====================================================================================
//
//       Filename:  FormFileRetriever.cpp
//
//    Description:  Implements parsing out requested entries from index file.
//
//        Version:  1.0
//        Created:  01/13/2014 10:34:25 AM
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

#include <iostream>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>
#include <thread>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <Poco/Net/NetException.h>

#include "FormFileRetriever.h"
#include "aLine.h"
#include "TickerConverter.h"

//--------------------------------------------------------------------------------------
//       Class:  FormFileRetriever
//      Method:  FormFileRetriever
// Description:  constructor
//--------------------------------------------------------------------------------------

FormFileRetriever::FormFileRetriever (const FTP_Server& ftp_server, int pause)
	: ftp_server_{ftp_server}, pause_{pause}

{
}  // -----  end of method FormFileRetriever::FormFileRetriever  (constructor)  -----

FormFileRetriever::FormsList FormFileRetriever::FindFilesForForms (const std::vector<std::string>& the_forms,
		const fs::path& local_index_file_name, const std::map<std::string, std::string>& ticker_map)
{
	//	Form types can have '/A' suffix to indicate ammended forms.
	//	For now, assume there is space in the file after form name such that I can
	//	safely add a trailing space to the form name while doing matching.

	//	The form index file is sorted by form type in ascending sequence.
	//	The form type is at the beginning of each line.
	//	The last field of each line is the path to the associated data file.

	//	the list of forms to search for needs to be sorted as well to match
	//	the sequence of forms in the index file.

	std::clog << "F: Searching index file: " << local_index_file_name << '\n';

	std::vector<std::string> forms_list{the_forms};
	std::sort(forms_list.begin(), forms_list.end());

	//	just in case there are duplicates in the forms list

	decltype(auto) pos = std::unique(forms_list.begin(), forms_list.end());
	if (pos != forms_list.end())
		forms_list.erase(pos);

	FormsList results;

	//	CIKs can have leading zeroes but the leading zeroes are not in the CIK field
	//	in the index file.
	//	Also, to avoid matching a substring, append a space to the CIK used in matching.
	//
	//	NOTE: we use a set below because the list of symbols(CIKs) that we are matching against
	//	is potentially large.  As such, search through a set will be faster than searching a vector.

	std::set<std::string> cik_list;
	if (! ticker_map.empty())
	{
		for (const auto& elem : ticker_map)
		{
			std::string search_key;
			boost::algorithm::trim_left_copy_if(std::back_inserter(search_key), elem.second, boost::is_any_of("0"));
		//	search_key += ' ';
			if (search_key != TickerConverter::NotFound)
				cik_list.insert(search_key);
		}
	}

	std::ifstream form_file{local_index_file_name.string()};

	std::istream_iterator<aLine> itor{form_file};
	std::istream_iterator<aLine> itor_end;

	//	let's skip over the header lines in the file

	for ( ; itor != itor_end; ++itor)
	{
		if (boost::algorithm::starts_with(itor->lineData, "----------"))
		{
			++itor;
			break;
		}
	}

	poco_assert_msg(itor != itor_end, ("Unable to find start of index entries in file: " + local_index_file_name.string()).c_str());

	for (auto& the_form : forms_list)
	{
		the_form += ' ';
		int found_a_form{0};
		std::vector<std::string> found_files;

		for ( ; itor != itor_end; ++itor)
		{
			if (itor->lineData < the_form)
				continue;

			if (boost::algorithm::starts_with(itor->lineData, the_form))
			{
				if (! cik_list.empty())
				{
					decltype(auto) pos1 = itor->lineData.find(' ', k_index_CIK_offset);
					decltype(auto) cik_from_index_file = itor->lineData.substr(k_index_CIK_offset, pos1 - k_index_CIK_offset);
					decltype(auto) pos = cik_list.find(cik_from_index_file);
					if (pos == cik_list.end())
						continue;
				}
				found_a_form += 1;
				decltype(auto) pos = itor->lineData.find("edgar/data");
				poco_assert_msg(pos != std::string::npos, "Badly formed index file record.");
				found_files.push_back(itor->lineData.substr(pos));
				boost::algorithm::trim_right(found_files.back());
			}
			else
			{
				if (found_a_form)
				{
					//	we've moved beyond our set of forms in the file so we can stop

					std::clog << "F: Found " << found_a_form << " files for form: " << the_form << '\n';
					the_form.pop_back();		//	remove trailing space we added at top of loop
					results[the_form] = found_files;
				}
				break;
			}
		}
	}

	int grand_total{0};
	for (const auto& elem : results)
		grand_total += elem.second.size();

	std::clog << "F: Found a total of " << grand_total << " files for specified forms.\n";
	return results;
}		// -----  end of method FormFileRetriever::FindFilesForForms  -----

FormFileRetriever::FormsList FormFileRetriever::FindFilesForForms (const std::vector<std::string>& the_forms, const fs::path& local_index_file_dir,
				const std::vector<std::string>& local_index_file_list, const std::map<std::string, std::string>& ticker_map)
{
	FormsList results;

	for (const auto& index_file : local_index_file_list)
	{
		fs::path local_index_file_name{local_index_file_dir};
		local_index_file_name /= index_file;

		decltype(auto) single_file_results = FindFilesForForms(the_forms, local_index_file_name, ticker_map);

		for (const auto& elem : single_file_results)
		{
			decltype(auto) pos = results.find(elem.first);
			if (pos != results.end())
				std::move(elem.second.begin(), elem.second.end(), std::back_inserter(results[pos->first]));
			else
				results[elem.first] = elem.second;
		}
	}

	int grand_total{0};
	for (const auto& elem : results)
		grand_total += elem.second.size();

	std::clog << "F: Found a grand total of " << grand_total << " files for specified forms in "
		<< local_index_file_list.size() << " files.\n";

	return results;
}		// -----  end of method FormFileRetriever::FindFilesForForm  -----

void FormFileRetriever::RetrieveSpecifiedFiles (const FormsList& form_list,
	const fs::path& local_form_directory, bool replace_files)
{
	for (const auto& elem : form_list)
		RetrieveSpecifiedFiles(elem.second, elem.first, local_form_directory, replace_files);
}

void FormFileRetriever::RetrieveSpecifiedFiles (const std::vector<std::string>& form_file_list, const std::string& the_form,
	const fs::path& local_form_directory, bool replace_files)
{
	ftp_server_.OpenFTPConnection();

	for (const auto& remote_file_name : form_file_list)
	{
		fs::path remote_file{remote_file_name};
		fs::path CIK_directory{remote_file.parent_path().leaf()};	//	pull off the CIK directory name
		fs::path local_file_name{local_form_directory};
		local_file_name /= CIK_directory;
		local_file_name /= the_form;
		fs::create_directories(local_file_name);
		local_file_name /= remote_file.leaf();

		if (replace_files || ! fs::exists(local_file_name))
		{
			try
			{
				ftp_server_.DownloadFile(remote_file_name, local_file_name);
				std::clog << "F: Retrieved remote form file: " << remote_file_name << " to: " << local_file_name << '\n';
				std::this_thread::sleep_for(pause_);
			}
			catch(Poco::Net::FTPException& e)
			{
				//	we just need to log this and then continue on with the next
				//	request just assuming the problem was temporary.

				std::clog << "F: !! Problem retrieving remote form file: " << remote_file_name
					<< " to: " << local_file_name << " !!\n" << e.displayText() << '\n';
			}
		}
	}

	ftp_server_.CloseFTPConnection();

}		// -----  end of method FormFileRetriever::RetrieveSpecifiedFiles  -----
