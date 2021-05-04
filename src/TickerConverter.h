// =====================================================================================
//
//       Filename:  TickerConverter.h
//
//    Description:  Header file for class which does file or web based lookup to convert
//    				a ticker ticker to an SEC CIK.
//
//        Version:  1.0
//        Created:  02/06/2014 01:54:27 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:
//
// =====================================================================================

#ifndef TICKERCONVERTER_H_
#define TICKERCONVERTER_H_

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


#include <filesystem>
#include <map>
#include <string>

namespace fs = std::filesystem;

#include "Collector_Utils.h"

// =====================================================================================
//        Class:  TickerConverter
//  Description:
// =====================================================================================
class TickerConverter
{
	public:

		using  TickerCIKMap = std::map<std::string, std::string>;

		// ====================  LIFECYCLE     =======================================

		// ====================  ACCESSORS     =======================================

		// ====================  MUTATORS      =======================================

		int UseCacheFile(const fs::path& cache_file_name);
		std::string ConvertTickerToCIK(const std::string& ticker, int pause=1);
		int DownloadTickerToCIKFile(const fs::path& ticker_file_name, const std::string& server_name="www.sec.gov", const std::string& port="443");
//		void SaveCIKDataToFile();
        TickerCIKMap ConvertFileOfTickersToCIKs(const fs::path& ticker_file_name);

		inline static constexpr const char* NotFound = "**no_CIK_found**";

		// ====================  OPERATORS     =======================================

	protected:

		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================

		TickerCIKMap ticker_to_CIK_;

		fs::path cache_file_name_;
		fs::path ticker_file_name_;

		std::size_t ticker_count_start_ = 0;
		std::size_t ticker_count_end_ = 0;


}; // -----  end of class TickerConverter  -----

std::string SEC_CIK_Lookup(COL::sview ticker, int pause=1);

#endif /* TICKERCONVERTER_H_ */
