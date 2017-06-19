// =====================================================================================
//
//       Filename:  HTTPS_Connection.cpp
//
//    Description:  Implements HTTPS server read-only access
//
//        Version:  1.0
//        Created:  01/14/2014 10:23:57 AM
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
#include <memory>
#include <iostream>

#include <boost/algorithm/string/trim.hpp>
#include "Poco/Bugcheck.h"

#include "HTTPS_Connection.h"

#include "Poco/URIStreamOpener.h"
#include "Poco/StreamCopier.h"
#include "Poco/URI.h"
#include "Poco/Exception.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/HTTPStreamFactory.h"
#include "Poco/Net/HTTPSStreamFactory.h"
#include "Poco/Net/FTPStreamFactory.h"


using Poco::URIStreamOpener;
using Poco::StreamCopier;
using Poco::URI;
using Poco::Exception;
using Poco::Net::HTTPStreamFactory;
using Poco::Net::HTTPSStreamFactory;
using Poco::Net::FTPStreamFactory;



//--------------------------------------------------------------------------------------
//       Class:  HTTPS_Server
//      Method:  HTTPS_Server
// Description:  constructor
//--------------------------------------------------------------------------------------

HTTPS_Server::HTTPS_Server(const std::string& server_name)
	: server_name_{server_name}, session_{nullptr}

{
	session_ = new SSLInitializer();

	HTTPStreamFactory::registerFactory();
	HTTPSStreamFactory::registerFactory();
	FTPStreamFactory::registerFactory();

	ptrCert_ = new Poco::Net::AcceptCertificateHandler(false); // ask the user via console
	ptrContext_ = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_RELAXED, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
	Poco::Net::SSLManager::instance().initializeClient(0, ptrCert_, ptrContext_);
}  // -----  end of method HTTPS_Server::HTTPS_Server  (constructor)  -----


HTTPS_Server::~HTTPS_Server (void)
{
	delete session_;
}		// -----  end of method HTTPS_Server::~HTTPS_Server  -----


void HTTPS_Server::OpenHTTPSConnection (void)
{
	URI uri(server_name_);
	std::unique_ptr<std::istream> pStr(URIStreamOpener::defaultOpener().open(uri));

}		// -----  end of method HTTPS_Server::MakeHTTPSConnection  -----

void HTTPS_Server::CloseHTTPSConnection (void)
{
	// // if (session_)
	// // 	session_->close();
	// delete session_;
	// session_ = nullptr;
}		// -----  end of method HTTPS_Server::CloseHTTPSConnection  -----


void HTTPS_Server::ChangeWorkingDirectoryTo (const std::string& directory_name)
{
	// poco_assert_msg(session_, "Must open session before doing 'cwd'.");
	// cwd_.clear();
	// session_->setWorkingDirectory(directory_name);
	// cwd_ = directory_name;
}		// -----  end of method HTTPS_Server::ChangeWorkingDirectoryTo  -----

std::vector<std::string> HTTPS_Server::ListWorkingDirectoryContents (void)
{
	poco_assert_msg(session_, "Must open session before doing 'dir'.");

	//	we read and store our results so we can end the active connection quickly.

	std::vector<std::string> results;

	// decltype(auto) listing = session_->beginList();
	//
	// std::istream_iterator<aLine> itor{listing};
	// std::istream_iterator<aLine> itor_end;
	//
	// std::move(itor, itor_end, std::back_inserter(results));
	//
	// session_->endList();
	//
	// //	one last thing...let's make sure there's no junk at end of each entry.
	//
	// for (auto& x : results)
	// 	boost::algorithm::trim_right(x);

	return results;
}		// -----  end of method DailyIndexFileRetriever::ListRemoteDirectoryContents  -----


void HTTPS_Server::DownloadFile (const std::string& remote_file_name, const fs::path& local_file_name)
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
}		// -----  end of method HTTPS_Server::DownloadFile  -----

void HTTPS_Server::DownloadBinaryFile (const std::string& remote_file_name, const fs::path& local_file_name)
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
}		// -----  end of method HTTPS_Server::DownloadBinaryFile  -----
