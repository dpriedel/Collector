// =====================================================================================
//
//       Filename:  CollectorApp.cpp
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


//--------------------------------------------------------------------------------------
//       Class:  CollectorApp
//      Method:  CollectorApp
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
#include "Poco/Util/AbstractConfiguration.h"

#include "CollectorApp.h"

#include "HTTPS_Downloader.h"
#include "DailyIndexFileRetriever.h"
#include "FormFileRetriever.h"
#include "QuarterlyIndexFileRetriever.h"


CollectorApp::CollectorApp (int argc, char* argv[])
	: Poco::Util::Application(argc, argv),
    ticker_converter_{logger()}

{
}  // -----  end of method CollectorApp::CollectorApp  (constructor)  -----

CollectorApp::CollectorApp ()
	: Poco::Util::Application(),
    ticker_converter_{logger()}

{
}

void CollectorApp::initialize(Application& self)
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

	the_logger.information("Command line:");
	std::ostringstream ostr;
	for (const auto& it : argv())
	{
		ostr << it << ' ';
	}
	the_logger.information(ostr.str());
	the_logger.information("Arguments to main():");
    auto args = argv();
	for (const auto& it : argv())
	{
		the_logger.information(it);
	}
	the_logger.information("Application properties:");
	printProperties("");

    // set log level here since options will have been parsed before we get here.
}

void CollectorApp::printProperties(const std::string& base)
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
		for (const auto& it : keys)
		{
			std::string fullKey = base;
			if (!fullKey.empty()) fullKey += '.';
			fullKey.append(it);
			printProperties(fullKey);
		}
	}
}

void  CollectorApp::uninitialize()
{
	// add your own uninitialization code here
	Application::uninitialize();
}

void  CollectorApp::reinitialize(Application& self)
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

