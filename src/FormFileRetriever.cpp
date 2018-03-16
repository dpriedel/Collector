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

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>
#include <thread>
#include <experimental/string_view>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>

// define our own 'transform_if' for now.
// don't use the one from boost because it pulls in a
// whole bunch of math stuff.

template<typename InputIterator, typename OutputIterator, typename Predicate, typename UnaryOperation>
OutputIterator transform_if(InputIterator first, InputIterator last,
    OutputIterator result, Predicate pred, UnaryOperation unaryop)
{
    for (; first != last; ++first)
    {
        if (pred(*first))
        {
          *result = unaryop(*first);
          ++result;
        }
    }
    return result;
}

#include <Poco/Net/NetException.h>

#include "FormFileRetriever.h"

// function to split a string on a delimiter and return a vector of string-views

std::vector<std::string_view> split_string(const std::string_view& string_data, char delim)
{
    std::vector<std::string_view> results;
	for (auto it = 0; it != string_data.npos; ++it)
	{
		auto pos = string_data.find(delim, it);
        if (pos != string_data.npos)
    		results.emplace_back(string_data.substr(it, pos - it));
        else
        {
    		results.emplace_back(string_data.substr(it));
            break;
        }
		it = pos;
	}
    return results;
}

// #include "aLine.h"

//--------------------------------------------------------------------------------------
//       Class:  FormFileRetriever
//      Method:  FormFileRetriever
// Description:  constructor
//--------------------------------------------------------------------------------------

FormFileRetriever::FormFileRetriever (HTTPS_Downloader& a_server, Poco::Logger& the_logger)
	: the_server_{a_server}, the_logger_{the_logger}

{
}  // -----  end of method FormFileRetriever::FormFileRetriever  (constructor)  -----

auto FormFileRetriever::ExtractFileName(int& found_a_form)
{
    return [&found_a_form] (const auto& e)
    {
        found_a_form += 1;
        auto pos = e.find("edgar/data");
        poco_assert_msg(pos != std::string::npos, "Badly formed index file record.");

        auto f_name{e.substr(pos)};
        f_name.remove_suffix(f_name.size() - f_name.find_last_not_of(" \r\t") - 1);
        fs::path f_path{"/Archives"};
        f_path /= f_name;
        return std::move(f_path);
    };
}

FormFileRetriever::FormsAndFilesList FormFileRetriever::FindFilesForForms (const std::vector<std::string>& the_form_types,
		const fs::path& local_index_file_name, const TickerConverter::TickerCIKMap& ticker_map)
{
	//	Form types can have '/A' suffix to indicate ammended forms.
	//	For now, assume there is space in the file after form name such that I can
	//	safely add a trailing space to the form name while doing matching.

	//	The form index file is sorted by form type in ascending sequence.
	//	The form type is at the beginning of each line.
	//	The last field of each line is the path to the associated data file.

	//	the list of forms to search for needs to be sorted as well to match
	//	the sequence of forms in the index file.

    poco_debug(the_logger_, "F: Searching index file: " + local_index_file_name.string());

	// we load our file into memory so we have more flexibility in how we search it.

	std::string index_file_data(fs::file_size(local_index_file_name), '\0');
	std::ifstream index_file{local_index_file_name, std::ios_base::in | std::ios_base::binary};
	poco_assert_msg(index_file, ("Unable to open index file: " + local_index_file_name.string()).c_str());
    index_file.read(&index_file_data[0], index_file_data.size());
	index_file.close();

	// split into lines
	// if there is a '\r' at the end of a line, it will be stripped out later.

	auto index_data_lines = split_string(index_file_data, '\n');

	std::vector<std::string> forms_list{the_form_types};
	std::sort(forms_list.begin(), forms_list.end());

	//	just in case there are duplicates in the forms list

    forms_list.erase(std::unique(forms_list.begin(), forms_list.end()), forms_list.end());

	FormsAndFilesList results;

	//	We may have been given a list of symbols to filter against.  If so,
	//	we need to translate the symbol to its CIK.  We have a table with this mapping.

	//	CIKs can have leading zeroes in the table but the leading zeroes are not in the CIK field
	//	in the index file.

	std::vector<std::string_view> cik_list;
	if (! ticker_map.empty())
	{
		std::transform(std::begin(ticker_map),
			std::end(ticker_map),
			std::back_inserter(cik_list),
			[](const auto& elem) { std::string_view x{elem.second}; x.remove_prefix(x.find_first_not_of('0')); return x; });
	}

	auto itor {std::begin(index_data_lines)};
	auto itor_end {std::end(index_data_lines)};

	//	let's skip over the header lines in the file

	auto form_itor = std::find_if(itor, itor_end,
		[] (const auto& line) { return boost::algorithm::starts_with(line, "----------"); });

	poco_assert_msg(form_itor != itor_end, ("Unable to find start of index entries in file: " + local_index_file_name.string()).c_str());

	++form_itor;

	// some functions to use in our processing.

    auto find_CIK_in_line = [&] (const auto& next_line)
    {
		auto pos1 = next_line.find_first_of(' ', k_index_CIK_offset);
		auto cik_from_index_file = next_line.substr(k_index_CIK_offset, pos1 - k_index_CIK_offset);
        return cik_from_index_file;
    };

	auto CIK_is_in_CIK_list = [&] (const auto& next_line)
	{
		if ( cik_list.empty())
			return true;		//	no filter so pass everything

		auto cik_from_index_file = find_CIK_in_line(next_line);
		auto pos = std::find(std::begin(cik_list), std::end(cik_list), cik_from_index_file);
		return pos == cik_list.end() ? false : true;
	};

	for (auto& the_form : forms_list)
	{
		the_form += ' ';
		int found_a_form{0};
		std::vector<fs::path> found_files;

		// first, find the entries, if any, for our form.

		auto list_begin = std::lower_bound(form_itor, itor_end, the_form,
            [] (const auto& the_line, const auto& the_form) { return the_line.compare(0, the_form.size(), the_form.data(), the_form.size()) < 0; } );
		auto list_end = std::upper_bound(list_begin, itor_end, the_form,
			[] (const auto& the_form, const auto& the_line) { return the_form.compare(0, the_form.size(), the_line.data(), the_form.size()) < 0; } );

		if (list_begin == list_end)
        {
            // found no lines in index file for our form type.

			poco_debug(the_logger_, "F: Found " + std::to_string(found_a_form) + " files for form: " + the_form);
			continue;
        }

        // collect list of files to download for our form type

        transform_if(
            list_begin,
            list_end,
            std::back_inserter(found_files),
            [&] (const auto& e) { return CIK_is_in_CIK_list(e); },
            ExtractFileName(found_a_form)
        );

		if (found_a_form)
		{
			poco_debug(the_logger_, "F: Found " + std::to_string(found_a_form) + " files for form: " + the_form);
			the_form.pop_back();		//	remove trailing space we added at top of loop
			results[the_form] = found_files;
		}

		form_itor = list_end;
	}

	int grand_total{0};
	for (const auto& [form, files] : results)
		grand_total += files.size();

	poco_debug(the_logger_, "F: Found a total of " + std::to_string(grand_total) + " files for specified forms.");
	return results;
}		// -----  end of method FormFileRetriever::FindFilesForForms  -----

