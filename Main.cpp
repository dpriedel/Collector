// =====================================================================================
//
//       Filename:  Main.cpp
//
//    Description:  Driver program for application
//
//        Version:  1.0
//        Created:  02/28/2014 03:04:02 PM
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


#include <iostream>

#include "spdlog/spdlog.h"

#include "CollectorApp.h"
#include "Collector_Utils.h"

int main(int argc, char** argv)
{
	//	help to optimize c++ stream I/O (may screw up threaded I/O though)

	std::ios_base::sync_with_stdio(false);

	int result = 0;
	try
	{
		CollectorApp myApp(argc, argv);
        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
		result = 7;
	}
	catch (...)
	{
        spdlog::error("Something totally unexpected happened.");
		result = 9;
	}

	return result;
}
