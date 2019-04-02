// =====================================================================================
//
//       Filename:  CollectEDGARApp.h
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


// =====================================================================================
//        Class:  CollectEDGARApp
//  Description:
// =====================================================================================

#ifndef COLLECTEDGARAPP_H_
#define COLLECTEDGARAPP_H_

// #include <fstream>
#include <map>
#include <filesystem>

// #include <boost/filesystem.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace bg = boost::gregorian;
namespace fs = std::filesystem;

#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/AutoPtr.h"
#include "Poco/Util/Validator.h"

#include "Poco/Logger.h"
#include "Poco/Channel.h"

#include "TickerConverter.h"

class CollectEDGARApp : public Poco::Util::Application
{

public:

	CollectEDGARApp(int argc, char* argv[]);
	CollectEDGARApp(const CollectEDGARApp& rhs) = delete;
    CollectEDGARApp();

protected:

	void initialize(Application& self) override;
	void uninitialize() override;
	void reinitialize(Application& self) override;

	void defineOptions(Poco::Util::OptionSet& options) override;

	void handleHelp(const std::string& name, const std::string& value);

	void handleDefine(const std::string& name, const std::string& value);

	void handleConfig(const std::string& name, const std::string& value);

	void displayHelp();

	void defineProperty(const std::string& def);

	int main(const ArgVec& args) override;

	void printProperties(const std::string& base);

	void	Do_Main ();
	void	Do_StartUp ();
	void	Do_CheckArgs ();
	void	Do_Run ();
	void	Do_Quit ();

	void Do_Run_DailyIndexFiles();
	void Do_Run_QuarterlyIndexFiles();
	void Do_Run_TickerLookup();
	void Do_Run_TickerFileLookup();

	void Do_TickerMap_Setup();

		// ====================  DATA MEMBERS  =======================================

private:

	struct comma_list_parser
	{
		std::vector<std::string>& destination_;
		std::string seperator_;

		comma_list_parser(std::vector<std::string>& destination, const std::string& seperator)
			: destination_(destination), seperator_{seperator} {}

		void parse_string(const std::string& comma_list);
	};
    class LogLevelValidator : public Poco::Util::Validator
    {
        LogLevelValidator() : Validator() {}

        virtual void Validate(const Poco::Util::Option& option, const std::string& value);
    };

    // a set of functions to be used to capture values from the command line.
    // called from the options handling code.
    // there should be a better way to do this but this works for now.

    void inline store_begin_date(const std::string& name, const std::string& value) { begin_date_ = bg::from_string(value); }
    void inline store_end_date(const std::string& name, const std::string& value) { end_date_ = bg::from_string(value); }
    void inline store_index_dir(const std::string& name, const std::string& value) { local_index_file_directory_ = value; }
    void inline store_form_dir(const std::string& name, const std::string& value) { local_form_file_directory_ = value; }
    void inline store_HTTPS_host(const std::string& name, const std::string& value) { HTTPS_host_ = value; }
    // void inline store_login_ID(const std::string& name, const std::string& value) { login_ID_ = value; }

    void inline store_log_level(const std::string& name, const std::string& value) { logging_level_ = value; }
    void inline store_mode(const std::string& name, const std::string& value) { mode_ = value; }
    void inline store_form(const std::string& name, const std::string& value) { form_ = value; }
    void inline store_ticker(const std::string& name, const std::string& value) { ticker_ = value; }
    void inline store_log_path(const std::string& name, const std::string& value) { log_file_path_name_ = value; }
    void inline store_ticker_cache(const std::string& name, const std::string& value) { ticker_cache_file_name_ = value; }
    void inline store_ticker_file(const std::string& name, const std::string& value) { ticker_list_file_name_ = value; }
    void inline store_replace_index_files(const std::string& name, const std::string& value) { replace_index_files_ = true; }
    void inline store_replace_form_files(const std::string& name, const std::string& value) { replace_form_files_ = true; }
    void inline store_index_only(const std::string& name, const std::string& value) { index_only_ = true; }
    void inline store_pause(const std::string& name, const std::string& value) { pause_ = std::stoi(value); }
    void inline store_max(const std::string& name, const std::string& value) { max_forms_to_download_ = std::stoi(value); }
    void inline store_concurrency_limit(const std::string& name, const std::string& value) { max_at_a_time_ = std::stoi(value); }

		// ====================  DATA MEMBERS  =======================================

	TickerConverter ticker_converter_;

    Poco::AutoPtr<Poco::Channel> logger_file_;

	bg::date begin_date_;
	bg::date end_date_;

	std::string mode_{"daily"};
	std::string form_{"10-Q"};
	std::string ticker_;
	// std::string login_ID_;
    std::string HTTPS_host_{"https://www.sec.gov"};
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

}; // -----  end of class CollectEDGARApp  -----

#endif /* COLLECTEDGARAPP_H_ */
