// =====================================================================================
//
//       Filename:  CollectEDGARApp.h
//
//    Description:  main application
//
//        Version:  1.0
//        Created:  01/17/2014 11:13:53 AM
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


// =====================================================================================
//        Class:  CollectEDGARApp
//  Description:
// =====================================================================================

#ifndef COLLECTEDGARAPP_H_
#define COLLECTEDGARAPP_H_

#include <fstream>
#include <map>

#include <boost/filesystem.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace bg = boost::gregorian;
namespace fs = boost::filesystem;

#include "CApplication.h"
#include "TickerConverter.h"

class CollectEDGARApp : public CApplication
{
	public:
		// ====================  LIFECYCLE     =======================================
		CollectEDGARApp (int argc, char* argv[]);                             // constructor
		CollectEDGARApp(int argc, char* argv[], const std::vector<std::string>& tokens);

		// ====================  ACCESSORS     =======================================

		// ====================  MUTATORS      =======================================

		// ====================  OPERATORS     =======================================

	protected:

	virtual void	Do_StartUp (void);
	virtual void	Do_CheckArgs (void);
	virtual void	Do_Run (void);
	virtual void	Do_Quit (void);
	virtual	void	Do_SetupProgramOptions(void);
	virtual	void	Do_ParseProgramOptions(po::parsed_options& options);

	void Do_Run_DailyIndexFiles(void);
	void Do_Run_QuarterlyIndexFiles(void);
	void Do_Run_TickerLookup(void);
	void Do_Run_TickerFileLookup(void);

	void Do_TickerMap_Setup(void);

		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================

	struct comma_list_parser
	{
		std::vector<std::string>& destination_;
		std::string seperator_;

		comma_list_parser(std::vector<std::string>& destination, const std::string& seperator)
			: destination_(destination), seperator_{seperator} {}

		void parse_string(const std::string& comma_list);
	};

	TickerConverter ticker_converter_;

	bg::date begin_date_;
	bg::date end_date_;

	std::string mode_;
	std::string form_;
	std::string ticker_;
	std::string login_ID_;
    std::string FTP_host_;

	std::vector<std::string> form_list_;
	std::vector<std::string> ticker_list_;

	std::map<std::string, std::string> ticker_map_;

	fs::path log_file_path_name_;
	fs::path local_index_file_directory_;
	fs::path local_form_file_directory_;
	fs::path ticker_cache_file_name_;
	fs::path ticker_list_file_name_;

	std::ofstream log_file_;
	std::streambuf* saved_from_clog_;

	int pause_;
    int max_forms_to_download_;     // mainly for testing

	bool replace_index_files_;
	bool replace_form_files_;
	bool index_only_;			//	do no download any form files

}; // -----  end of class CollectEDGARApp  -----

#endif /* COLLECTEDGARAPP_H_ */
