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

#include <iostream>
#include <random> //	just for initial development.  used in Quarterly form retrievals

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "CollectorApp.h"

#include "Collector_Utils.h"
#include "DailyIndexFileRetriever.h"
#include "FinancialStatementsAndNotes.h"
#include "FormFileRetriever.h"
#include "QuarterlyIndexFileRetriever.h"

/*
 *--------------------------------------------------------------------------------------
 *       Class:  CollectorApp
 *      Method:  CollectorApp
 * Description:  constructor
 *--------------------------------------------------------------------------------------
 */
CollectorApp::CollectorApp(int argc, char *argv[]) : mArgc{argc}, mArgv{argv}
{
    original_logger_ = spdlog::default_logger();
} /* -----  end of method CollectorApp::CollectorApp  (constructor)  ----- */

/*
 *--------------------------------------------------------------------------------------
 *       Class:  CollectorApp
 *      Method:  CollectorApp
 * Description:  constructor
 *--------------------------------------------------------------------------------------
 */
CollectorApp::CollectorApp(const std::vector<std::string> &tokens) : tokens_{tokens}
{
    original_logger_ = spdlog::default_logger();
} /* -----  end of method CollectorApp::CollectorApp  (constructor)  ----- */

CollectorApp::~CollectorApp()
{
    if (spdlog::get("Collector_logger"))
    {
        spdlog::drop("Collector_logger");
    }
    if (original_logger_)
    {
        spdlog::set_default_logger(original_logger_);
    }
} /* -----  end of method CollectorApp::CollectorApp  (constructor)  ----- */

void CollectorApp::ConfigureLogging()
{
    // this logging code comes from gemini

    spdlog::init_thread_pool(8192, 1);

    if (!log_file_path_name_.empty())
    {
        fs::path log_dir = log_file_path_name_.parent_path();
        if (!fs::exists(log_dir))
        {
            fs::create_directories(log_dir);
        }

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file_path_name_, true);

        auto async_logger = std::make_shared<spdlog::async_logger>(
            "Collector_logger",
            spdlog::sinks_init_list{file_sink},
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block // Or spdlog::async_overflow_policy::discard_log_msg
        );

        spdlog::set_default_logger(async_logger);
    }
    else
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        // 3. Create an asynchronous logger using the console sink.
        auto async_logger = std::make_shared<spdlog::async_logger>("Collector_logger", // Name for the console logger
                                                                   spdlog::sinks_init_list{console_sink},
                                                                   spdlog::thread_pool(),
                                                                   spdlog::async_overflow_policy::block);

        spdlog::set_default_logger(async_logger);
    }

    // we are running before 'CheckArgs' so we need to do a little editiing
    // ourselves.

    const std::map<std::string, spdlog::level::level_enum> levels{{"none", spdlog::level::off},
                                                                  {"error", spdlog::level::err},
                                                                  {"information", spdlog::level::info},
                                                                  {"debug", spdlog::level::debug}};

    auto which_level = levels.find(logging_level_);
    if (which_level != levels.end())
    {
        spdlog::set_level(which_level->second);
    }
    else
    {
        spdlog::set_level(spdlog::level::info);
    }

} /* -----  end of method CollectorApp::ConfigureLogging  ----- */

bool CollectorApp::Startup()
{
    spdlog::info(catenate("\n\n*** Begin run ", std::chrono::system_clock::now(), " ***\n"));
    bool result{true};
    try
    {
        SetupProgramOptions();
        if (tokens_.empty())
        {
            ParseProgramOptions();
        }
        else
        {
            ParseProgramOptions(tokens_);
        }
        ConfigureLogging();
        result = CheckArgs();
    }
    catch (std::exception &e)
    {
        spdlog::error(catenate("Problem in startup: ", e.what(), '\n'));
        //	we're outta here!

        this->Shutdown();
        result = false;
    }
    catch (...)
    {
        spdlog::error("Unexpectd problem during Starup processing\n");

        //	we're outta here!

        this->Shutdown();
        result = false;
    }
    return result;
} /* -----  end of method CollectorApp::Startup  ----- */

