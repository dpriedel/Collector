// =====================================================================================
//
//       Filename:  TickerConverter.cpp
//
//    Description:  Implementation file for class which does a file or web
//    lookup to 				convert a ticker ticker to an SEC CIK.
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
// #include <regex>

// I know I'm using boost exceptions in this program

#include <boost/json.hpp>
#include <boost/regex.hpp>
#include <spdlog/spdlog.h>

#include "HTTPS_Downloader.h"
#include "TickerConverter.h"

//--------------------------------------------------------------------------------------
//       Class:  TickerConverter
//      Method:  TickerConverter
// Description:  constructor
//--------------------------------------------------------------------------------------

// constexpr const char* TickerConverter::NotFound;

std::string TickerConverter::ConvertTickerToCIK(const std::string &ticker,
                                                int pause) {
  //	we have a cache of our already looked up tickers so let's check that
  //first

  spdlog::debug(catenate("T: Doing CIK lookup for ticker: ", ticker));

  decltype(auto) pos = ticker_to_CIK_.find(ticker);
  if (pos != ticker_to_CIK_.end()) {
    spdlog::debug(catenate("T: Found CIK: ", pos->second, " in cache."));
    return pos->second;
  }

  auto cik = NotFound;
  spdlog::debug(fmt::format("T: Unable to find CIK for ticker: {}.", ticker));

  return cik;
} // -----  end of method TickerConverter::ConvertTickerToCIK  -----

TickerConverter::TickerCIKMap
TickerConverter::ConvertFileOfTickersToCIKs(const fs::path &ticker_file_name) {
  BOOST_ASSERT_MSG(!ticker_to_CIK_.empty(),
                   "Must load ticker cache file data before doing lookups.");

  std::ifstream tickers_file{ticker_file_name};
  BOOST_ASSERT_MSG(
      tickers_file.is_open(),
      fmt::format("Unable to open tickers list file: {}", ticker_file_name)
          .c_str());

  TickerConverter::TickerCIKMap result;

  while (tickers_file) {
    std::string next_ticker;
    tickers_file >> next_ticker;
    if (!next_ticker.empty()) {
      auto pos = ticker_to_CIK_.find(next_ticker);
      if (pos != ticker_to_CIK_.end()) {
        result.insert_or_assign(pos->first, pos->second);
      }
    }
  }

  tickers_file.close();

  spdlog::debug(
      fmt::format("T: Did Ticker lookup for: {} tickers from file: {}.",
                  ticker_to_CIK_.size(), ticker_file_name));

  return result;
} // -----  end of method TickerConverter::ConvertFileOfTickersToCIKs  -----

int TickerConverter::DownloadTickerToCIKFile(const fs::path &ticker_file_name,
                                             const std::string &server_name,
                                             const std::string &port) {
  spdlog::debug(
      fmt::format("T: Downloading tickers file to: {} .", ticker_file_name));

  std::string uri = "/files/company_tickers.json";

  HTTPS_Downloader edgar_server(server_name, port);

  std::string ticker_to_CIK_data = edgar_server.RetrieveDataFromServer(uri);
  BOOST_ASSERT_MSG(!ticker_to_CIK_data.empty(),
                   "Unable to download ticker-to-CIK data.");

  //  ticker data is in json format.  we will store as tab-delimited file.

  const auto json_listing = boost::json::parse(ticker_to_CIK_data);

  std::string extracted_data;
  int ticker_count = 0;

  for (const auto &[k, v] : json_listing.as_object()) {
    extracted_data.append(fmt::format(
        "{}\t{:0>10d}\n", v.as_object().at("ticker").as_string().c_str(),
        v.as_object().at("cik_str").as_int64()));
    ++ticker_count;
  }
  std::ofstream tickers_file{ticker_file_name,
                             std::ios::out | std::ios::binary};
  BOOST_ASSERT_MSG(
      tickers_file.is_open(),
      fmt::format("Unable to open ticker_CIK file: {}", ticker_file_name)
          .c_str());
  tickers_file.write(extracted_data.data(), extracted_data.size());
  tickers_file.close();

  spdlog::debug(fmt::format("T: Did Ticker download for: {} ticker symbols.",
                            ticker_count));

  return ticker_count;
} // -----  end of method TickerConverter::ConvertTickerFileToCIKs  -----

std::string SEC_CIK_Lookup(COL::sview ticker, int pause) {
  // let's use our HTTPS_Downloader class since it knows how to do what we want
  // to do.

  std::chrono::seconds pause_time{pause};

  std::string uri = catenate("/cgi-bin/browse-edgar?CIK=", ticker,
                             "&Find=Search&owner=exclude&action=getcompany");

  HTTPS_Downloader edgar_server("www.sec.gov", "443");

  std::string the_html = edgar_server.RetrieveDataFromServer(uri);

  boost::regex ex{R"***(name="CIK"\s*value="(\d+)")***"};

  boost::smatch cik;
  bool found = boost::regex_search(the_html, cik, ex);

  std::this_thread::sleep_for(pause_time);

  if (found) {
    return cik[1].str();
  }
  return {};
} // -----  end of method ConvertTickerToCIK  -----

int TickerConverter::UseCacheFile(const fs::path &cache_file_name) {
  BOOST_ASSERT_MSG(
      fs::exists(cache_file_name),
      fmt::format("Unable to find ticker_CIK file: {}", cache_file_name)
          .c_str());
  ticker_to_CIK_.clear();
  cache_file_name_ = cache_file_name;

  std::ifstream ticker_cache_file{cache_file_name_};

  while (ticker_cache_file) {
    std::string ticker;
    std::string cik;

    ticker_cache_file >> ticker >> cik;
    if (ticker.empty() || cik.empty()) {
      continue;
    }

    ticker_to_CIK_[ticker] = cik;
  }

  ticker_count_start_ = ticker_to_CIK_.size();
  spdlog::debug(catenate("T: Loaded ", ticker_count_start_,
                         " tickers from file: ", cache_file_name.string()));

  return ticker_count_start_;

} // -----  end of method TickerConverter::UseTickerFile  -----

// void TickerConverter::SaveCIKDataToFile ()
//{
//	ticker_count_end_ = ticker_to_CIK_.size();
//
//	if (ticker_count_end_ == 0)
//     {
//		return;
//     }
//
//	std::ofstream ticker_cache_file{cache_file_name_.string()};
//
//	for (const auto& x : ticker_to_CIK_)
//     {
//		ticker_cache_file << x.first << '\t' << x.second << '\n';
//     }
//
//	spdlog::debug(catenate("T: Saved ", ticker_to_CIK_.size(), " tickers to
//file: ", cache_file_name_.string())); }		// -----  end of method
// TickerConverter::SaveCIKToFile  -----
