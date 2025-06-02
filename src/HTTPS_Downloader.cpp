// =====================================================================================
//
//       Filename:  HTTPS_Connection.cpp
//
//    Description:  Implements class which downloads files from an HTTPS server.
//
//        Version:  1.0
//        Created:  06/21/2017 10:08:13 AM
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

// This code for basice file retrieval is based on sample code from boost Beast
// SSL HTTP sync client.

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <exception>
#include <fstream>
#include <future>
#include <iterator>
#include <sstream>
#include <system_error>
#include <thread>

#include <boost/algorithm/string/trim.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/json.hpp>

#include <spdlog/spdlog.h>
#include <zip.h>

#include "Collector_Utils.h"
#include "HTTPS_Downloader.h"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
namespace ssl = net::ssl;       // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using namespace std::literals::chrono_literals;

bool HTTPS_Downloader::had_signal_ = false;

// code from "The C++ Programming Language" 4th Edition. p. 1243.

template <typename T>
int wait_for_any(std::vector<std::future<T>> &vf,
                 std::chrono::steady_clock::duration d)
// return index of ready future
// if no future is ready, wait for d before trying again
{
  while (true) {
    for (int i = 0; i != vf.size(); ++i) {
      if (!vf[i].valid())
        continue;
      switch (vf[i].wait_for(0s)) {
      case std::future_status::ready:
        return i;

      case std::future_status::timeout:
        break;

      case std::future_status::deferred:
        throw std::runtime_error("wait_for_all(): deferred future");
      }
    }
    std::this_thread::sleep_for(d);
  }
}

//--------------------------------------------------------------------------------------
//       Class:  HTTPS_Downloader
//      Method:  HTTPS_Downloader
// Description:  constructor
//--------------------------------------------------------------------------------------

HTTPS_Downloader::HTTPS_Downloader(const std::string &server_name,
                                   const std::string &port)
    : server_name_{server_name}, port_{port}, ctx{ssl::context::tlsv12_client} {

#ifndef NOCERTTEST
  load_root_certificates(ctx);
  ctx.set_verify_mode(ssl::verify_peer);
#endif

} // -----  end of method HTTPS_Downloader::HTTPS_Downloader  (constructor)
  // -----

std::string HTTPS_Downloader::RetrieveDataFromServer(const fs::path &request) {
  // if any problems occur here, we'll just let beast throw an exception.

  tcp::resolver resolver(ioc);
  beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

  if (!SSL_set_tlsext_host_name(stream.native_handle(), server_name_.c_str())) {
    beast::error_code ec{static_cast<int>(::ERR_get_error()),
                         net::error::get_ssl_category()};
    throw beast::system_error{ec};
  }

  auto const results = resolver.resolve(server_name_, port_);

  beast::get_lowest_layer(stream).connect(results);

  stream.handshake(ssl::stream_base::client);

  http::request<http::string_body> req{http::verb::get, request.c_str(),
                                       version_};
  req.set(http::field::host, server_name_);
  req.set(http::field::user_agent, "dpriedel@cox.net");

  http::write(stream, req);

  beast::flat_buffer buffer;

  http::response<http::string_body> res;

  http::read(stream, buffer, res);
  std::string result = res.body();

  // shutdown without causing a 'stream_truncated' error.

  beast::get_lowest_layer(stream).cancel();
  beast::get_lowest_layer(stream).close();

  return result;
}

std::vector<std::string>
HTTPS_Downloader::ListDirectoryContents(const fs::path &directory_name) {
  //	we read and store our results so we can end the active connection
  // quickly.
  // we will ask for the directory listing in JSON format which means we will
  // have to parse it out.

  fs::path index_file_name{directory_name};
  index_file_name /= "index.json";

  std::string index_listing =
      this->RetrieveDataFromServer(index_file_name.string());

  auto json_listing = boost::json::parse(index_listing);

  std::vector<std::string> results;

  for (auto &e :
       json_listing.as_object()["directory"].as_object()["item"].as_array()) {
    results.emplace_back(e.as_object()["name"].get_string().data());
  }

  //	one last thing...let's make sure there's no junk at end of each entry.

  for (auto &x : results) {
    boost::algorithm::trim_right(x);
  }

  return results;
} // -----  end of method DailyIndexFileRetriever::ListRemoteDirectoryContents
  // -----

