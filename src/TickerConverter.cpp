// =====================================================================================
//
//       Filename:  TickerConverter.cpp
//
//    Description:  Implementation file for class which does a file or web lookup to
//    				convert a ticker ticker to an SEC CIK.
//
//        Version:  1.0
//        Created:  02/06/2014 01:57:16 PM
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


#include <chrono>
#include <fstream>
#include <thread>
#include <regex>

// #include <boost/regex.hpp>
#include "HTTPS_Downloader.h"

#include "TickerConverter.h"

//--------------------------------------------------------------------------------------
//       Class:  TickerConverter
//      Method:  TickerConverter
// Description:  constructor
//--------------------------------------------------------------------------------------

constexpr char TickerConverter::NotFound[];

TickerConverter::TickerConverter (Poco::Logger& the_logger)
    : the_logger_{the_logger}
{
}  // -----  end of method TickerConverter::TickerConverter  (constructor)  -----


std::string TickerConverter::ConvertTickerToCIK (const std::string& ticker, int pause)
{
	//	we have a cache of our already looked up tickers so let's check that first

	poco_debug(the_logger_, "T: Doing CIK lookup for ticker: " + ticker);

	decltype(auto) pos = ticker_to_CIK_.find(ticker);
	if (pos != ticker_to_CIK_.end())
	{
		poco_debug(the_logger_, "T: Found CIK: " + pos->second + " in cache.");
		return pos->second;
	}

	decltype(auto) cik = SEC_CIK_Lookup(ticker, pause);
	if (cik.empty())
		cik = NotFound;
	ticker_to_CIK_[ticker] = cik;

	poco_debug(the_logger_, "T: Found CIK: " + cik + " via SEC query.");

	return cik;
}		// -----  end of method TickerConverter::ConvertTickerToCIK  -----

int TickerConverter::ConvertTickerFileToCIKs (const fs::path& ticker_file_name, int pause)
{
	poco_debug(the_logger_, "T: Doing CIK lookup for tickers in file: " + ticker_file_name.string());

	std::ifstream tickers_file{ticker_file_name.string()};
	poco_assert_msg(tickers_file.is_open(), ("Unable to open tickers file: " + ticker_file_name.string()).c_str());

	int result{0};

	while (tickers_file)
	{
		std::string next_ticker;
		tickers_file >> next_ticker;
		if (! next_ticker.empty())
		{
			ConvertTickerToCIK(next_ticker, pause);
			++result;
		}
	}

	tickers_file.close();

	poco_debug(the_logger_, "T: Did Ticker lookup for: " + std::to_string(result) + " ticker symbols.");

	return result;
}		// -----  end of method TickerConverter::ConvertTickerFileToCIKs  -----

std::string TickerConverter::SEC_CIK_Lookup (const std::string& ticker, int pause)
{
    // let's use our HTTPS_Downloader class since it knows how to do what we want to do.

	std::chrono::seconds pause_time{pause};

	std::string uri{"/cgi-bin/browse-edgar?CIK="};
	uri += ticker;
	uri += "&Find=Search&owner=exclude&action=getcompany";

    HTTPS_Downloader edgar_server("www.sec.gov", "443");

	std::string the_html = edgar_server.RetrieveDataFromServer(uri);

	std::regex ex{R"***(name="CIK"\s*value="(\d+)")***"};

	std::smatch cik;
	bool found = std::regex_search(the_html, cik, ex);

	std::this_thread::sleep_for(pause_time);

	if (found)
		return cik[1].str();
	else
		return std::string();
}		// -----  end of method TickerConverter::ConvertTickerToCIK  -----

void TickerConverter::UseCacheFile (const fs::path& cache_file_name)
{
	cache_file_name_ = cache_file_name;
	if (! fs::exists(cache_file_name_))
		return ;

	std::ifstream ticker_cache_file{cache_file_name_.string()};

	while (ticker_cache_file)
	{
		std::string ticker;
		std::string cik;

		ticker_cache_file >> ticker >> cik;
		if (ticker.empty() || cik.empty())
			continue;

		ticker_to_CIK_[ticker] = cik;
	}

	ticker_count_start_ = ticker_to_CIK_.size();
	poco_debug(the_logger_, "T: Loaded " + std::to_string(ticker_count_start_) + " tickers from file: " + cache_file_name.string());

}		// -----  end of method TickerConverter::UseTickerFile  -----

void TickerConverter::SaveCIKDataToFile (void)
{
	ticker_count_end_ = ticker_to_CIK_.size();

	if (ticker_count_end_ == 0)
		return;

	std::ofstream ticker_cache_file{cache_file_name_.string()};

	for (const auto& x : ticker_to_CIK_)
		ticker_cache_file << x.first << '\t' << x.second << '\n';

	poco_debug(the_logger_, "T: Saved " + std::to_string(ticker_to_CIK_.size()) + " tickers to file: " + cache_file_name_.string());
}		// -----  end of method TickerConverter::SaveCIKToFile  -----
