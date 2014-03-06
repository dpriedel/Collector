// =====================================================================================
// 
//       Filename:  TickerConverter.cpp
// 
//    Description:  Implementation file for class which does a file or web lookup to
//    				convert a ticker ticker to an EDGAR CIK.
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


#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>

//#include <regex>
#include <boost/regex.hpp>

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>

namespace pn = Poco::Net;

#include "TickerConverter.h"
#include "TException.h"

//--------------------------------------------------------------------------------------
//       Class:  TickerConverter
//      Method:  TickerConverter
// Description:  constructor
//--------------------------------------------------------------------------------------

constexpr char TickerConverter::NotFound[];

TickerConverter::TickerConverter ()
{
}  // -----  end of method TickerConverter::TickerConverter  (constructor)  -----


std::string TickerConverter::ConvertTickerToCIK (const std::string& ticker, int pause)
{
	//	we have a cache of our already looked up tickers so let's check that first
	
	std::clog << "T: Doing CIK lookup for ticker: " << ticker << '\n';

	auto pos = ticker_to_CIK_.find(ticker);
	if (pos != ticker_to_CIK_.end())
	{
		std::clog << "T: Found CIK: " << pos->second << " in cache.\n";
		return pos->second;
	}

	auto cik = EDGAR_CIK_Lookup(ticker, pause);
	if (cik.empty())
		cik = NotFound;
	ticker_to_CIK_[ticker] = cik;

	std::clog << "T: Found CIK: " << cik << " via EDGAR query.\n";

	return cik;
}		// -----  end of method TickerConverter::ConvertTickerToCIK  -----

int TickerConverter::ConvertTickerFileToCIKs (const fs::path& ticker_file_name, int pause)
{
	std::clog << "T: Doing CIK lookup for tickers in file: " << ticker_file_name << '\n';

	std::ifstream tickers_file{ticker_file_name.string()};
	dfail_if_(! tickers_file.is_open(), "Unable to open tickers file: ", ticker_file_name.string());

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

	std::clog << "T: Did Ticker lookup for: " << result << " ticker symbols." << '\n';

	return result;
}		// -----  end of method TickerConverter::ConvertTickerFileToCIKs  -----

std::string TickerConverter::EDGAR_CIK_Lookup (const std::string& ticker, int pause)
{
	std::chrono::seconds pause_time{pause};

	pn::HTTPClientSession session{"www.sec.gov"};
	
	pn::HTTPRequest req{pn::HTTPMessage::HTTP_1_1};
	req.setMethod(pn::HTTPRequest::HTTP_GET);

	std::string uri{"/cgi-bin/browse-edgar?CIK="};
	uri += ticker;
	uri += "&Find=Search&owner=exclude&action=getcompany";
	req.setURI(uri);

	std::string the_html;
	the_html.reserve(5000);		//	we know we're going to get a couple K at least

	try
	{
		session.sendRequest(req);

		pn::HTTPResponse res;

		auto& response = session.receiveResponse(res);

		the_html.assign(std::istreambuf_iterator<char>(response), std::istreambuf_iterator<char>());
	}
	catch (std::exception& e)
	{
		std::cerr << "Unable to do ticker-to-CIK conversion for ticker: " << ticker << '\n' << e.what() << '\n';
		return "";
	}
	boost::regex ex{R"***(name="CIK"\s*value="(\d+)")***"};

	boost::smatch cik;
	bool found = boost::regex_search(the_html, cik, ex);
	
	std::this_thread::sleep_for(pause_time);

	if (found)
		return cik[1].str();
	else
		return "";
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
		if (! ticker.size() || ! cik.size())
			continue;

		ticker_to_CIK_[ticker] = cik;
	}

	ticker_count_start_ = ticker_to_CIK_.size();
	std::clog << "T: Loaded " << ticker_count_start_ << " tickers from file: " << cache_file_name << '\n';

}		// -----  end of method TickerConverter::UseTickerFile  -----

void TickerConverter::SaveCIKDataToFile (void)
{
	ticker_count_end_ = ticker_to_CIK_.size();

	if (ticker_count_end_ == 0)
		return;

	std::ofstream ticker_cache_file{cache_file_name_.string()};
	
	for (const auto& x : ticker_to_CIK_)
		ticker_cache_file << x.first << '\t' << x.second << '\n';

	std::clog << "T: Saved " << ticker_to_CIK_.size() << " tickers to file: " << cache_file_name_ << '\n';
}		// -----  end of method TickerConverter::SaveCIKToFile  -----