void HTTPS_Downloader::DownloadFile(const fs::path &remote_file_name,
                                    const fs::path &local_file_name) {
  // basically the same as RetrieveDataFromServer but write the output to a file
  // instead of a string.
  // but we also need to decompress any zipped files.  Might as well do it here.

  const auto remote_ext = remote_file_name.extension();
  const auto local_ext = local_file_name.extension();

  // we will unzip zipped files but only if the local file name indicates the
  // local file is not zipped.

  const bool need_to_unzip =
      (remote_ext == ".gz" or remote_ext == ".zip") && remote_ext != local_ext;

  tcp::resolver resolver(ioc);
  beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

  if (!SSL_set_tlsext_host_name(stream.native_handle(), server_name_.c_str())) {
    beast::error_code ec{static_cast<int>(::ERR_get_error()),
                         net::error::get_ssl_category()};
    throw beast::system_error{ec};
  }

  auto const results = resolver.resolve(server_name_, port_);

  beast::get_lowest_layer(stream).connect(results);

  stream.handshake(ssl::stream_base::client);

  http::request<http::string_body> req{http::verb::get,
                                       remote_file_name.c_str(), version_};
  req.set(http::field::host, server_name_);
  req.set(http::field::user_agent, "dpriedel@cox.net");

  http::write(stream, req);

  beast::flat_buffer buffer;

  http::response_parser<http::vector_body<char>> res_parser;
  // Allow for an unlimited body size
  res_parser.body_limit((std::numeric_limits<std::uint64_t>::max)());

  beast::error_code ec;
  http::read(stream, buffer, res_parser, ec);
  auto response_content = res_parser.get();

  if (http::int_to_status(response_content.base().result_int()) ==
      http::status::request_timeout) {
    throw Collector::TimeOutException(catenate(
        remote_file_name, ": Result: ", ec.message(), "  ",
        response_content.base().reason().data(), ": Unable to download file."));
  }
  if (ec != beast::errc::success ||
      http::int_to_status(response_content.base().result_int()) !=
          http::status::ok) {
    throw std::system_error(
        ec, catenate(remote_file_name, ": Result: ", ec.message(), "  ",
                     response_content.base().reason().data(),
                     ": Unable to download file."));
  }
  std::vector<char> remote_data = response_content.body();

  // shutdown without causing a 'stream_truncated' error.

  beast::get_lowest_layer(stream).cancel();
  beast::get_lowest_layer(stream).close();

  if (!need_to_unzip) {
    DownloadTextFile(local_file_name, remote_data, remote_file_name);
  } else {
    if (remote_ext == ".gz") {
      DownloadGZipFile(local_file_name, remote_data, remote_file_name);
    } else {
      DownloadZipFile(local_file_name, remote_data, remote_file_name);
    }
  }
} // -----  end of method HTTPS_Downloader::DownloadFile  -----

std::pair<int, int>
HTTPS_Downloader::DownloadFilesConcurrently(const remote_local_list &file_list,
                                            int max_at_a_time)

