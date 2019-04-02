// =====================================================================================
// 
//       Filename:  FTP_Connection.h
// 
//    Description:  Class provides read-only acces to an FTP server.
// 
//        Version:  1.0
//        Created:  01/14/2014 10:08:13 AM
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


#ifndef FTP_CONNECTION_H_
#define FTP_CONNECTION_H_

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

//#include <Poco/URI.h>
#include <Poco/Net/FTPClientSession.h>

// =====================================================================================
//        Class:  FTP_Server
//  Description:  provides read-only access to an FTP server
// =====================================================================================
class FTP_Server
{
	public:
		// ====================  LIFECYCLE     =======================================
		FTP_Server ()=delete;                             // constructor
		FTP_Server(const std::string& server_name, const std::string& user_name, const std::string& pswd);
		~FTP_Server(void);

		// ====================  ACCESSORS     =======================================

		bool HaveActiveSession(void) const { return session_; }
		const std::string& GetWorkingDirectory(void) const { return cwd_;}
	
		// ====================  MUTATORS      =======================================

		void OpenFTPConnection(void);
		void CloseFTPConnection(void);
		void ChangeWorkingDirectoryTo(const std::string& directory_name);
		void DownloadFile(const std::string& remote_file_name, const fs::path& local_file_name);
		void DownloadBinaryFile(const std::string& remote_file_name, const fs::path& local_file_name);
		
		std::vector<std::string> ListWorkingDirectoryContents(void);

		// ====================  OPERATORS     =======================================

	protected:
		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================

		std::string server_name_;
		std::string user_name_;
		std::string pswd_;
		std::string cwd_;
		Poco::Net::FTPClientSession* session_;

}; // -----  end of class FTP_Server  -----

#endif /* FTP_CONNECTION_H_ */

