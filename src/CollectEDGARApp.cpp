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

#include "CollectEDGARApp.h"
#include "TException.h"

#include "FTP_Connection.h"
#include "DailyIndexFileRetriever.h"
#include "FormFileRetriever.h"
#include "QuarterlyIndexFileRetriever.h"

CollectEDGARApp::CollectEDGARApp (int argc, char* argv[])
	: CApplication(argc, argv), saved_from_clog_{nullptr}, replace_index_files_{false}, replace_form_files_{false},
	index_only_{false}
{
}  // -----  end of method CollectEDGARApp::CollectEDGARApp  (constructor)  -----

CollectEDGARApp::CollectEDGARApp (int argc, char* argv[], const std::vector<std::string>& tokens)
	: CApplication{argc, argv, tokens}, saved_from_clog_{nullptr}, replace_index_files_{false}, replace_form_files_{false},
	index_only_{false}
	
{

}

void CollectEDGARApp::Do_StartUp (void)
{
	return;
}		// -----  end of method CollectEDGARApp::Do_StartUp  -----

void CollectEDGARApp::Do_CheckArgs (void)
{
	if (! log_file_path_name_.empty())
	{
		log_file_.open(log_file_path_name_.string(), std::ios_base::app);
		dfail_if_(! log_file_.is_open(), "Unable to open log file: ", log_file_path_name_.string());

		saved_from_clog_ = std::clog.rdbuf();
		std::clog.rdbuf(log_file_.rdbuf());

		std::clog << "\n\n*** Begin run " << boost::posix_time::second_clock::local_time() << " ***\n"; 
	}

	dthrow_if_(mode_ != "daily" && mode_ != "quarterly" && mode_ != "ticker-only", "Mode must be either 'daily','quarterly' or 'ticker-only' ==> ", mode_);
	
	//	the user may specify multiple stock tickers in a comma delimited list. We need to parse the entries out
	//	of that list and place into ultimate home.  If just a single entry, copy it to our form list destination too.

	if (! ticker_.empty())
	{
		comma_list_parser x(ticker_list_, ",");
		x.parse_string(ticker_);
	}	

	if (! ticker_list_file_name_.empty())
		dthrow_if_(ticker_cache_file_name_.empty(), "You must use a cache file when using a file of ticker symbols.");

	if (mode_ == "ticker-only")
		return;
	
	dthrow_if_(begin_date_ == bg::date(), "Must specify 'begin-date' for index and/or form downloads.");

	if (end_date_ == bg::date())
		end_date_ = begin_date_;

	dthrow_if_(local_index_file_directory_.empty(), "Must specify 'index-dir' when downloading index and/or forms.");
	dthrow_if_(! index_only_ && local_form_file_directory_.empty(), "Must specify 'form-dir' when not using 'index-only' option.");

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
	FTP_Server a_server{"ftp.sec.gov", "anonymous", login_ID_};
	DailyIndexFileRetriever idxFileRet{a_server};

	Do_TickerMap_Setup();

	if (begin_date_ == end_date_)
	{
		idxFileRet.FindIndexFileNameNearestDate(this->begin_date_);
		idxFileRet.RetrieveRemoteIndexFileTo(this->local_index_file_directory_, replace_index_files_);

		if (! index_only_)
		{
			auto local_daily_index_file_name = idxFileRet.GetLocalIndexFilePath();
			FormFileRetriever form_file_getter{a_server, pause_};
			auto form_file_list = form_file_getter.FindFilesForForms(form_list_, local_daily_index_file_name, ticker_map_);
			
			//	JUST FOR INITIAL DEVELOPMENT
			/* for (auto& elem : form_file_list) */
			/* { */
			/* 	std::default_random_engine dre; */
			/* 	std::shuffle(elem.second.begin(), elem.second.end(), dre); */
			/* 	if (elem.second.size() > 10) */
			/* 		elem.second.resize(10); */
			/* } */
			form_file_getter.RetrieveSpecifiedFiles(form_file_list, this->local_form_file_directory_,
					replace_form_files_);
		}
	}
	else
	{
		idxFileRet.FindIndexFileNamesForDateRange(begin_date_, end_date_);
		idxFileRet.RetrieveIndexFilesForDateRangeTo(local_index_file_directory_, replace_index_files_);

		if (! index_only_)
		{
			auto index_file_list = idxFileRet.GetfRemoteIndexFileNamesForDateRange();
			FormFileRetriever form_file_getter{a_server, pause_};
			auto form_file_list = form_file_getter.FindFilesForForms(form_list_, local_index_file_directory_, index_file_list,
					ticker_map_);
			
			//	JUST FOR INITIAL DEVELOPMENT
			/* for (auto& elem : form_file_list) */
			/* { */
			/* 	std::default_random_engine dre; */
			/* 	std::shuffle(elem.second.begin(), elem.second.end(), dre); */
			/* 	if (elem.second.size() > 10) */
			/* 		elem.second.resize(10); */
			/* } */
			form_file_getter.RetrieveSpecifiedFiles(form_file_list, local_form_file_directory_, replace_form_files_);
		}
	}

}		// -----  end of method CollectEDGARApp::Do_Run_DailyIndexFiles  -----