void CollectorApp::SetupProgramOptions()
{
    mNewOptions = std::make_unique<po::options_description>();

    mNewOptions->add_options()("help,h", "produce help message")("begin-date",
                                                                 po::value<std::string>(&this->start_date_),
                                                                 "retrieve files with dates greater than or equal to.")(
        "end-date", po::value<std::string>(&this->stop_date_), "retrieve files with dates less than or equal to.")(
        "index-dir", po::value<fs::path>(&this->local_index_file_directory_),
        "directory index files are downloaded to.")("form-dir", po::value<fs::path>(&this->local_form_file_directory_),
                                                    "directory form files are downloaded to.")(
        "host",
        po::value<std::string>(&this->HTTPS_host_)->default_value("www.sec.gov"),
        "web site we download from. Default is 'www.sec.gov'.")(
        "port", po::value<std::string>(&this->HTTPS_port_)->default_value("443"),
        "Port number to use for web site. Default is '443' for SSL.")(
        "mode", po::value<std::string>(&this->mode_)->default_value("daily"),
        "'daily' or 'quarterly' for index files, 'ticker-only' or 'notes'. "
        "Default is 'daily'.")("form", po::value<std::string>(&this->form_)->default_value("10-Q"),
                               "name of form type[s] we are downloading. Default is '10-Q'.")(
        "ticker", po::value<std::string>(&this->ticker_), "ticker[s] to lookup and filter form downloads.")(
        "log-path", po::value<fs::path>(&this->log_file_path_name_), "path name for log file")(
        "ticker-cache", po::value<fs::path>(&this->ticker_cache_file_name_), "path name for ticker-to-CIK cache file.")(
        "notes-directory",
        po::value<fs::path>(&this->financial_notes_directory_name_),
        "top level path name for financial statements and notes files "
        "downloads.")("ticker-file", po::value<fs::path>(&this->ticker_list_file_name_),
                      "path name for file with list of ticker symbols to convert to CIKs.")(
        "replace-index-files",
        po::value<bool>(&this->replace_index_files_)->implicit_value(true),
        "over write local index files if specified. Default is 'false'.")(
        "replace-form-files",
        po::value<bool>(&this->replace_form_files_)->implicit_value(true),
        "over write local form files if specified. Default is 'false'.")(
        "replace-notes-files",
        po::value<bool>(&this->replace_notes_files_)->implicit_value(true),
        "over write local financial notes files if specified. Default is "
        "'false'.")("index-only",
                    po::value<bool>(&this->index_only_)->implicit_value(true),
                    "do not download form files. Default is 'false'.")(
        "pause,p", po::value<int>(&this->pause_)->default_value(1),
        "how long to wait between downloads. Default: 1 second.")(
        "max", po::value<int>(&this->max_forms_to_download_)->default_value(-1),
        "Maximun number of forms to download -- mainly for testing. Default of "
        "-1 means no limit.")("log-level,l",
                              po::value<std::string>(&this->logging_level_)->default_value("information"),
                              "logging level. Must be 'none|error|information|debug'. Default is "
                              "'information'.")("concurrent,k",
                                                po::value<int>(&this->max_at_a_time_)->default_value(10),
                                                "Maximun number of concurrent downloads. Default of 10.")
        /* ("file,f",    po::value<std::string>(), "name of file containing data
           for ticker. Default is stdin") */
        /* ("mode,m",    po::value<std::string>(), "mode: either 'load' new data
           or 'update' existing data. Default is 'load'") */
        ;

} // -----  end of method CollectorApp::Do_SetupProgramOptions  -----

void CollectorApp::ParseProgramOptions()
{
    auto options = po::parse_command_line(mArgc, mArgv, *mNewOptions);
    po::store(options, mVariableMap);
    if (this->mArgc == 1 || mVariableMap.count("help") == 1)
    {
        std::cout << *mNewOptions << "\n";
        throw std::runtime_error("\nExiting after 'help'.");
    }
    po::notify(mVariableMap);

} /* -----  end of method CollectorApp::ParseProgramOptions  ----- */

