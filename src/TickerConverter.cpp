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
//#include <regex>

#include <boost/regex.hpp>

#include "spdlog/spdlog.h"

#include "HTTPS_Downloader.h"
#include "TickerConverter.h"

//--------------------------------------------------------------------------------------
//       Class:  TickerConverter
//      Method:  TickerConverter
// Description:  constructor
//--------------------------------------------------------------------------------------

constexpr char TickerConverter::NotFound[];

std::string TickerConverter::ConvertTickerToCIK (const std::string& ticker, int pause)
{
	//	we have a cache of our already looked up tickers so let's check that first

	spdlog::debug(catenate("T: Doing CIK lookup for ticker: ", ticker));

	decltype(auto) pos = ticker_to_CIK_.find(ticker);
	if (pos != ticker_to_CIK_.end())
	{
		spdlog::debug(catenate("T: Found CIK: ", pos->second, " in cache."));
		return pos->second;
	}

	decltype(auto) cik = SEC_CIK_Lookup(ticker, pause);
	if (cik.empty())
    {
		cik = NotFound;
    }
	ticker_to_CIK_[ticker] = cik;

	spdlog::debug(catenate("T: Found CIK: ", cik, " via SEC query."));

	return cik;
}		// -----  end of method TickerConverter::ConvertTickerToCIK  -----

int TickerConverter::ConvertTickerFileToCIKs (const fs::path& ticker_file_name, int pause)
{
	spdlog::debug(catenate("T: Doing CIK lookup for tickers in file: ", ticker_file_name.string()));

	std::ifstream tickers_file{ticker_file_name};
	BOOST_ASSERT_MSG(tickers_file.is_open(), catenate("Unable to open tickers file: ", ticker_file_name).c_str());

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

	spdlog::debug(catenate("T: Did Ticker lookup for: ", result, " ticker symbols."));

	return result;
}		// -----  end of method TickerConverter::ConvertTickerFileToCIKs  -----

std::string SEC_CIK_Lookup (COL::sview ticker, int pause)
{
    // let's use our HTTPS_Downloader class since it knows how to do what we want to do.

	std::chrono::seconds pause_time{pause};

	std::string uri = catenate("/cgi-bin/browse-edgar?CIK=", ticker, "&Find=Search&owner=exclude&action=getcompany");

    HTTPS_Downloader edgar_server("www.sec.gov", "443");

	std::string the_html = edgar_server.RetrieveDataFromServer(uri);

	boost::regex ex{R"***(name="CIK"\s*value="(\d+)")***"};

	boost::smatch cik;
	bool found = boost::regex_search(the_html, cik, ex);

	std::this_thread::sleep_for(pause_time);

	if (found)
    {
		return cik[1].str();
    }
    return {};
}		// -----  end of method ConvertTickerToCIK  -----

void TickerConverter::UseCacheFile (const fs::path& cache_file_name)
{
	cache_file_name_ = cache_file_name;
	if (! fs::exists(cache_file_name_))
    {
		return ;
    }

	std::ifstream ticker_cache_file{cache_file_name_};

	while (ticker_cache_file)
	{
		std::string ticker;
		std::string cik;

		ticker_cache_file >> ticker >> cik;
		if (ticker.empty() || cik.empty())
        {
			continue;
        }

		ticker_to_CIK_[ticker] = cik;
	}

	ticker_count_start_ = ticker_to_CIK_.size();
	spdlog::debug(catenate("T: Loaded ", ticker_count_start_, " tickers from file: ", cache_file_name.string()));

}		// -----  end of method TickerConverter::UseTickerFile  -----

void TickerConverter::SaveCIKDataToFile ()
{
	ticker_count_end_ = ticker_to_CIK_.size();

	if (ticker_count_end_ == 0)
    {
		return;
    }

	std::ofstream ticker_cache_file{cache_file_name_.string()};

	for (const auto& x : ticker_to_CIK_)
    {
		ticker_cache_file << x.first << '\t' << x.second << '\n';
    }

	spdlog::debug(catenate("T: Saved ", ticker_to_CIK_.size(), " tickers to file: ", cache_file_name_.string()));
}		// -----  end of method TickerConverter::SaveCIKToFile  -----