void CollectEDGARApp::Do_Run_QuarterlyIndexFiles (void)
{
	Do_TickerMap_Setup();

	//FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
	FTP_Server a_server{"ftp.sec.gov", "anonymous", login_ID_};
	QuarterlyIndexFileRetriever idxFileRet{a_server};

	if (begin_date_ == end_date_)
	{
		idxFileRet.MakeQuarterIndexPathName(begin_date_);
		idxFileRet.RetrieveRemoteIndexFileTo(this->local_index_file_directory_, replace_index_files_);

		if (! index_only_)
		{
			auto local_quarterly_index_file_name = idxFileRet.GetLocalIndexFilePath();
			FormFileRetriever form_file_getter{a_server, pause_};
			auto form_file_list = form_file_getter.FindFilesForForms(form_list_, local_quarterly_index_file_name, ticker_map_);

			//	JUST FOR INITIAL DEVELOPMENT
			/* for (auto& elem : form_file_list) */
			/* { */
			/* 	std::default_random_engine dre; */
			/* 	std::shuffle(elem.second.begin(), elem.second.end(), dre); */
			/* 	if (elem.second.size() > 10) */
			/* 		elem.second.resize(10); */
			/* } */
			form_file_getter.RetrieveSpecifiedFiles(form_file_list, this->local_form_file_directory_,
					replace_form_files_);
		}
	}
	else
	{
		idxFileRet.FindIndexFileNamesForDateRange(begin_date_, end_date_);
		idxFileRet.RetrieveIndexFilesForDateRangeTo(local_index_file_directory_, replace_index_files_);

		if (! index_only_)
		{
			auto index_file_list = idxFileRet.GetLocalIndexFileNamesForDateRange();
			FormFileRetriever form_file_getter{a_server, pause_};
			auto form_file_list = form_file_getter.FindFilesForForms(form_list_, local_index_file_directory_, index_file_list,
					ticker_map_);

			//	JUST FOR INITIAL DEVELOPMENT
			/* for (auto& elem : form_file_list) */
			/* { */
			/* 	std::default_random_engine dre; */
			/* 	std::shuffle(elem.second.begin(), elem.second.end(), dre); */
			/* 	if (elem.second.size() > 10) */
			/* 		elem.second.resize(10); */
			/* } */
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

	if (! log_file_path_name_.empty())
	{
		std::clog << "*** End run " << boost::posix_time::second_clock::local_time() << " ***\n"; 
		log_file_.close();

		std::clog.rdbuf(saved_from_clog_);
	}
}		// -----  end of method CollectEDGARApp::Do_Quit  -----

void CollectEDGARApp::Do_SetupProgramOptions (void)
{
	mNewOptions.add_options()
		("help,h",								"produce help message")
		("mode", 		po::value<std::string>(&this->mode_)->default_value("daily"), "'daily' or 'quarterly' for index files, 'ticker-only'")
		("begin-date",	po::value<bg::date>(&this->begin_date_)->default_value(bg::day_clock::local_day()), "retrieve files with dates greater than or equal to")
		("end-date",	po::value<bg::date>(&this->end_date_), "retrieve files with dates less than or equal to")
		("form",	po::value<std::string>(&this->form_)->default_value("10-Q"),	"name of form type[s] we are downloading")
		("ticker",	po::value<std::string>(&this->ticker_),	"ticker to lookup and filter form downloads")
		/* ("file,f",				po::value<std::string>(),	"name of file containing data for ticker. Default is stdin") */
		/* ("mode,m",				po::value<std::string>(),	"mode: either 'load' new data or 'update' existing data. Default is 'load'") */
		/* ("output,o",			po::value<std::string>(),	"output file name") */
		/* ("destination,d",		po::value<std::string>(),	"send data to file or DB. Default is 'stdout'.") */
		/* ("boxsize,b",			po::value<DprDecimal::DDecimal<16>>(),	"box step size. 'n', 'm.n'") */
		/* ("reversal,r",			po::value<int>(),			"reversal size in number of boxes. Default is 1") */
		/* ("scale",				po::value<std::string>(),	"'arithmetic', 'log'. Default is 'arithmetic'") */
		("index-dir",		po::value<fs::path>(&this->local_index_file_directory_),	"directory index files are downloaded to")
		("form-dir",		po::value<fs::path>(&this->local_form_file_directory_),	"directory form files are downloaded to")
		("log-path",		po::value<fs::path>(&this->log_file_path_name_),	"path name for log file")
		("ticker-cache",		po::value<fs::path>(&this->ticker_cache_file_name_),	"path name for ticker-to-CIK cache file")
		("ticker-file",		po::value<fs::path>(&this->ticker_list_file_name_),	"path name for file with list of ticker symbols to convert to CIKs")
		("replace-index-files",		po::value<bool>(&this->replace_index_files_)->implicit_value(true),	"over write local index files if specified")
		("replace-form-files",		po::value<bool>(&this->replace_form_files_)->implicit_value(true),	"over write local form files if specified")
		("index-only",		po::value<bool>(&this->index_only_)->implicit_value(true),	"do not download form files.")
		("pause,p", 		po::value<int>(&this->pause_)->default_value(1), "how long to wait between downloads. Default: 1 second.")
		("login", 		po::value<std::string>(&this->login_ID_)->required(), "email address to use for anonymous login to EDGAR")
		;

}		// -----  end of method CollectEDGARApp::Do_SetupProgramOptions  -----

void CollectEDGARApp::Do_ParseProgramOptions (po::parsed_options&)
{
}		// -----  end of method CollectEDGARApp::Do_ParseProgramOptions  -----

void CollectEDGARApp::comma_list_parser::parse_string (const std::string& comma_list)
{
	boost::algorithm::split(destination_, comma_list, boost::algorithm::is_any_of(seperator_));

	return ;
}		// -----  end of method comma_list_parser::parse_string  -----

