// =====================================================================================
//
//       Filename:  HTTPS_Connection.cpp
//
//    Description:  Implements class which downloads files from an HTTPS server.
//
//        Version:  1.0
//        Created:  06/21/2017 10:08:13 AM
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

    // This code is based on sample code from Poco.

#include <fstream>
#include <iterator>

#include <chrono>
#include <future>
#include <thread>

#include <boost/algorithm/string/trim.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include "Poco/Bugcheck.h"

#include "HTTPS_Downloader.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"

// for .zip files, we need to use Poco's tools.

#include <Poco/Zip/Decompress.h>
#include "Poco/Zip/ZipArchive.h"
#include "Poco/Zip/ZipStream.h"
#include "Poco/StreamCopier.h"

#include "cpp-json/json.h"


// code from "The C++ Programming Language" 4th Edition. p. 1243.

template<typename T>
int wait_for_any(std::vector<std::future<T>>& vf, std::chrono::steady_clock::duration d)
// return index of ready future
// if no future is ready, wait for d before trying again
{
    while(true)
    {
        for (int i=0; i!=vf.size(); ++i)
        {
            if (!vf[i].valid()) continue;
            switch (vf[i].wait_for(std::chrono::seconds{0}))
            {
            case std::future_status::ready:
                    return i;

                case std::future_status::timeout:
                    break;

                case std::future_status::deferred:
                    throw std::runtime_error("wait_for_all(): deferred future");
            }
        }
    std::this_thread::sleep_for(d);
    }
}


//--------------------------------------------------------------------------------------
//       Class:  HTTPS_Downloader
//      Method:  HTTPS_Downloader
// Description:  constructor
//--------------------------------------------------------------------------------------

HTTPS_Downloader::HTTPS_Downloader(const std::string& server_name, Poco::Logger& the_logger)
	: server_name_{server_name}, the_logger_{the_logger}
{
	ssl_initializer_.reset(new SSLInitializer());

	ptrCert_ = new Poco::Net::AcceptCertificateHandler(false); // always accept FOR TESTING ONLY
	ptrContext_ = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_RELAXED,
		9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
	Poco::Net::SSLManager::instance().initializeClient(0, ptrCert_, ptrContext_);

	server_uri_ = server_name_;
	std::string path(server_uri_.getPathAndQuery());
	if (path.empty()) path = "/";
}  // -----  end of method HTTPS_Downloader::HTTPS_Downloader  (constructor)  -----


HTTPS_Downloader::~HTTPS_Downloader (void)
{
}		// -----  end of method HTTPS_Downloader::~HTTPS_Downloader  -----

std::string HTTPS_Downloader::RetrieveDataFromServer(const fs::path& request)

{
	poco_assert_msg(ssl_initializer_, "Must initialize SSL before interacting with the server.");

	auto session{Poco::Net::HTTPSClientSession{server_uri_.getHost(), server_uri_.getPort(), ptrContext_}};

	Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, request.string(), Poco::Net::HTTPMessage::HTTP_1_1);
	std::ostream& ostr = session.sendRequest(req);
	req.write(ostr);

	Poco::Net::HTTPResponse res;
	std::istream& rs = session.receiveResponse(res);

	std::string result;

	if (res.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
	{
		session.reset();
		throw std::runtime_error(request.string() + ". Result: " + std::to_string(res.getStatus()) +
			": Unable to complete request with server because: " + res.getReason());
	}
	else
		std::copy(std::istreambuf_iterator<char>(rs), std::istreambuf_iterator<char>(), std::back_inserter(result));
	return result;
}

std::vector<std::string> HTTPS_Downloader::ListDirectoryContents (const fs::path& directory_name)
{
	poco_assert_msg(ssl_initializer_, "Must initialize SSL before accessing the server.");

	//	we read and store our results so we can end the active connection quickly.
	// we will ask for the directory listing in JSON format which means we will
	// have to parse it out.

	fs::path index_file_name{directory_name};
	index_file_name /= "index.json";
	// server_uri_.setPath(directory_name.string());

	std::string index_listing = this->RetrieveDataFromServer(index_file_name.string());

	auto json_listing = json::parse(index_listing);

	std::vector<std::string> results;

	for (auto& e : json::as_array(json_listing["directory"]["item"]))
		results.push_back(json::as_string(e["name"]));

	//	one last thing...let's make sure there's no junk at end of each entry.

	for (auto& x : results)
		boost::algorithm::trim_right(x);

	return results;
}		// -----  end of method DailyIndexFileRetriever::ListRemoteDirectoryContents  -----


