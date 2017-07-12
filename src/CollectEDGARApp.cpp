// =====================================================================================
//
//       Filename:  CollectEDGARApp.cpp
//
//    Description:  main application
//
//        Version:  1.0
//        Created:  01/17/2014 11:18:10 AM
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


//--------------------------------------------------------------------------------------
//       Class:  CollectEDGARApp
//      Method:  CollectEDGARApp
// Description:  constructor
//--------------------------------------------------------------------------------------

#include <random>		//	just for initial development.  used in Quarterly form retrievals

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Poco/ConsoleChannel.h"
#include "Poco/SimpleFileChannel.h"
#include "Poco/LogStream.h"
#include "Poco/Util/OptionException.h"

#include "CollectEDGARApp.h"

#include "HTTPS_Downloader.h"
#include "DailyIndexFileRetriever.h"
#include "FormFileRetriever.h"
#include "QuarterlyIndexFileRetriever.h"


CollectEDGARApp::CollectEDGARApp (int argc, char* argv[])
	: Poco::Util::Application(argc, argv),
    ticker_converter_{logger()},
    mode_{"daily"},
    form_{"10-Q"},
    HTTPS_host_{"https://www.sec.gov"},
    logging_level_{"information"},
    pause_{0},
    max_forms_to_download_{-1},
    replace_index_files_{false},
    replace_form_files_{false},
	index_only_{false},
    help_requested_{false}

{
}  // -----  end of method CollectEDGARApp::CollectEDGARApp  (constructor)  -----

CollectEDGARApp::CollectEDGARApp (void)
	: Poco::Util::Application(),
    ticker_converter_{logger()},
    mode_{"daily"},
    form_{"10-Q"},
    HTTPS_host_{"https://www.sec.gov"},
    logging_level_{"information"},
    pause_{0},
    max_forms_to_download_{-1},
    replace_index_files_{false},
    replace_form_files_{false},
	index_only_{false},
    help_requested_{false}

{
}

void CollectEDGARApp::initialize(Application& self)
{
	loadConfiguration(); // load default configuration files, if present
	Application::initialize(self);
	// add your own initialization code here

	if (! log_file_path_name_.empty())
	{
        logger_file_ = new Poco::SimpleFileChannel;
        logger_file_->setProperty("path", log_file_path_name_.string());
        logger_file_->setProperty("rotation", "2 M");
	}
    else
    {
        logger_file_ = new Poco::ConsoleChannel;
    }

    decltype(auto) the_logger = Poco::Logger::root();
    the_logger.setChannel(logger_file_);
    the_logger.setLevel(logging_level_);

    setLogger(the_logger);
    // set log level here since options will have been parsed before we get here.
}

void  CollectEDGARApp::uninitialize()
{
	// add your own uninitialization code here
	Application::uninitialize();
}

void  CollectEDGARApp::reinitialize(Application& self)
{
	Application::reinitialize(self);
	// add your own reinitialization code here

	if (! log_file_path_name_.empty())
	{
        logger_file_ = new Poco::SimpleFileChannel;
        logger_file_->setProperty("path", log_file_path_name_.string());
        logger_file_->setProperty("rotation", "2 M");
	}
    else
    {
        logger_file_ = new Poco::ConsoleChannel;
    }

    decltype(auto) the_logger = Poco::Logger::root();
    the_logger.setChannel(logger_file_);

	the_logger.information("\n\n*** Restarting run " + boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time()) + " ***\n");

    setLogger(the_logger);
}

