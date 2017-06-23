// =====================================================================================
//
//       Filename:  HTTPS_Connection.h
//
//    Description:  Class downloads files from an HTTPS server.
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


#ifndef HTTPS_DOWNLOADER_H
#define HTTPS_DOWNLOADER_H

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#include <Poco/URI.h>
#include "Poco/Net/Context.h"
#include "Poco/Net/AcceptCertificateHandler.h"
#include "Poco/Net/ConsoleCertificateHandler.h"
#include "Poco/SharedPtr.h"


// =====================================================================================
//        Class:  HTTPS_Downloader
//  Description:  provides read-only access to an HTTPS server
// =====================================================================================
class HTTPS_Downloader
{
	public:
		// ====================  LIFECYCLE     =======================================
		HTTPS_Downloader ()=delete;                             // constructor
		HTTPS_Downloader(const std::string& server_name);
		~HTTPS_Downloader(void);

		// ====================  ACCESSORS     =======================================

		// request must be a full path name for what is to be retrieved.

        std::string RetrieveDataFromServer(const fs::path& request);

		std::vector<std::string> ListDirectoryContents(const fs::path& directory_name);

		// for these next 2 methods, the file name will be appended to whatever
		// the current working directory is.

		void DownloadFile(const std::string& remote_file_name, const fs::path& local_file_name);
		void DownloadBinaryFile(const std::string& remote_file_name, const fs::path& local_file_name);

		// ====================  MUTATORS      =======================================

		// ====================  OPERATORS     =======================================

	protected:
		// ====================  DATA MEMBERS  =======================================

	private:

		class SSLInitializer
		{
		public:
			SSLInitializer()
			{
				Poco::Net::initializeSSL();
			}

			~SSLInitializer()
			{
				Poco::Net::uninitializeSSL();
			}
		};

		// ====================  DATA MEMBERS  =======================================

		Poco::URI server_uri_;
		std::string server_name_;
		fs::path path_;

		// Poco::SharedPtr<Poco::Net::InvalidCertificateHandler> ptrCert_; // ask the user via console
		Poco::SharedPtr<Poco::Net::AcceptCertificateHandler> ptrCert_; // ask the user via console
		Poco::Net::Context::Ptr ptrContext_;

		std::unique_ptr<SSLInitializer> ssl_initializer_;
}; // -----  end of class HTTPS_Downloader  -----

#endif /* HTTPS_DOWNLOADER_H */
