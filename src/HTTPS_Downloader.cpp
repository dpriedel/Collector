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

#include <algorithm>
#include <fstream>
#include <iterator>
#include <memory>
#include <iostream>

#include <boost/algorithm/string/trim.hpp>
#include "Poco/Bugcheck.h"

#include "HTTPS_Downloader.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/URIStreamOpener.h"
#include "Poco/StreamCopier.h"
#include "Poco/Exception.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"

#include "cpp-json/json.h"

using Poco::URIStreamOpener;
using Poco::StreamCopier;
using Poco::URI;
using Poco::Exception;



//--------------------------------------------------------------------------------------
//       Class:  HTTPS_Downloader
//      Method:  HTTPS_Downloader
// Description:  constructor
//--------------------------------------------------------------------------------------

HTTPS_Downloader::HTTPS_Downloader(const std::string& server_name)
	: server_name_{server_name}

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
		throw std::runtime_error(request.string() + ". Result: " + std::to_string(res.getStatus()) +
			": Unable to complete request with server.");
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


void HTTPS_Downloader::DownloadFile (const std::string& remote_file_name, const fs::path& local_file_name)
{
	// poco_assert_msg(session_, "Must open session before doing 'download'.");
	//
	// std::ofstream local_file{local_file_name.string(), std::ios::out | std::ios::binary};
	// std::ostream_iterator<aLine> otor{local_file, "\n"};
	//
    // try
    // {
    // 	decltype(auto) remote_file = session_->beginDownload(remote_file_name);
	//
    // 	std::istream_iterator<aLine> itor{remote_file};
    // 	std::istream_iterator<aLine> itor_end;
	//
    // 	std::move(itor, itor_end, otor);
    //     session_->endDownload();
    // }
    // catch(...)
    // {
    //     session_->endDownload();
    //     throw;
    // }
}		// -----  end of method HTTPS_Downloader::DownloadFile  -----

void HTTPS_Downloader::DownloadBinaryFile (const std::string& remote_file_name, const fs::path& local_file_name)
{
	// //	found this approach at insanecoding.blogspot.com
	//
	// poco_assert_msg(session_, "Must open session before doing 'download'.");
	//
	// session_->setFileType(Poco::Net::HTTPSClientSession::TYPE_BINARY);
	// std::ofstream local_file{local_file_name.string(), std::ios::out | std::ios::binary};
	//
	// decltype(auto) remote_file = session_->beginDownload(remote_file_name);
	// local_file << remote_file.rdbuf();
	// session_->endDownload();
	//
}		// -----  end of method HTTPS_Downloader::DownloadBinaryFile  -----
