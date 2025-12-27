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

#include <ranges>

using namespace std::string_literals;
// using namespace std::chrono_literals;
using namespace std::string_view_literals;

namespace rng = std::ranges;

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
CollectorApp::CollectorApp(int argc, char *argv[]) : argc_{argc}, argv_{argv}
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
} /* -----  end of method CollectorApp::~CollectorApp  (destructor)  ----- */

void CollectorApp::ConfigureLogging()
{
    // this logging code comes from gemini

    if (!log_file_path_name_.empty())
    {
        fs::path log_dir = log_file_path_name_.parent_path();
        if (!fs::exists(log_dir))
        {
            fs::create_directories(log_dir);
        }

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file_path_name_, true);

        auto app_logger = std::make_shared<spdlog::logger>("Collector_logger", file_sink);

        spdlog::set_default_logger(app_logger);
    }
    else
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        auto app_logger =
            std::make_shared<spdlog::logger>("Collector_logger", console_sink); // Name for the console logger

        spdlog::set_default_logger(app_logger);
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
    spdlog::info(catenate("\n\n*** Begin run ", LocalDateTimeAsString(std::chrono::system_clock::now()), " ***\n"));
    bool result{true};
    try
    {
        SetupProgramOptions();
        ParseProgramOptions(tokens_);
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

/*
 * ===  FUNCTION  ======================================================================
 * Name:  ClassName::member_function_name
 * Description:  brief description
 * =====================================================================================
 */
void CollectorApp::SetupProgramOptions()
{
    // Add a preparse callback to check for no arguments
    app.preparse_callback([](size_t argCount) {
        if (argCount == 0)
        {
            throw(CLI::CallForHelp());
        }
    });
    // Set a failure message for when an option is needed but not provided
    app.failure_message(CLI::FailureMessage::help);

    // Add options and flags, binding them directly to your member variables.
    // CLI11 automatically deduces the type from the bound variable.
    app.add_option("--begin-date", this->start_date_, "Retrieve files with dates greater than or equal to.");
    app.add_option("--end-date", this->stop_date_, "Retrieve files with dates less than or equal to.");
    app.add_option("--index-dir", this->local_index_file_directory_, "Directory index files are downloaded to.");
    app.add_option("--form-dir", this->local_form_file_directory_, "Directory form files are downloaded to.");
    app.add_option("--host", this->HTTPS_host_, "Web site to download from.")->default_val("www.sec.gov");
    app.add_option("--port", this->HTTPS_port_, "Port number for the web site.")->default_val("443");
    app.add_option("--mode", this->mode_, "'daily' or 'quarterly' index files, 'ticker-only' or 'notes'.")
        ->default_val("daily");
    app.add_option("--form", this->form_, "Name of form type(s) to download.")->default_val("10-Q");
    app.add_option("--ticker", this->ticker_, "Ticker(s) to lookup and filter form downloads.");
    app.add_option("--log-path", this->log_file_path_name_, "Path name for the log file.");
    app.add_option("--ticker-cache", this->ticker_cache_file_name_, "Path for the ticker-to-CIK cache file.");
    app.add_option("--notes-directory", this->financial_notes_directory_name_,
                   "Top-level path for financial statements and notes files.");
    app.add_option("--new-files-logs-directory", this->new_forms_log_directory_name_,
                   "Directory to write new forms file name logs to.");
    app.add_option("--ticker-file", this->ticker_list_file_name_, "Path for file with a list of ticker symbols.");

    // Boolean flags: In Boost, these were bools with an implicit_value.
    // In CLI11, add_flag is the idiomatic way to handle this.
    // The flag being present on the command line sets the bound variable to true.
    app.add_flag("--replace-index-files", this->replace_index_files_, "Overwrite local index files if specified.");
    app.add_flag("--replace-form-files", this->replace_form_files_, "Overwrite local form files if specified.");
    app.add_flag("--replace-notes-files", this->replace_notes_files_,
                 "Overwrite local financial notes files if specified.");
    app.add_flag("--log-new-form-files", this->log_new_form_files_, "Log path names of newly downloaded form files.");
    app.add_flag("--index-only", this->index_only_, "Only download index files; do not download form files.");

    // Options with short names
    app.add_option("-p,--pause", this->pause_, "Time to wait between downloads (seconds).")->default_val(1);
    app.add_option("--max", this->max_forms_to_download_, "Maximum number of forms to download (-1 for no limit).")
        ->default_val(-1);
    app.add_option("-l,--log-level", this->logging_level_, "Logging level: 'none|error|information|debug'.")
        ->default_val("information");
    app.add_option("-k,--concurrent", this->max_at_a_time_, "Maximum number of concurrent downloads.")->default_val(10);

    // CLI11 automatically adds a -h,--help flag, so you don't need to add it manually.
}

void CollectorApp::ParseProgramOptions(const std::vector<std::string> &tokens)
{
    try
    {
        if (tokens.empty())
        {
            // If the token vector is empty, parse the original argc/argv.
            // This is the standard execution path.
            auto args = std::views::counted(argv_, argc_) |
                        std::views::transform([](char *arg) { return std::string_view(arg); });
            auto cmd_line_vw = rng::views::join_with(rng::views::drop(args, 1), " "sv);
            std::string cmd_line = rng::to<std::string>(cmd_line_vw);
            spdlog::info("cmd line: {}", cmd_line);
            app.parse(argc_, argv_);
        }
        else
        {
            // Note: CLI11's vector parse does NOT expect the program name.
            // NOTE: I don't understand how to setup the call for using
            // the tokens vector directly (it doesn't seem to work with the obvious call
            // as I get parse errors that I shouldn't)
            // so I'll join them into a comand line and use that.
            auto cmd_line_vw = rng::views::join_with(rng::views::drop(tokens, 1), " "sv);

            std::string cmd_line = rng::to<std::string>(cmd_line_vw);
            spdlog::info("tokens: {}", cmd_line);
            app.parse(cmd_line);
        }
    }
    catch (const CLI::CallForHelp &e)
    {
        // CLI11 automatically prints the help message when it sees -h or --help.
        // It then throws CLI::CallForHelp.
        // All we need to do is exit gracefully. Re-throwing is a clean way
        // to signal the caller that execution should stop.
        app.exit(e);
    }
    catch (const CLI::ParseError &e)
    {
        // For any other parsing error (missing required option, bad value, etc.),
        // CLI11 throws a ParseError. We can format a clean message and throw.
        // The app.exit(e) call is often used in main() to get an exit code,
        // but re-throwing is better for a class member function.
        throw std::runtime_error(std::format("Command line parse error: {}", e.what()));
    }
} /* -----  end of method CollectorApp::ParseProgramOptions  ----- */

bool CollectorApp::CheckArgs()
{
    // don't do any checking if there is nothing to check
    // or help was asked for.

    if (app.count_all() == 1 || app.get_option("--help")->count() == 1)
    {
        return false;
    }
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

    if (log_new_form_files_)
    {
        BOOST_ASSERT_MSG(!new_forms_log_directory_name_.empty(),
                         "You must specify 'new-files-logs-directory' when logging download of new forms files.");
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
    if (log_new_form_files_)
    {
        if (!fs::exists(new_forms_log_directory_name_))
        {
            fs::create_directories(new_forms_log_directory_name_);
        }
        const std::string log_name = std::format("new_forms_{:%Y-%m-%d_%H:%M:%S}", std::chrono::system_clock::now());
        new_forms_log_file_name_ = new_forms_log_directory_name_ / log_name;

        auto downloads_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(new_forms_log_file_name_, true);
        auto downloads_logger = std::make_shared<spdlog::logger>(DOWNLOADS_LOGGER_NAME, downloads_sink);
        downloads_logger->set_pattern("%v");
        downloads_logger->set_level(spdlog::level::info);
        spdlog::register_logger(downloads_logger);
    }

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

    spdlog::shutdown(); // Ensure all messages are flushed

} // -----  end of method CollectorApp::Do_Quit  -----
