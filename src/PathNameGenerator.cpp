// =====================================================================================
//
//       Filename:  PathNameGenerator.h
//
//    Description:  Implements class which knows how to generate EDGAR index file path names
//
//        Version:  1.0
//        Created:  06/29/2017 09:14:10 AM
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

#include "PathNameGenerator.h"

//--------------------------------------------------------------------------------------
//       Class:  PathNameGenerator
//      Method:  PathNameGenerator
// Description:  constructor
//--------------------------------------------------------------------------------------

//	NOTE:	The range validity edits on greg_year and greg_month don't allow you to use
//			meaningful default values.

PathNameGenerator::PathNameGenerator (void)
	: start_year_{2000}, end_year_{2000}, active_year_{2000}, start_month_{1}, end_month_{1}, active_month_{1}
{

}  // -----  end of method PathNameGenerator  (constructor)  -----


//--------------------------------------------------------------------------------------
//       Class:  PathNameGenerator
//      Method:  PathNameGenerator
// Description:  constructor
//--------------------------------------------------------------------------------------
PathNameGenerator::PathNameGenerator (const fs::path& prefix, const bg::date& start_date, const bg::date& end_date)
	: start_date_{start_date}, end_date_{end_date},
		start_year_{start_date.year()}, end_year_{end_date.year()}, active_year_{start_date.year()},
	    start_month_{start_date.month()}, end_month_{end_date.month()}, active_month_{start_date.month()},
		remote_directory_prefix_{prefix}
{
	// compute the first day of the quarter and use that as the base of calculations

	active_date_ = bg::date(start_date.year(), (start_date.month() / 3 + (start_date.month() % 3 == 0 ? 0 : 1)) * 3 - 2, 1);
	active_month_ = active_date_.month();

	EDGAR_path_ = remote_directory_prefix_;
	EDGAR_path_ /= std::to_string(active_year_);
	EDGAR_path_ /= "QTR" + std::to_string(active_month_ / 3 + (active_month_ % 3 == 0 ? 0 : 1));

}  // -----  end of method PathNameGenerator::PathNameGenerator  (constructor)  -----


void PathNameGenerator::increment (void)
{
	bg::months a_quarter{3};
	active_date_ += a_quarter;

	if (active_date_ > end_date_)
	{
		//	we need to become equal to and 'end' iterator

		active_year_ = start_year_;
		active_month_ = start_month_;
		EDGAR_path_.clear();
	}
	else
	{
		active_year_ = active_date_.year();
		active_month_ = active_date_.month();

		EDGAR_path_ = remote_directory_prefix_;
		EDGAR_path_ /= std::to_string(active_year_);
		EDGAR_path_ /= "QTR" + std::to_string(active_month_ / 3 + (active_month_ % 3 == 0 ? 0 : 1));
	}
}		// -----  end of method PathNameGenerator::increment  -----