void CollectorApp::ParseProgramOptions(const std::vector<std::string> &tokens)
{
    auto options = po::command_line_parser(tokens).options(*mNewOptions).run();
    po::store(options, mVariableMap);
    if (mVariableMap.count("help") == 1)
    {
        std::cout << *mNewOptions << "\n";
        throw std::runtime_error("\nExiting after 'help'.");
    }
    po::notify(mVariableMap);
} /* -----  end of method CollectorApp::ParseProgramOptions  ----- */

bool CollectorApp::CheckArgs()
{
    BOOST_ASSERT_MSG(mode_ == "daily" || mode_ == "quarterly" || mode_ == "ticker-only" || mode_ == "notes",
                     catenate("Mode must be either 'daily','quarterly', 'notes', "
                              "or 'ticker-only' ==> ",
                              mode_)
                         .c_str());

    //	the user may specify multiple stock tickers in a comma delimited list.
    // We need to parse the entries out 	of that list and place into ultimate
    // home. If just a single entry, copy it to our form list destination too.

    if (!ticker_.empty())
    {
        BOOST_ASSERT_MSG(!ticker_cache_file_name_.empty(), "You must provide a cache file when using ticker symbols.");
        ticker_list_ = split_string_to_strings(ticker_, ',');
    }

    if (!ticker_list_file_name_.empty())
    {
        BOOST_ASSERT_MSG(!ticker_cache_file_name_.empty(),
                         "You must provide a cache file when using a file of ticker symbols.");
    }

    if (mode_ == "ticker-only")
    {
        BOOST_ASSERT_MSG(!ticker_cache_file_name_.empty(),
                         "You must specify a cache file when downloading ticker symbols.");
        return true;
    }

    if (!start_date_.empty())
    {
        std::istringstream in{start_date_};
        std::chrono::sys_days tp;
        std::chrono::from_stream(in, "%F", tp);
        if (in.fail())
        {
            // try an alternate representation

            in.clear();
            in.rdbuf()->pubseekpos(0);
            std::chrono::from_stream(in, "%Y-%b-%d", tp);
        }
        BOOST_ASSERT_MSG(!in.fail() && !in.bad(), catenate("Unable to parse begin date: ", start_date_).c_str());
        begin_date_ = tp;
        BOOST_ASSERT_MSG(begin_date_.ok(), catenate("Invalid begin date: ", start_date_).c_str());
    }
    if (!stop_date_.empty())
    {
        std::istringstream in{stop_date_};
        std::chrono::sys_days tp;
        std::chrono::from_stream(in, "%F", tp);
        if (in.fail())
        {
            // try an alternate representation

            in.clear();
            in.rdbuf()->pubseekpos(0);
            std::chrono::from_stream(in, "%Y-%b-%d", tp);
        }
        BOOST_ASSERT_MSG(!in.fail() && !in.bad(), catenate("Unable to parse end date: ", end_date_).c_str());
        end_date_ = tp;
        BOOST_ASSERT_MSG(end_date_.ok(), catenate("Invalid end date: ", stop_date_).c_str());
    }

    BOOST_ASSERT_MSG(!start_date_.empty(),
                     "Must specify 'begin-date' for index and/or form downloads "
                     "and/or notes files downloads.");

    if (stop_date_.empty())
    {
        end_date_ = begin_date_;
    }

    if (mode_ == "notes")
    {
        BOOST_ASSERT_MSG(!financial_notes_directory_name_.empty(),
                         "You must specify a directory when downloading financial notes files.");
        return true;
    }

    BOOST_ASSERT_MSG(!local_index_file_directory_.empty(),
                     "Must specify 'index-dir' when downloading index and/or forms.");
    BOOST_ASSERT_MSG(index_only_ || !local_form_file_directory_.empty(),
                     "Must specify 'form-dir' when not using 'index-only' option.");

    //	the user may specify multiple form types in a comma delimited list. We
    // need to parse the entries out 	of that list and place into ultimate
    // home.  If just a single entry, copy it to our form list destination too.

    if (!form_.empty())
    {
        form_list_ = split_string_to_strings(form_, ',');
    }

    return true;
} // -----  end of method CollectorApp::Do_CheckArgs  -----