void  CollectorApp::defineOptions(Poco::Util::OptionSet& options)
{
	Application::defineOptions(options);

	options.addOption(
		Poco::Util::Option("help", "h", "display help information on command line arguments")
			.required(false)
			.repeatable(false)
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::handleHelp)));

	options.addOption(
		Poco::Util::Option("gtest_filter", "", "select which tests to run.")
			.required(false)
			.repeatable(true)
			.argument("name=value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::handleDefine)));

	options.addOption(
		Poco::Util::Option("begin-date", "", "retrieve files with dates greater than or equal to.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_begin_date)));

	options.addOption(
		Poco::Util::Option("end-date", "", "retrieve files with dates less than or equal to.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_end_date)));

	options.addOption(
		Poco::Util::Option("index-dir", "", "directory index files are downloaded to.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_index_dir)));

	options.addOption(
		Poco::Util::Option("form-dir", "", "directory form files are downloaded to.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_form_dir)));

	options.addOption(
		Poco::Util::Option("host", "", "Server address to use for SEC.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_HTTPS_host)));

	options.addOption(
		Poco::Util::Option("port", "", "Server port number to use for SEC. Default value is '443'.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_HTTPS_port)));

	// options.addOption(
	// 	Poco::Util::Option("login", "", "email address to use for anonymous login to SEC.")
	// 		.required(true)
	// 		.repeatable(false)
	// 		.argument("value")
	// 		.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_login_ID)));

	options.addOption(
		Poco::Util::Option("mode", "", "'daily' or 'quarterly' for index files, 'ticker-only'. Default is 'daily'.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_mode)));

	options.addOption(
		Poco::Util::Option("form", "", "name of form type[s] we are downloading. Default is '10-Q'.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_form)));

	options.addOption(
		Poco::Util::Option("ticker", "", "ticker[s] to lookup and filter form downloads.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_ticker)));

	options.addOption(
		Poco::Util::Option("log-path", "", "path name for log file.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_log_path)));

	options.addOption(
		Poco::Util::Option("ticker-cache", "", "path name for ticker-to-CIK cache file.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_ticker_cache)));

	options.addOption(
		Poco::Util::Option("ticker-file", "", "path name for file with list of ticker symbols to convert to CIKs.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_ticker_file)));

	options.addOption(
		Poco::Util::Option("replace-index-files", "", "over write local index files if specified. Default is 'false'")
			.required(false)
			.repeatable(false)
			.noArgument()
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_replace_index_files)));

	options.addOption(
		Poco::Util::Option("replace-form-files", "", "over write local form files if specified. Default is 'false'")
			.required(false)
			.repeatable(false)
			.noArgument()
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_replace_form_files)));

	options.addOption(
		Poco::Util::Option("index-only", "", "do not download form files.. Default is 'false'")
			.required(false)
			.repeatable(false)
			.noArgument()
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_index_only)));

	options.addOption(
		Poco::Util::Option("pause", "", "how many seconds to wait between downloads. Default: 1 second.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_pause)));

	options.addOption(
		Poco::Util::Option("max", "", "Maximun number of forms to download -- mainly for testing. Default of -1 means no limit.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_max)));

	options.addOption(
		Poco::Util::Option("log-level", "l", "logging level. Must be 'none|error|information|debug'. Default is 'information'.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_log_level)));

	options.addOption(
		Poco::Util::Option("concurrent", "k", "Maximun number of concurrent downloads. Default of 10.")
			.required(false)
			.repeatable(false)
			.argument("value")
			.callback(Poco::Util::OptionCallback<CollectorApp>(this, &CollectorApp::store_concurrency_limit)));

	/* options.addOption( */
	/* 	Option("define", "D", "define a configuration property") */
	/* 		.required(false) */
	/* 		.repeatable(true) */
	/* 		.argument("name=value") */
	/* 		.callback(OptionCallback<SEC_UnitTest>(this, &SEC_UnitTest::handleDefine))); */

	/* options.addOption( */
	/* 	Option("config-file", "f", "load configuration data from a file") */
	/* 		.required(false) */
	/* 		.repeatable(true) */
	/* 		.argument("file") */
	/* 		.callback(OptionCallback<SEC_UnitTest>(this, &SEC_UnitTest::handleConfig))); */

	/* options.addOption( */
	/* 	Option("bind", "b", "bind option value to test.property") */
	/* 		.required(false) */
	/* 		.repeatable(false) */
	/* 		.argument("value") */
	/* 		.binding("test.property")); */
}

void  CollectorApp::handleHelp(const std::string& name, const std::string& value)
{
	help_requested_ = true;
	displayHelp();
	stopOptionsProcessing();
}

void  CollectorApp::handleDefine(const std::string& name, const std::string& value)
{
	defineProperty(value);
}

void  CollectorApp::handleConfig(const std::string& name, const std::string& value)
{
	loadConfiguration(value);
}

void CollectorApp::displayHelp()
{
	Poco::Util::HelpFormatter helpFormatter(options());
	helpFormatter.setCommand(commandName());
	helpFormatter.setUsage("OPTIONS");
	helpFormatter.setHeader("Program which manages the download of selected files from the SEC's SEC FTP server.");
	helpFormatter.format(std::cout);
}

void  CollectorApp::defineProperty(const std::string& def)
{
	std::string name;
	std::string value;
	std::string::size_type pos = def.find('=');
	if (pos != std::string::npos)
	{
		name.assign(def, 0, pos);
		value.assign(def, pos + 1, def.length() - pos);
	}
	else
    {
        name = def;
    }
	config().setString(name, value);
}

int  CollectorApp::main(const ArgVec& args)
{
	if (!help_requested_)
	{
        Do_Main();
	}
	return Application::EXIT_OK;
}

void CollectorApp::Do_Main()

{
	Do_StartUp();
    try
    {
        Do_CheckArgs();
        Do_Run();
    }
	catch (Poco::AssertionViolationException& e)
	{
		std::cout << e.displayText() << '\n';
	}
    catch (std::exception& e)
    {
        std::cout << "Problem collecting files: " << e.what() << '\n';
    }
    catch (...)
    {
        std::cout << "Unknown problem collecting files." << '\n';
    }

    Do_Quit();
}


void CollectorApp::Do_StartUp ()
{
	logger().information("\n\n*** Begin run " + boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time()) + " ***\n");
}		// -----  end of method CollectorApp::Do_StartUp  -----

void CollectorApp::Do_CheckArgs ()
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

}		// -----  end of method CollectorApp::Do_CheckArgs  -----

void CollectorApp::Do_Run ()
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

	if (! ticker_cache_file_name_.empty())
		ticker_converter_.SaveCIKDataToFile();

}		// -----  end of method CollectorApp::Do_Run  -----

void CollectorApp::Do_Run_TickerLookup ()
{
	for (const auto& ticker : ticker_list_)
		ticker_converter_.ConvertTickerToCIK(ticker);

}		// -----  end of method CollectorApp::Do_Run_tickerLookup  -----

void CollectorApp::Do_Run_TickerFileLookup ()
{
	ticker_converter_.ConvertTickerFileToCIKs(ticker_list_file_name_, pause_);

}		// -----  end of method CollectorApp::Do_Run_TickerFileLookup  -----

void CollectorApp::Do_Run_DailyIndexFiles ()
{
	//FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	HTTPS_Downloader a_server{HTTPS_host_, HTTPS_port_};
	DailyIndexFileRetriever idxFileRet{a_server, "/Archives/edgar/daily-index"};

	Do_TickerMap_Setup();

	if (begin_date_ == end_date_)
	{
		auto remote_daily_index_file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(this->begin_date_);
		auto local_daily_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(remote_daily_index_file_name, this->local_index_file_directory_, replace_index_files_);

		if (! index_only_)
		{
			FormFileRetriever form_file_getter{a_server};
			decltype(auto) form_file_list = form_file_getter.FindFilesForForms(form_list_, local_daily_index_file_name, ticker_map_);

            if (max_forms_to_download_ > -1)
            {
    			for (auto& [form, files] : form_file_list)
    			{
                    // I don't remember why I'm doing this...it's for testing !!
                    // If we are downloading only some of the files possible
                    // to download, then take a random selection of those files.

        			if (files.size() > max_forms_to_download_)
                    {
            			std::default_random_engine dre;
            			std::shuffle(files.begin(), files.end(), dre);
        				files.resize(max_forms_to_download_);
                    }
    			}
            }
			form_file_getter.ConcurrentlyRetrieveSpecifiedFiles(form_file_list, this->local_form_file_directory_, max_at_a_time_, replace_form_files_);
		}
	}
	else
	{
		auto remote_daily_index_file_list = idxFileRet.FindRemoteIndexFileNamesForDateRange(begin_date_, end_date_);
		auto local_daily_index_file_list = idxFileRet.ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo(remote_daily_index_file_list, local_index_file_directory_, max_at_a_time_, replace_index_files_);

		if (! index_only_)
		{
			FormFileRetriever form_file_getter{a_server};
			decltype(auto) form_file_list = form_file_getter.FindFilesForForms(form_list_, local_daily_index_file_list, ticker_map_);

            if (max_forms_to_download_ > -1)
            {
                // same comment here as above for single file.

    			for (auto& [form, files] : form_file_list)
    			{
        			if (files.size() > max_forms_to_download_)
                    {
            			std::default_random_engine dre;
            			std::shuffle(files.begin(), files.end(), dre);
        				files.resize(max_forms_to_download_);
                    }
    			}
            }
			form_file_getter.ConcurrentlyRetrieveSpecifiedFiles(form_file_list, local_form_file_directory_, max_at_a_time_, replace_form_files_);
		}
	}

}		// -----  end of method CollectorApp::Do_Run_DailyIndexFiles  -----

void CollectorApp::Do_Run_QuarterlyIndexFiles ()
{
	Do_TickerMap_Setup();

	HTTPS_Downloader a_server{HTTPS_host_, HTTPS_port_};
	QuarterlyIndexFileRetriever idxFileRet{a_server, "/Archives/edgar/full-index", logger()};

	if (begin_date_ == end_date_)
	{
		auto remote_quarterly_index_file_name = idxFileRet.MakeQuarterlyIndexPathName(begin_date_);
		auto local_quarterly_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(remote_quarterly_index_file_name, this->local_index_file_directory_, replace_index_files_);

		if (! index_only_)
		{
			FormFileRetriever form_file_getter{a_server};
			decltype(auto) form_file_list = form_file_getter.FindFilesForForms(form_list_, local_quarterly_index_file_name, ticker_map_);

            if (max_forms_to_download_ > -1)
            {
    			for (auto& [form, files] : form_file_list)
    			{
        			if (files.size() > max_forms_to_download_)
                    {
            			std::default_random_engine dre;
            			std::shuffle(files.begin(), files.end(), dre);
        				files.resize(max_forms_to_download_);
                    }
    			}
            }
			form_file_getter.ConcurrentlyRetrieveSpecifiedFiles(form_file_list, this->local_form_file_directory_, max_at_a_time_, replace_form_files_);
		}
	}
	else
	{
		auto remote_index_file_list = idxFileRet.MakeIndexFileNamesForDateRange(begin_date_, end_date_);
		auto local_index_file_list = idxFileRet.ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo(remote_index_file_list, local_index_file_directory_, max_at_a_time_, replace_index_files_);

		if (! index_only_)
		{
			FormFileRetriever form_file_getter{a_server};
			decltype(auto) form_file_list = form_file_getter.FindFilesForForms(form_list_, local_index_file_list, ticker_map_);

            if (max_forms_to_download_ > -1)
            {
    			for (auto& [form, files] : form_file_list)
    			{
        			if (files.size() > max_forms_to_download_)
                    {
            			std::default_random_engine dre;
            			std::shuffle(files.begin(), files.end(), dre);
        				files.resize(max_forms_to_download_);
                    }
    			}
            }
			form_file_getter.ConcurrentlyRetrieveSpecifiedFiles(form_file_list, local_form_file_directory_, max_at_a_time_, replace_form_files_);
		}
	}

}		// -----  end of method CollectorApp::Do_Run_QuarterlyIndexFiles  -----

void CollectorApp::Do_TickerMap_Setup ()
{
	for (const auto& ticker : ticker_list_)
		ticker_map_[ticker] = ticker_converter_.ConvertTickerToCIK(ticker);

}		// -----  end of method CollectorApp::Do_TickerMap_Setup  -----

void CollectorApp::Do_Quit ()
{
	logger().information("\n\n*** End run " + boost::posix_time::to_simple_string(boost::posix_time::second_clock::local_time()) + " ***\n");
}		// -----  end of method CollectorApp::Do_Quit  -----

void CollectorApp::comma_list_parser::parse_string (const std::string& comma_list)
{
	boost::algorithm::split(destination_, comma_list, boost::algorithm::is_any_of(seperator_));

	return ;
}		// -----  end of method comma_list_parser::parse_string  -----

void CollectorApp::LogLevelValidator::Validate(const Poco::Util::Option& option, const std::string& value)
{
    if (value != "error" && value != "none" && value != "information" && value != "debug")
        throw Poco::Util::OptionException("Log level must be: 'none|error|information|debug'");
}