FormFileRetriever::FormsAndFilesList FormFileRetriever::FindFilesForForms (const std::vector<std::string>& the_form_types,
    const std::vector<fs::path>& local_index_files, const TickerConverter::TickerCIKMap& ticker_map)
{
    // separately process each file in our list and accumulate the results

	FormsAndFilesList results;

	for (const auto& index_file : local_index_files)
	{
		decltype(auto) single_file_results = FindFilesForForms(the_form_types, index_file, ticker_map);

		for (const auto& [form, files] : single_file_results)
		{
			// if we already have found files for the given form type, add them to to the list
			// otherwise, make a new entry.
            // this code nicely works in either case.

            std::move(files.begin(), files.end(), std::back_inserter(results[form]));
		}
	}

	int grand_total{0};
	for (const auto& elem : results)
		grand_total += elem.second.size();

	poco_information(the_logger_, "F: Found a grand total of " + std::to_string(grand_total) + " files for specified forms in "
		+ std::to_string(local_index_files.size()) + " files.");

	return results;
}		// -----  end of method FormFileRetriever::FindFilesForForm  -----

void FormFileRetriever::RetrieveSpecifiedFiles (const FormsAndFilesList& form_list,
	const fs::path& local_form_directory, bool replace_files)
{
	for (const auto& [form_type, form_files] : form_list)
		RetrieveSpecifiedFiles(form_files, form_type, local_form_directory, replace_files);
}

void FormFileRetriever::ConcurrentlyRetrieveSpecifiedFiles (const FormsAndFilesList& form_list,
	const fs::path& local_form_directory, int max_at_a_time, bool replace_files)
{
	for (const auto& [form_type, form_files] : form_list)
		ConcurrentlyRetrieveSpecifiedFiles(form_files, form_type, local_form_directory, max_at_a_time, replace_files);
}

