// =====================================================================================
//
//       Filename:  CollectorApp.h
//
//    Description:  main application
//
//        Version:  1.0
//        Created:  01/17/2014 11:13:53 AM
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


// =====================================================================================
//        Class:  CollectorApp
//  Description:
// =====================================================================================

#ifndef COLLECTORAPP_H_
#define COLLECTORAPP_H_

#include <map>
#include <memory>
#include <filesystem>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/program_options.hpp>

namespace bg = boost::gregorian;
namespace fs = std::filesystem;
namespace po = boost::program_options;

#include "TickerConverter.h"

class CollectorApp
{
public:

    // use ctor below for testing with predefined options

    explicit CollectorApp(const std::vector<std::string>& tokens);
    
    CollectorApp() = delete;
	CollectorApp(int argc, char* argv[]);
	CollectorApp(const CollectorApp& rhs) = delete;
	CollectorApp(CollectorApp&& rhs) = delete;

    ~CollectorApp() = default;

    CollectorApp& operator=(const CollectorApp& rhs) = delete;
    CollectorApp& operator=(CollectorApp&& rhs) = delete;

    bool Startup();
    void Run();
    void Shutdown();

protected:

	//	Setup for parsing program options.

	void	SetupProgramOptions();
	void 	ParseProgramOptions();
	void 	ParseProgramOptions(const std::vector<std::string>& tokens);

    void    ConfigureLogging();

	bool	CheckArgs ();
	void	Do_Quit ();

	void Do_Run_DailyIndexFiles();
	void Do_Run_QuarterlyIndexFiles();
	void Do_Run_TickerLookup();
	void Do_Run_TickerFileLookup();

	void Do_TickerMap_Setup();

		// ====================  DATA MEMBERS  =======================================

private:


		// ====================  DATA MEMBERS  =======================================

	po::positional_options_description	mPositional;			//	old style options
    std::unique_ptr<po::options_description> mNewOptions;    	//	new style options (with identifiers)
	po::variables_map					mVariableMap;

	int mArgc = 0;
	char** mArgv = nullptr;
	const std::vector<std::string> tokens_;

	TickerConverter ticker_converter_;

	bg::date begin_date_;
	bg::date end_date_;

    std::string start_date_;
    std::string stop_date_;

	std::string mode_{"daily"};
	std::string form_{"10-Q"};
	std::string ticker_;
	// std::string login_ID_;
    std::string HTTPS_host_{"www.sec.gov"};
    std::string HTTPS_port_{"443"};
    std::string logging_level_{"information"};

	std::vector<std::string> form_list_;
	std::vector<std::string> ticker_list_;

	std::map<std::string, std::string> ticker_map_;

	fs::path log_file_path_name_;
	fs::path local_index_file_directory_;
	fs::path local_form_file_directory_;
	fs::path ticker_cache_file_name_;
	fs::path ticker_list_file_name_;

	int pause_{0};
    int max_forms_to_download_{-1};     // mainly for testing
    int max_at_a_time_{10};             // how many concurrent downloads allowed

	bool replace_index_files_{false};
	bool replace_form_files_{false};
	bool index_only_{false};			//	do no download any form files
	bool help_requested_{false};

}; // -----  end of class CollectorApp  -----

#endif /* COLLECTORAPP_H_ */
