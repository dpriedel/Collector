// =====================================================================================
//
//       Filename:  TickerConverter.h
//
//    Description:  Header file for class which does file or web based lookup to convert
//    				a ticker ticker to an EDGAR CIK.
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


#include <map>
#include <string>
#include <filesystem>

// #include <boost/filesystem.hpp>
#include "Poco/Logger.h"

namespace fs = std::filesystem;

// =====================================================================================
//        Class:  TickerConverter
//  Description:
// =====================================================================================
class TickerConverter
{
	public:

		using  TickerCIKMap = std::map<std::string, std::string>;

		// ====================  LIFECYCLE     =======================================
		TickerConverter (Poco::Logger& the_logger);                             // constructor

		// ====================  ACCESSORS     =======================================

		// ====================  MUTATORS      =======================================

		void UseCacheFile(const fs::path& cache_file_name);
		std::string ConvertTickerToCIK(const std::string& ticker, int pause=1);
		int ConvertTickerFileToCIKs(const fs::path& ticker_file_name, int pause=1);
		void SaveCIKDataToFile(void);

		static constexpr char NotFound[] = "**no_CIK_found**";

		// ====================  OPERATORS     =======================================

	protected:

		//	NOTE: make this function virtual to run Google MOCK tests for it.

		std::string EDGAR_CIK_Lookup(const std::string& ticker, int pause=1);

		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================

		TickerCIKMap ticker_to_CIK_;

		fs::path cache_file_name_;
		fs::path ticker_file_name_;

        Poco::Logger& the_logger_;

		std::size_t ticker_count_start_ = 0;
		std::size_t ticker_count_end_ = 0;


}; // -----  end of class TickerConverter  -----

#endif /* TICKERCONVERTER_H_ */