void CollectorApp::Run()
{
    if (!ticker_cache_file_name_.empty() && mode_ != "ticker-only")
    {
        ticker_converter_.UseCacheFile(ticker_cache_file_name_);
    }

    if (!ticker_list_file_name_.empty())
    {
        Do_Run_TickerFileLookup();
    }

    if (mode_ == "ticker-only")
    {
        Do_Run_TickerDownload();
    }
    else if (mode_ == "notes")
    {
        Do_Run_FinancialNotesDownload();
    }
    else if (mode_ == "daily")
    {
        Do_Run_DailyIndexFiles();
    }
    else
    {
        Do_Run_QuarterlyIndexFiles();
    }

} // -----  end of method CollectorApp::Do_Run  -----

void CollectorApp::Do_Run_TickerDownload()
{
    ticker_converter_.DownloadTickerToCIKFile(ticker_cache_file_name_);

} // -----  end of method CollectorApp::Do_Run_tickerLookup  -----

void CollectorApp::Do_Run_FinancialNotesDownload()
{
    FinancialStatementsAndNotes fin_statement_downloader{begin_date_, end_date_};
    fin_statement_downloader.download_files(HTTPS_host_, HTTPS_port_, financial_notes_directory_name_ / "zip_files",
                                            financial_notes_directory_name_ / "data_files", replace_notes_files_);
} // -----  end of method CollectorApp::Do_Run_FinancialNotesDownload  -----

void CollectorApp::Do_Run_TickerFileLookup()
{
    ticker_converter_.UseCacheFile(ticker_cache_file_name_);
    ticker_map_ = ticker_converter_.ConvertFileOfTickersToCIKs(ticker_list_file_name_);

} // -----  end of method CollectorApp::Do_Run_TickerFileLookup  -----