void  CollectEDGARApp::defineOptions(Poco::Util::OptionSet& options)
{
	Application::defineOptions(options);

	options.addOption(
		Poco::Util::Option("help", "h", "display help information on command line arguments")
			.required(false)
			.repeatable(false)
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::handleHelp)));

	options.addOption(
		Poco::Util::Option("gtest_filter", "", "select which tests to run.")
			.required(false)
			.repeatable(true)
			.argument("name=value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::handleDefine)));

	options.addOption(
		Poco::Util::Option("begin-date", "", "retrieve files with dates greater than or equal to.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_begin_date)));

	options.addOption(
		Poco::Util::Option("end-date", "", "retrieve files with dates less than or equal to.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_end_date)));

	options.addOption(
		Poco::Util::Option("index-dir", "", "directory index files are downloaded to.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_index_dir)));

	options.addOption(
		Poco::Util::Option("form-dir", "", "directory form files are downloaded to.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_form_dir)));

	options.addOption(
		Poco::Util::Option("host", "", "Server address to use for EDGAR.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_HTTPS_host)));

	// options.addOption(
	// 	Poco::Util::Option("login", "", "email address to use for anonymous login to EDGAR.")
	// 		.required(true)
	// 		.repeatable(false)
	// 		.argument("value")
	// 		.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_login_ID)));

	options.addOption(
		Poco::Util::Option("mode", "", "'daily' or 'quarterly' for index files, 'ticker-only'. Default is 'daily'.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_mode)));

	options.addOption(
		Poco::Util::Option("form", "", "name of form type[s] we are downloading. Default is '10-Q'.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_form)));

	options.addOption(
		Poco::Util::Option("ticker", "", "ticker[s] to lookup and filter form downloads.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_ticker)));

	options.addOption(
		Poco::Util::Option("log-path", "", "path name for log file.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_log_path)));

	options.addOption(
		Poco::Util::Option("ticker-cache", "", "path name for ticker-to-CIK cache file.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_ticker_cache)));

	options.addOption(
		Poco::Util::Option("ticker-file", "", "path name for file with list of ticker symbols to convert to CIKs.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_ticker_file)));

	options.addOption(
		Poco::Util::Option("replace-index-files", "", "over write local index files if specified. Default is 'false'")
			.required(false)
			.repeatable(false)
			.noArgument()
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_replace_index_files)));

	options.addOption(
		Poco::Util::Option("replace-form-files", "", "over write local form files if specified. Default is 'false'")
			.required(false)
			.repeatable(false)
			.noArgument()
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_replace_form_files)));

	options.addOption(
		Poco::Util::Option("index-only", "", "do not download form files.. Default is 'false'")
			.required(false)
			.repeatable(false)
			.noArgument()
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_index_only)));

	options.addOption(
		Poco::Util::Option("pause", "", "how many seconds to wait between downloads. Default: 1 second.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_pause)));

	options.addOption(
		Poco::Util::Option("max", "", "Maximun number of forms to download -- mainly for testing. Default of -1 means no limit.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_max)));

	options.addOption(
		Poco::Util::Option("log-level", "l", "logging level. Must be 'none|error|information|debug'. Default is 'information'.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectEDGARApp>(this, &CollectEDGARApp::store_log_level)));


	/* options.addOption( */
	/* 	Option("define", "D", "define a configuration property") */
	/* 		.required(false) */
	/* 		.repeatable(true) */
	/* 		.argument("name=value") */
	/* 		.callback(OptionCallback<EDGAR_UnitTest>(this, &EDGAR_UnitTest::handleDefine))); */

	/* options.addOption( */
	/* 	Option("config-file", "f", "load configuration data from a file") */
	/* 		.required(false) */
	/* 		.repeatable(true) */
	/* 		.argument("file") */
	/* 		.callback(OptionCallback<EDGAR_UnitTest>(this, &EDGAR_UnitTest::handleConfig))); */

	/* options.addOption( */
	/* 	Option("bind", "b", "bind option value to test.property") */
	/* 		.required(false) */
	/* 		.repeatable(false) */
	/* 		.argument("value") */
	/* 		.binding("test.property")); */
}

void  CollectEDGARApp::handleHelp(const std::string& name, const std::string& value)
{
	help_requested_ = true;
	displayHelp();
	stopOptionsProcessing();
}

void  CollectEDGARApp::handleDefine(const std::string& name, const std::string& value)
{
	defineProperty(value);
}

void  CollectEDGARApp::handleConfig(const std::string& name, const std::string& value)
{
	loadConfiguration(value);
}

void CollectEDGARApp::displayHelp()
{
	Poco::Util::HelpFormatter helpFormatter(options());
	helpFormatter.setCommand(commandName());
	helpFormatter.setUsage("OPTIONS");
	helpFormatter.setHeader("Program which manages the download of selected files from the SEC's EDGAR FTP server.");
	helpFormatter.format(std::cout);
}

void  CollectEDGARApp::defineProperty(const std::string& def)
{
	std::string name;
	std::string value;
	std::string::size_type pos = def.find('=');
	if (pos != std::string::npos)
	{
		name.assign(def, 0, pos);
		value.assign(def, pos + 1, def.length() - pos);
	}
	else name = def;
	config().setString(name, value);
}

int  CollectEDGARApp::main(const ArgVec& args)
{
	if (!help_requested_)
	{
		// logger().information("Command line:");
		// std::ostringstream ostr;
		// for (ArgVec::const_iterator it = argv().begin(); it != argv().end(); ++it)
		// {
		// 	ostr << *it << ' ';
		// }
		// logger().information(ostr.str());
		// logger().information("Arguments to main():");
		// for (ArgVec::const_iterator it = args.begin(); it != args.end(); ++it)
		// {
		// 	logger().information(*it);
		// }
		// logger().information("Application properties:");
		// printProperties("");

        Do_Main();
	}
	return Application::EXIT_OK;
}

void  CollectEDGARApp::printProperties(const std::string& base)
{
	Poco::Util::AbstractConfiguration::Keys keys;
	config().keys(base, keys);
	if (keys.empty())
	{
		if (config().hasProperty(base))
		{
			std::string msg;
			msg.append(base);
			msg.append(" = ");
			msg.append(config().getString(base));
			logger().information(msg);
		}
	}
	else
	{
		for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = keys.begin(); it != keys.end(); ++it)
		{
			std::string fullKey = base;
			if (!fullKey.empty()) fullKey += '.';
			fullKey.append(*it);
			printProperties(fullKey);
		}
	}
}

void CollectEDGARApp::Do_Main(void)

{
	Do_StartUp();
	Do_CheckArgs();
	Do_Run();
    Do_Quit();
}


void CollectEDGARApp::Do_StartUp (void)
{
	logger().information("\n\n*** Begin run " + boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time()) + " ***\n");
}		// -----  end of method CollectEDGARApp::Do_StartUp  -----

void CollectEDGARApp::Do_CheckArgs (void)
{
	poco_assert_msg(mode_ == "daily" || mode_ == "quarterly" || mode_ == "ticker-only", ("Mode must be either 'daily','quarterly' or 'ticker-only' ==> " + mode_).c_str());

	//	the user may specify multiple stock tickers in a comma delimited list. We need to parse the entries out
	//	of that list and place into ultimate home.  If just a single entry, copy it to our form list destination too.

	if (! ticker_.empty())
	{
		comma_list_parser x(ticker_list_, ",");
		x.parse_string(ticker_);
	}

	if (! ticker_list_file_name_.empty())
    {
		poco_assert_msg(! ticker_cache_file_name_.empty(), "You must use a cache file when using a file of ticker symbols.");
    }

	if (mode_ == "ticker-only")
		return;

	poco_assert_msg(begin_date_ != bg::date(), "Must specify 'begin-date' for index and/or form downloads.");

	if (end_date_ == bg::date())
		end_date_ = begin_date_;

	poco_assert_msg(! local_index_file_directory_.empty(), "Must specify 'index-dir' when downloading index and/or forms.");
	poco_assert_msg(index_only_ || ! local_form_file_directory_.empty(), "Must specify 'form-dir' when not using 'index-only' option.");

	//	the user may specify multiple form types in a comma delimited list. We need to parse the entries out
	//	of that list and place into ultimate home.  If just a single entry, copy it to our form list destination too.

	if (! form_.empty())
	{
		comma_list_parser x(form_list_, ",");
		x.parse_string(form_);
	}

}		// -----  end of method CollectEDGARApp::Do_CheckArgs  -----

void CollectEDGARApp::Do_Run (void)
{
	if (! ticker_cache_file_name_.empty())
		ticker_converter_.UseCacheFile(ticker_cache_file_name_);

	if (! ticker_list_file_name_.empty())
		Do_Run_TickerFileLookup();

	if (mode_ == "ticker-only")
		Do_Run_TickerLookup();
	else if (mode_ == "daily")
		Do_Run_DailyIndexFiles();
	else
		Do_Run_QuarterlyIndexFiles();

}		// -----  end of method CollectEDGARApp::Do_Run  -----

void CollectEDGARApp::Do_Run_TickerLookup (void)
{
	for (const auto& ticker : ticker_list_)
		ticker_converter_.ConvertTickerToCIK(ticker);

}		// -----  end of method CollectEDGARApp::Do_Run_tickerLookup  -----

void CollectEDGARApp::Do_Run_TickerFileLookup (void)
{
	ticker_converter_.ConvertTickerFileToCIKs(ticker_list_file_name_, pause_);

}		// -----  end of method CollectEDGARApp::Do_Run_TickerFileLookup  -----

void CollectEDGARApp::Do_Run_DailyIndexFiles (void)
{
	//FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	HTTPS_Downloader a_server{HTTPS_host_};
	DailyIndexFileRetriever idxFileRet{a_server, "/edgar/daily-index", logger()};

	Do_TickerMap_Setup();

	if (begin_date_ == end_date_)
	{
		idxFileRet.FindIndexFileNameNearestDate(this->begin_date_);
		idxFileRet.HierarchicalCopyRemoteIndexFileTo(this->local_index_file_directory_, replace_index_files_);

		if (! index_only_)
		{
			decltype(auto) local_daily_index_file_name = idxFileRet.GetLocalIndexFilePath();
			FormFileRetriever form_file_getter{a_server, logger(), pause_};
			decltype(auto) form_file_list = form_file_getter.FindFilesForForms(form_list_, local_daily_index_file_name, ticker_map_);

            if (max_forms_to_download_ > -1)
            {
    			for (auto& elem : form_file_list)
    			{
                    // I don't remember why I'm doing this...
                    // If we are downloading only some of the files possible
                    // to download, then take a random selection of those files.

        			if (elem.second.size() > max_forms_to_download_)
                    {
            			std::default_random_engine dre;
            			std::shuffle(elem.second.begin(), elem.second.end(), dre);
        				elem.second.resize(max_forms_to_download_);
                    }
    			}
            }
			form_file_getter.RetrieveSpecifiedFiles(form_file_list, this->local_form_file_directory_,
					replace_form_files_);
		}
	}
	else
	{
		idxFileRet.FindIndexFileNamesForDateRange(begin_date_, end_date_);
		idxFileRet.HierarchicalCopyIndexFilesForDateRangeTo(local_index_file_directory_, replace_index_files_);

		if (! index_only_)
		{
			decltype(auto) index_file_list = idxFileRet.GetfRemoteIndexFileNamesForDateRange();
			FormFileRetriever form_file_getter{a_server, logger(), pause_};
			decltype(auto) form_file_list = form_file_getter.FindFilesForForms(form_list_, local_index_file_directory_, index_file_list,
					ticker_map_);

            if (max_forms_to_download_ > -1)
            {
                // same comment here as above for single file.

    			for (auto& elem : form_file_list)
    			{
        			if (elem.second.size() > max_forms_to_download_)
                    {
            			std::default_random_engine dre;
            			std::shuffle(elem.second.begin(), elem.second.end(), dre);
        				elem.second.resize(max_forms_to_download_);
                    }
    			}
            }
			form_file_getter.RetrieveSpecifiedFiles(form_file_list, local_form_file_directory_, replace_form_files_);
		}
	}

}		// -----  end of method CollectEDGARApp::Do_Run_DailyIndexFiles  -----

void CollectEDGARApp::Do_Run_QuarterlyIndexFiles (void)
{
	Do_TickerMap_Setup();

	HTTPS_Downloader a_server{HTTPS_host_};
	QuarterlyIndexFileRetriever idxFileRet{a_server, "/edgar/full-index", logger()};

	if (begin_date_ == end_date_)
	{
		idxFileRet.MakeQuarterIndexPathName(begin_date_);
		idxFileRet.HierarchicalCopyRemoteIndexFileTo(this->local_index_file_directory_, replace_index_files_);

		if (! index_only_)
		{
			decltype(auto) local_quarterly_index_file_name = idxFileRet.GetLocalIndexFilePath();
			FormFileRetriever form_file_getter{a_server, logger(), pause_};
			decltype(auto) form_file_list = form_file_getter.FindFilesForForms(form_list_, local_quarterly_index_file_name, ticker_map_);

            if (max_forms_to_download_ > -1)
            {
    			for (auto& elem : form_file_list)
    			{
        			if (elem.second.size() > max_forms_to_download_)
                    {
            			std::default_random_engine dre;
            			std::shuffle(elem.second.begin(), elem.second.end(), dre);
        				elem.second.resize(max_forms_to_download_);
                    }
    			}
            }
			form_file_getter.RetrieveSpecifiedFiles(form_file_list, this->local_form_file_directory_,
					replace_form_files_);
		}
	}
	else
	{
		idxFileRet.MakeIndexFileNamesForDateRange(begin_date_, end_date_);
		idxFileRet.HierarchicalCopyIndexFilesForDateRangeTo(local_index_file_directory_, replace_index_files_);

		if (! index_only_)
		{
			decltype(auto) index_file_list = idxFileRet.GetLocalIndexFileNamesForDateRange();
			FormFileRetriever form_file_getter{a_server, logger(), pause_};
			decltype(auto) form_file_list = form_file_getter.FindFilesForForms(form_list_, local_index_file_directory_, index_file_list,
					ticker_map_);

            if (max_forms_to_download_ > -1)
            {
    			for (auto& elem : form_file_list)
    			{
        			if (elem.second.size() > max_forms_to_download_)
                    {
            			std::default_random_engine dre;
            			std::shuffle(elem.second.begin(), elem.second.end(), dre);
        				elem.second.resize(max_forms_to_download_);
                    }
    			}
            }
			form_file_getter.RetrieveSpecifiedFiles(form_file_list, local_form_file_directory_, replace_form_files_);
		}
	}

}		// -----  end of method CollectEDGARApp::Do_Run_QuarterlyIndexFiles  -----

void CollectEDGARApp::Do_TickerMap_Setup (void)
{
	for (const auto& ticker : ticker_list_)
		ticker_map_[ticker] = ticker_converter_.ConvertTickerToCIK(ticker);

}		// -----  end of method CollectEDGARApp::Do_TickerMap_Setup  -----

void CollectEDGARApp::Do_Quit (void)
{
	if (! ticker_cache_file_name_.empty())
		ticker_converter_.SaveCIKDataToFile();

	logger().information("\n\n*** End run " + boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time()) + " ***\n");
}		// -----  end of method CollectEDGARApp::Do_Quit  -----

void CollectEDGARApp::comma_list_parser::parse_string (const std::string& comma_list)
{
	boost::algorithm::split(destination_, comma_list, boost::algorithm::is_any_of(seperator_));

	return ;
}		// -----  end of method comma_list_parser::parse_string  -----

void CollectEDGARApp::LogLevelValidator::Validate(const Poco::Util::Option& option, const std::string& value)
{
    if (value != "error" && value != "none" && value != "information" && value != "debug")
        throw Poco::Util::OptionException("Log level must be: 'none|error|information|debug'");
}
