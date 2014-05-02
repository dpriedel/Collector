// =====================================================================================
// 
//       Filename:  FTP_Connection.cpp
// 
//    Description:  Implements FTP server read-only access
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


#include <fstream>
#include <iterator>

#include <boost/algorithm/string/trim.hpp>

#include "FTP_Connection.h"
#include "aLine.h"
#include "TException.h"

//--------------------------------------------------------------------------------------
//       Class:  FTP_Server
//      Method:  FTP_Server
// Description:  constructor
//--------------------------------------------------------------------------------------

FTP_Server::FTP_Server(const std::string& server_name, const std::string& user_name, const std::string& pswd)
	: server_name_{server_name}, user_name_{user_name}, pswd_{pswd}, session_{nullptr}

{
}  // -----  end of method FTP_Server::FTP_Server  (constructor)  -----


FTP_Server::~FTP_Server (void)
{
	delete session_;
}		// -----  end of method FTP_Server::~FTP_Server  -----


void FTP_Server::OpenFTPConnection (void)
{
	if (! session_)
	{	
		session_ = new Poco::Net::FTPClientSession{server_name_};
		session_->login(user_name_, pswd_);
		session_->setPassive(true);
	}
	
}		// -----  end of method FTP_Server::MakeFTPConnection  -----

void FTP_Server::CloseFTPConnection (void)
{
	if (session_)
		session_->close();
	delete session_;
	session_ = nullptr;
}		// -----  end of method FTP_Server::CloseFTPConnection  -----


void FTP_Server::ChangeWorkingDirectoryTo (const std::string& directory_name)
{
	dthrow_if_(! session_, "Must open session before doing 'cwd'.");
	cwd_.clear();
	session_->setWorkingDirectory(directory_name);
	cwd_ = directory_name;
}		// -----  end of method FTP_Server::ChangeWorkingDirectoryTo  -----

std::vector<std::string> FTP_Server::ListWorkingDirectoryContents (void)
{
	dthrow_if_(! session_, "Must open session before doing 'dir'.");

	//	we read and store our results so we can end the active connection quickly.
	
	std::vector<std::string> results;
	
	decltype(auto) listing = session_->beginList();

	std::istream_iterator<aLine> itor{listing};
	std::istream_iterator<aLine> itor_end;

	std::move(itor, itor_end, std::back_inserter(results));
	
	session_->endList();
	
	//	one last thing...let's make sure there's no junk at end of each entry.
	
	for (auto& x : results)
		boost::algorithm::trim_right(x);

	return results;
}		// -----  end of method DailyIndexFileRetriever::ListRemoteDirectoryContents  -----


void FTP_Server::DownloadFile (const std::string& remote_file_name, const fs::path& local_file_name)
{
	dthrow_if_(! session_, "Must open session before doing 'download'.");

	std::ofstream local_file{local_file_name.string(), std::ios::out | std::ios::binary};
	std::ostream_iterator<aLine> otor{local_file, "\n"};

	decltype(auto) remote_file = session_->beginDownload(remote_file_name);

	std::istream_iterator<aLine> itor{remote_file};
	std::istream_iterator<aLine> itor_end;

	std::move(itor, itor_end, otor);

	session_->endDownload();

}		// -----  end of method FTP_Server::DownloadFile  -----

void FTP_Server::DownloadBinaryFile (const std::string& remote_file_name, const fs::path& local_file_name)
{
	//	found this approach at insanecoding.blogspot.com
	
	dthrow_if_(! session_, "Must open session before doing 'download'.");

	session_->setFileType(Poco::Net::FTPClientSession::TYPE_BINARY);
	std::ofstream local_file{local_file_name.string(), std::ios::out | std::ios::binary};

	decltype(auto) remote_file = session_->beginDownload(remote_file_name);
	local_file << remote_file.rdbuf();
	session_->endDownload();

}		// -----  end of method FTP_Server::DownloadBinaryFile  -----