{
  // since this code can potentially run for hours on end (depending on internet
  // connection throughput) it's a good idea to provide a way to break into this
  // processing and shut it down cleanly. so, a little bit of C...(taken from
  // "Advanced Unix Programming" by Warren W. Gay, p. 317)

  struct sigaction sa_old;
  struct sigaction sa_new;

  sa_new.sa_handler = HTTPS_Downloader::HandleSignal;
  sigemptyset(&sa_new.sa_mask);
  sa_new.sa_flags = 0;
  sigaction(SIGINT, &sa_new, &sa_old);

  // make exception handling a little bit better (I think).
  // If some kind of system error occurs, it may affect more than 1
  // our our tasks so let's check each of them and log any exceptions
  // which occur. We'll then rethrow our first exception.

  // ok, get ready to handle keyboard interrupts, if any.

  HTTPS_Downloader::had_signal_ = false;

  std::exception_ptr ep = nullptr;

  int success_counter = 0;
  int error_counter = 0;

  for (int i = 0; i < file_list.size();) {
    // keep track of our async processes here.

    std::vector<std::future<void>> tasks;
    tasks.reserve(max_at_a_time + 1);

    for (; tasks.size() < max_at_a_time && i < file_list.size(); ++i) {
      // queue up our tasks up to the limit.

      auto &[remote_file, local_file] = file_list[i];
      if (remote_file) {
        tasks.emplace_back(std::async(std::launch::async,
                                      &HTTPS_Downloader::DownloadFile, this,
                                      *remote_file, local_file));
      }
      // std::cout << "i: " << i << " j: " << j << '\n';
    }

    // lastly, throw in our delay just in case we need it.

    tasks.emplace_back(
        std::async(std::launch::async, &HTTPS_Downloader::Timer, this));

    // now, let's wait till they're all done
    // and then we'll do the next bunch.

    for (int count = tasks.size(); count; --count) {
      int k = wait_for_any(tasks, 100us);
      // std::cout << "k: " << k << '\n';
      try {
        tasks[k].get();
        ++success_counter;
      } catch (std::system_error &e) {
        // any system problems, we eventually abort, but only after finishing
        // work in process.

        spdlog::error(e.what());
        auto ec = e.code();
        spdlog::error(catenate("Category: ", ec.category().name(), ". Value: ",
                               ec.value(), ". Message: ", ec.message()));
        ++error_counter;

        // OK, let's remember our first time here.

        if (!ep) {
          ep = std::current_exception();
        }
        continue;
      } catch (std::exception &e) {
        // any problems, we'll document them and continue.

        spdlog::error(e.what());
        ++error_counter;

        // OK, let's remember our first time here.

        if (!ep) {
          ep = std::current_exception();
        }
        continue;
      } catch (...) {
        // any problems, we'll document them and continue.

        spdlog::error("Unknown problem with an async download process");
        ++error_counter;

        // OK, let's remember our first time here.

        if (!ep) {
          ep = std::current_exception();
        }
        continue;
      }
    }

    if (ep || HTTPS_Downloader::had_signal_) {
      break;
    }

    // need to subtract 1 from our success_counter because of our timer task.
    --success_counter;
  }

  if (ep) {
    spdlog::error(catenate("Processed: ", file_list.size(),
                           " files. Successes: ", success_counter,
                           ". Errors: ", error_counter, "."));
    std::rethrow_exception(ep);
  }

  if (HTTPS_Downloader::had_signal_) {
    spdlog::error(catenate("Processed: ", file_list.size(),
                           " files. Successes: ", success_counter,
                           ". Errors: ", error_counter, "."));
    throw std::runtime_error(
        "Received keyboard interrupt.  Processing manually terminated.");
  }

  // if we return successfully, let's just restore the default

  sigaction(SIGINT, &sa_old, nullptr);

  return std::pair(success_counter, error_counter);

} // -----  end of method HTTPS_Downloader::DownloadFilesConcurrently  -----

void HTTPS_Downloader::Timer()

{
  //	given the size of the files we are downloading, it
  // seems unlikely this will have any effect.  But, for
  // small files it may.

  std::this_thread::sleep_for(1s);
}

void HTTPS_Downloader::HandleSignal(int signal)

{
  std::signal(SIGINT, HTTPS_Downloader::HandleSignal);

  // only thing we need to do

  HTTPS_Downloader::had_signal_ = true;
}

void DownloadTextFile(const fs::path &local_file_name,
                      const std::vector<char> &remote_data,
                      const fs::path &remote_file_name) {
  std::ofstream local_file{local_file_name, std::ios::out | std::ios::binary};
  if (!local_file || remote_data.empty()) {
    throw std::runtime_error(
        catenate("Unable to initiate download of remote file: ",
                 remote_file_name.string(),
                 " to local file: ", local_file_name.string()));
  }

  // we have an array of chars so let's just dump it out.

  errno = 0;
  auto &download_result =
      local_file.write(remote_data.data(), remote_data.size());
  local_file.close();
  if (download_result.fail()) {
    std::error_code err{errno, std::system_category()};
    throw std::system_error{
        err, catenate("Unable to complete download of remote file: ",
                      remote_file_name.string(),
                      " to local file: ", local_file_name.string())};
  }
}

