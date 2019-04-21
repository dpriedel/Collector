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


#ifndef HTTPS_DOWNLOADER_H
#define HTTPS_DOWNLOADER_H

#include <filesystem>
#include <optional>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

namespace fs = std::filesystem;

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>

#ifndef NOCERTTEST
#include "example/common/root_certificates.hpp"
#endif
#include "Poco/Logger.h"


// =====================================================================================
//        Class:  HTTPS_Downloader
//  Description:  provides read-only access to an HTTPS server
// =====================================================================================
class HTTPS_Downloader
{
	public:

		using  copy_file_names = std::pair<std::optional<fs::path>, fs::path>;
		using  remote_local_list = std::vector<copy_file_names>;

		// ====================  LIFECYCLE     =======================================
		HTTPS_Downloader ()=delete;                             // constructor
		HTTPS_Downloader(const std::string& server_name, const std::string& port, Poco::Logger& the_logger);
		~HTTPS_Downloader()=default;

		// ====================  ACCESSORS     =======================================

		// request must be a full path name for what is to be retrieved.

        std::string RetrieveDataFromServer(const fs::path& request);

		std::vector<std::string> ListDirectoryContents(const fs::path& directory_name);

		// download a file at a time

		void DownloadFile(const fs::path& remote_file_name, const fs::path& local_file_name);

		// download multiple files at a time, up to specified limit.
        // this version returns the number of errors encountered.
        // Errors are trapped and logged by the downloader.

		std::pair<int, int> DownloadFilesConcurrently(const remote_local_list& file_list, int max_at_a_time);

		// ====================  MUTATORS      =======================================

		// ====================  OPERATORS     =======================================

	protected:
		// ====================  DATA MEMBERS  =======================================

	private:

		// we use a timer to stay within usage restrictions of SEC web site.

		void Timer();
        static void HandleSignal(int signal);

		void DownloadTextFile(const fs::path& local_file_name, const std::vector<char>& remote_data, const fs::path& remote_file_name);
		void DownloadGZipFile(const fs::path& local_file_name, const std::vector<char>& remote_data, const fs::path& remote_file_name);
		void DownloadZipFile(const fs::path& local_file_name, const std::vector<char>& remote_data, const fs::path& remote_file_name);

		// ====================  DATA MEMBERS  =======================================

		std::string server_name_;
        std::string port_ = "443";      // default for SSL
        int version_ = 11;

		fs::path path_;
        
        boost::asio::io_context ioc;
        boost::asio::ssl::context ctx;

        Poco::Logger& the_logger_;

        static bool had_signal_;
}; // -----  end of class HTTPS_Downloader  -----

#endif /* HTTPS_DOWNLOADER_H */