void CollectorApp::Do_Run_DailyIndexFiles()
{
    // FTP_Server a_server{"localhost", "anonymous", "aaa@bbb.net"};
    DailyIndexFileRetriever idxFileRet{HTTPS_host_, HTTPS_port_, "/Archives/edgar/daily-index"};

    Do_TickerMap_Setup();

    if (begin_date_ == end_date_)
    {
        auto remote_daily_index_file_name = idxFileRet.FindRemoteIndexFileNameNearestDate(this->begin_date_);
        auto local_daily_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(
            remote_daily_index_file_name, this->local_index_file_directory_, replace_index_files_);

        if (!index_only_)
        {
            FormFileRetriever form_file_getter{HTTPS_host_, HTTPS_port_};
            decltype(auto) form_file_list =
                form_file_getter.FindFilesForForms(form_list_, local_daily_index_file_name, ticker_map_);

            if (max_forms_to_download_ > -1)
            {
                for (auto &[form, files] : form_file_list)
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
            form_file_getter.ConcurrentlyRetrieveSpecifiedFiles(form_file_list, this->local_form_file_directory_,
                                                                max_at_a_time_, replace_form_files_);
        }
    }
    else
    {
        auto remote_daily_index_file_list = idxFileRet.FindRemoteIndexFileNamesForDateRange(begin_date_, end_date_);
        auto local_daily_index_file_list = idxFileRet.ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo(
            remote_daily_index_file_list, local_index_file_directory_, max_at_a_time_, replace_index_files_);

        if (!index_only_)
        {
            FormFileRetriever form_file_getter{HTTPS_host_, HTTPS_port_};
            decltype(auto) form_file_list =
                form_file_getter.FindFilesForForms(form_list_, local_daily_index_file_list, ticker_map_);

            if (max_forms_to_download_ > -1)
            {
                // same comment here as above for single file.

                for (auto &[form, files] : form_file_list)
                {
                    if (files.size() > max_forms_to_download_)
                    {
                        std::default_random_engine dre;
                        std::shuffle(files.begin(), files.end(), dre);
                        files.resize(max_forms_to_download_);
                    }
                }
            }
            form_file_getter.ConcurrentlyRetrieveSpecifiedFiles(form_file_list, local_form_file_directory_,
                                                                max_at_a_time_, replace_form_files_);
        }
    }

} // -----  end of method CollectorApp::Do_Run_DailyIndexFiles  -----

void CollectorApp::Do_Run_QuarterlyIndexFiles()
{
    Do_TickerMap_Setup();

    QuarterlyIndexFileRetriever idxFileRet{HTTPS_host_, HTTPS_port_, "/Archives/edgar/full-index"};

    if (begin_date_ == end_date_)
    {
        auto remote_quarterly_index_file_name = idxFileRet.MakeQuarterlyIndexPathName(begin_date_);
        auto local_quarterly_index_file_name = idxFileRet.HierarchicalCopyRemoteIndexFileTo(
            remote_quarterly_index_file_name, this->local_index_file_directory_, replace_index_files_);

        if (!index_only_)
        {
            FormFileRetriever form_file_getter{HTTPS_host_, HTTPS_port_};
            decltype(auto) form_file_list =
                form_file_getter.FindFilesForForms(form_list_, local_quarterly_index_file_name, ticker_map_);

            if (max_forms_to_download_ > -1)
            {
                for (auto &[form, files] : form_file_list)
                {
                    if (files.size() > max_forms_to_download_)
                    {
                        std::default_random_engine dre;
                        std::shuffle(files.begin(), files.end(), dre);
                        files.resize(max_forms_to_download_);
                    }
                }
            }
            form_file_getter.ConcurrentlyRetrieveSpecifiedFiles(form_file_list, this->local_form_file_directory_,
                                                                max_at_a_time_, replace_form_files_);
        }
    }
    else
    {
        auto remote_index_file_list = idxFileRet.MakeIndexFileNamesForDateRange(begin_date_, end_date_);
        auto local_index_file_list = idxFileRet.ConcurrentlyHierarchicalCopyIndexFilesForDateRangeTo(
            remote_index_file_list, local_index_file_directory_, max_at_a_time_, replace_index_files_);

        if (!index_only_)
        {
            FormFileRetriever form_file_getter{HTTPS_host_, HTTPS_port_};
            decltype(auto) form_file_list =
                form_file_getter.FindFilesForForms(form_list_, local_index_file_list, ticker_map_);

            if (max_forms_to_download_ > -1)
            {
                for (auto &[form, files] : form_file_list)
                {
                    if (files.size() > max_forms_to_download_)
                    {
                        std::default_random_engine dre;
                        std::shuffle(files.begin(), files.end(), dre);
                        files.resize(max_forms_to_download_);
                    }
                }
            }
            form_file_getter.ConcurrentlyRetrieveSpecifiedFiles(form_file_list, local_form_file_directory_,
                                                                max_at_a_time_, replace_form_files_);
        }
    }

} // -----  end of method CollectorApp::Do_Run_QuarterlyIndexFiles  -----

void CollectorApp::Do_TickerMap_Setup()
{
    for (const auto &ticker : ticker_list_)
    {
        ticker_map_[ticker] = ticker_converter_.ConvertTickerToCIK(ticker);
    }

} // -----  end of method CollectorApp::Do_TickerMap_Setup  -----

void CollectorApp::Shutdown()
{
    spdlog::info(catenate("\n\n*** End run ", LocalDateTimeAsString(std::chrono::system_clock::now()), " ***\n"));

    std::this_thread::sleep_for(std::chrono::seconds(2)); // Give time for async processing

    // spdlog::shutdown(); // Ensure all messages are flushed

} // -----  end of method CollectorApp::Do_Quit  -----