void DownloadGZipFile(const fs::path &local_file_name,
                      const std::vector<char> &remote_data,
                      const fs::path &remote_file_name) {
  // we are going to decompress on the fly...

  std::ofstream expanded_file{local_file_name,
                              std::ios::out | std::ios::binary};
  if (!expanded_file || remote_data.empty()) {
    throw std::runtime_error(
        catenate("Unable to initiate download of remote file: ",
                 remote_file_name.string(),
                 " to local file: ", local_file_name.string()));
  }

  // for gzipped files, we can use boost (which uses zlib)
  // we need an appropriate data source.
  // (expansion filter only seems to work on the input side so some extra
  // steps...)

  boost::iostreams::array_source remote(remote_data.data(), remote_data.size());

  boost::iostreams::filtering_istream in;
  in.push(boost::iostreams::gzip_decompressor());
  in.push(remote);

  errno = 0;
  auto download_result = std::copy(
      std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>(),
      std::ostreambuf_iterator<char>{expanded_file});
  expanded_file.close();
  if (download_result.failed()) {
    std::error_code err{errno, std::system_category()};
    throw std::system_error{
        err, catenate("Unable to complete download of remote file: ",
                      remote_file_name.string(),
                      " to local file: ", local_file_name.string())};
  }
}

void DownloadZipFile(const fs::path &local_file_name,
                     const std::vector<char> &remote_data,
                     const fs::path &remote_file_name) {
  std::ofstream expanded_file{local_file_name,
                              std::ios::out | std::ios::binary};
  if (!expanded_file || remote_data.empty()) {
    throw std::runtime_error(
        catenate("Unable to initiate download of remote file: ",
                 remote_file_name.string(),
                 " to local file: ", local_file_name.string()));
  }

  // for zipped files, we need to use some non-boost code.
  // we'll try libzip for now.
  // it's not exactly straight-forward to use.  We need to do a buch of setup to
  // work with an in-memory buffer.

  zip_error_t err;
  zip_error_init(&err);

  // we have already downloaded the entire zipped file into a buffer.
  // now, we need to ziplibify it...

  zip_source_t *downloaded_raw_zip_data =
      zip_source_buffer_create(remote_data.data(), remote_data.size(), 0, &err);
  if (int err1 = zip_error_code_zip(&err); err1 != 0) {
    throw std::runtime_error(
        catenate("Problem setting up zip data for processing. ",
                 zip_error_strerror(&err)));
  }

  zip_t *zip_data_archive =
      zip_open_from_source(downloaded_raw_zip_data, ZIP_RDONLY, &err);
  if (int err1 = zip_error_code_zip(&err); err1 != 0) {
    throw std::runtime_error(catenate(
        "Problem opening zip data for processing. ", zip_error_strerror(&err)));
  }

  struct zip_stat st;
  zip_stat_init(&st);
  if (int stat_rc = zip_stat(zip_data_archive,
                             local_file_name.filename().c_str(), 0, &st);
      stat_rc != 0) {
    auto *stat_error = zip_get_error(zip_data_archive);
    throw std::runtime_error(catenate(
        "Unable to find file: ", local_file_name.filename().string(),
        " in downloaded zip archive.\n", zip_error_strerror(stat_error)));
  }

  // we're all set now, we've found our data in the archive so
  // we're just going to chunk through the archive and write the data out.
  // downloaded files can by 30MB or more in size. Can be smaller too..

  constexpr int buffer_size = 100'000;
  constexpr int pad_bytes = 100;
  std::array<char, buffer_size + pad_bytes> my_buffer;

  zip_int64_t total_bytes_read = 0;
  auto *downloaded_zip_file = zip_fopen(
      zip_data_archive, local_file_name.filename().c_str(), ZIP_RDONLY);
  if (downloaded_zip_file == nullptr) {
    auto *stat_error = zip_get_error(zip_data_archive);
    throw std::runtime_error(
        catenate("Unable to open compressed file: ", local_file_name.string(),
                 " in archive because: ", zip_error_strerror(stat_error)));
  }

  while (total_bytes_read < st.size) {
    auto bytes_read =
        zip_fread(downloaded_zip_file, my_buffer.data(), buffer_size);
    if (bytes_read == -1) {
      auto *stat_error = zip_get_error(zip_data_archive);
      throw std::runtime_error(
          catenate("Unable to read from zip archive because: ",
                   zip_error_strerror(stat_error)));
    }
    total_bytes_read += bytes_read;

    auto &download_result = expanded_file.write(my_buffer.data(), bytes_read);
    if (download_result.fail()) {
      std::error_code err{errno, std::system_category()};
      throw std::system_error{
          err, catenate("Unable to complete download of remote file: ",
                        remote_file_name.string(),
                        " to local file: ", local_file_name.string())};
    }
  }

  expanded_file.close();
  zip_fclose(downloaded_zip_file);
  zip_source_close(downloaded_raw_zip_data);
  zip_error_fini(&err);
}