void FormFileRetriever::RetrieveSpecifiedFiles (const std::vector<fs::path>& remote_file_names, const std::string& form_type,
	const fs::path& local_form_directory, bool replace_files)
{
    // some forms can have slash in the form local_file_name so...

    std::string form_name{form_type};
    std::replace(form_name.begin(), form_name.end(), '/', '_');

    int downloaded_files_counter = 0;
    int skipped_files_counter = 0;
	int error_counter = 0;

	for (const auto& remote_file_name : remote_file_names)
	{
        // we download the remote file to a local directory structure like this
        // <local_form_directory>/<CIK>/<form_type>/<remote_file_name>

		auto local_dir_name = MakeLocalDirNameFromRemoteFileName(local_form_directory, remote_file_name, form_name);
        auto local_file_name{local_dir_name};
        local_file_name /= remote_file_name.filename();

		if (replace_files || ! fs::exists(local_file_name))
		{
			try
			{
				fs::create_directories(local_dir_name);
				the_server_.DownloadFile(remote_file_name, local_file_name);
                ++downloaded_files_counter;
				poco_debug(the_logger_, "F: Retrieved remote form file: " + remote_file_name.string() + " to: " + local_file_name.string());
				// std::this_thread::sleep_for(pause_);
			}
			catch(Poco::TimeoutException& e)
			{
				//	we just need to log this and then continue on with the next
				//	request just assuming the problem was temporary.

				poco_error(the_logger_, "F: !! Timeout retrieving remote form file: " + remote_file_name.string()
					+ " to: " + local_file_name.string() + " !!\n" + e.displayText());

				++error_counter;
                continue;
			}
			catch(std::exception& e)
			{
				//	we just need to log this and then continue on with the next
				//	request just assuming the problem was temporary.

				poco_error(the_logger_, "F: !! Problem retrieving remote form file: " + remote_file_name.string()
					+ " to: " + local_file_name.string() + " !!\n" + e.what());
				++error_counter;
			}
		}
		else
		{
			poco_debug(the_logger_, "F: File exists and 'replace' is false: skipping download: " + local_file_name.filename().string());
			++skipped_files_counter;
		}
	}

	poco_information(the_logger_, "F: Downloaded: " + std::to_string(downloaded_files_counter) +
		". Skipped: " + std::to_string(skipped_files_counter) +
		". Errors: " + std::to_string(error_counter) + ". for files for form type: " + form_type);
}		// -----  end of method FormFileRetriever::RetrieveSpecifiedFiles  -----

fs::path FormFileRetriever::MakeLocalDirNameFromRemoteFileName(const fs::path& local_form_directory_name, const fs::path& remote_file_name,
    const std::string& form_name)
{
    auto CIK_directory{remote_file_name.parent_path().filename()};	//	pull off the CIK directory name
    auto local_dir_name{local_form_directory_name};
    local_dir_name /= CIK_directory;
    local_dir_name /= form_name;
    return local_dir_name;
}

auto FormFileRetriever::AddToCopyList(const std::string& form_name, const fs::path& local_form_directory, bool replace_files)
{
	//	construct our lambda function here so it doesn't clutter up our code below.

    return [this, &form_name, &local_form_directory, replace_files] (const auto& remote_file_name)
    {
        auto local_dir_name = this->MakeLocalDirNameFromRemoteFileName(local_form_directory, remote_file_name, form_name);
        auto local_file_name{local_dir_name};
        local_file_name /= remote_file_name.filename();

        if (replace_files || ! fs::exists(local_file_name))
        {
            fs::create_directories(local_dir_name);
            return HTTPS_Downloader::copy_file_names(remote_file_name, local_file_name);
        }
        else
        {
            // we use an empty remote file name to indicate no copy needed as the local file already exists.

            return HTTPS_Downloader::copy_file_names({}, local_file_name);
        }
	};
}

void FormFileRetriever::ConcurrentlyRetrieveSpecifiedFiles (const std::vector<fs::path>& remote_file_names, const std::string& form_type,
	const fs::path& local_form_directory, int max_at_a_time, bool replace_files)
{
	if (remote_file_names.size() < max_at_a_time)
		return RetrieveSpecifiedFiles(remote_file_names, form_type, local_form_directory, replace_files);

    // some forms can have slash in the form local_file_name so...

    std::string form_name{form_type};
    std::replace(form_name.begin(), form_name.end(), '/', '_');

	// we need to create a list of file name pairs -- remote file name, local file name.
	// we'll pass that list to the downloader and let it manage to process from there.

	// Also, here we will create the directory hierarchies for the to-be downloaded files.
	// Taking the easy way out so we don't have to worry about file system race conditions.

	HTTPS_Downloader::remote_local_list concurrent_copy_list;

    std::transform(
        std::begin(remote_file_names),
        std::end(remote_file_names),
        std::back_inserter(concurrent_copy_list),
		AddToCopyList(form_name, local_form_directory, replace_files)
        );
	// now, we expect some magic to happen here...

	auto [success_counter, error_counter] = the_server_.DownloadFilesConcurrently(concurrent_copy_list, max_at_a_time);

    // if the first file name in the pair is empty, there was no download done.

    int skipped_files_counter = std::count_if(std::begin(concurrent_copy_list), std::end(concurrent_copy_list),
		[] (const auto& e) { return ! e.first; });

	poco_information(the_logger_, "F: Downloaded: " + std::to_string(success_counter) +
		". Skipped: " + std::to_string(skipped_files_counter) +
		". Errors: " + std::to_string(error_counter) + ". for files for form type: " + form_type);

	// TODO: figure our error handling when some files do not get downloaded.
    // Let's try this for now.

    if (concurrent_copy_list.size() != success_counter + skipped_files_counter)
        throw std::runtime_error("Download count = " + std::to_string(success_counter) + ". Should be: " + std::to_string(concurrent_copy_list.size()));

}		// -----  end of method FormFileRetriever::RetrieveSpecifiedFiles  -----