void HTTPS_Downloader::DownloadFile (const fs::path& remote_file_name, const fs::path& local_file_name)
{
	// basically the same as RetrieveDataFromServer but write the output to a file instead of a string.

	poco_assert_msg(ssl_initializer_, "Must initialize SSL before interacting with the server.");

    // but we also need to decompress any zipped files.  Might as well do it here.

    auto remote_ext = remote_file_name.extension();
    auto local_ext = local_file_name.extension();

    // we can unzip zipped files but only if the local file name indicates the local file is not zipped.

	bool need_to_unzip = (remote_ext == ".gz" or remote_ext == ".zip") && remote_ext != local_ext;

	auto session{Poco::Net::HTTPSClientSession{server_uri_.getHost(), server_uri_.getPort(), ptrContext_}};

	Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, remote_file_name.string(), Poco::Net::HTTPMessage::HTTP_1_1);
	std::ostream& ostr = session.sendRequest(req);
	req.write(ostr);

	Poco::Net::HTTPResponse res;
	std::istream& remote_file = session.receiveResponse(res);

	if (res.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
	{
		session.reset();
		throw std::runtime_error(remote_file_name.string() + ": Result: " + std::to_string(res.getStatus()) +
			": Unable to download file.");
	}
	else
	{
		if (! need_to_unzip)
		{
			std::ofstream local_file{local_file_name.string(), std::ios::out | std::ios::binary};

			// avoid stream formatting by using streambufs

			std::copy(std::istreambuf_iterator<char>(remote_file), std::istreambuf_iterator<char>(), std::ostreambuf_iterator<char>(local_file));
		}
		else
		{
			// we are going to decompress on the fly...

			std::ofstream expanded_file{local_file_name.string(), std::ios::out | std::ios::binary};

			if (remote_ext == ".gz")
			{
				// for gzipped files, we can use boost (which uses zlib)

		    	boost::iostreams::filtering_istream in;
		    	in.push(boost::iostreams::gzip_decompressor());
		    	in.push(remote_file);
		    	boost::iostreams::copy(in, expanded_file);
			}
			else
			{
				// zip archives, we need to use Poco.
				// but, to do that, we need to download the file then expand it.

				fs::path temp_file_name{local_file_name};
				temp_file_name.replace_extension(remote_ext);

				std::ofstream local_file{temp_file_name.string(), std::ios::out | std::ios::binary};

				// avoid stream formatting by using streambufs

				std::copy(std::istreambuf_iterator<char>(remote_file), std::istreambuf_iterator<char>(), std::ostreambuf_iterator<char>(local_file));

				local_file.close();

				std::ifstream zipped_file(temp_file_name.string(), std::ios_base::in | std::ios_base::binary);
				Poco::Zip::Decompress expander{zipped_file, local_file_name.parent_path().string(), true};
				expander.decompressAllFiles();

				fs::remove(temp_file_name);
			}
		}
	}
}		// -----  end of method HTTPS_Downloader::DownloadFile  -----

std::pair<int, int> HTTPS_Downloader::DownloadFilesConcurrently(const remote_local_list& file_list, int max_at_a_time)

{
    int success_counter = 0;
    int error_counter = 0;

    for (int i = 0; i < file_list.size(); )
    {
        // keep track of our async processes here.

        std::vector<std::future<void>> tasks;
        tasks.reserve(max_at_a_time + 1);

        for (int j = 0; j < max_at_a_time && i < file_list.size(); ++j, ++i)
        {
            // queue up our tasks up to the limit.

            auto& [remote_file, local_file] = file_list[i];
            tasks.push_back(std::async(&HTTPS_Downloader::DownloadFile, this, remote_file, local_file));
        }

        // lastly, throw in our delay just in case we need it.

        tasks.push_back(std::async(&HTTPS_Downloader::Timer, this));

        // now, let's wait till they're all done
        // and then we'll do the next bunch.

        for (int count = tasks.size(); count; --count)
        {
            int i = wait_for_any(tasks, std::chrono::microseconds{100});
            try
            {
                tasks[i].get();
                ++success_counter;
            }
            catch (const std::exception& e)
            {
                // any problems, we'll document them and continue.

                poco_error(the_logger_, e.what());
                ++error_counter;
                continue;
            }
            catch (...)
            {
                // any problems, we'll document them and continue.

                poco_error(the_logger_, "Unknown problem with an async download process");
                ++error_counter;
                continue;
            }
        }
    }

    return std::make_pair(success_counter, error_counter);

}		// -----  end of method HTTPS_Downloader::DownloadFilesConcurrently  -----


void HTTPS_Downloader::Timer(void)

{
	//	given the size of the files we are downloading, it
	// seems unlikely this will have any effect.  But, for
	// small files it may.

	std::this_thread::sleep_for(std::chrono::seconds{1});
}
