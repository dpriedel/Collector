/*
 * =====================================================================================
 *
 *       Filename:  Collector_Utils.cpp
 *
 *    Description:  Useful routines from Extractor code base
 *
 *        Version:  1.0
 *        Created:  04/22/2019 03:03:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  David P. Riedel (), driedel@cox.net
 *        License:  GNU General Public License V3
 *
 * =====================================================================================
 */

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

#include "Collector_Utils.h"


/*
 *--------------------------------------------------------------------------------------
 *       Class:  AssertionException
 *      Method:  AssertionException
 * Description:  constructor
 *--------------------------------------------------------------------------------------
 */
Collector::AssertionException::AssertionException(const char* text)
    : std::invalid_argument(text)
{
}  /* -----  end of method AssertionException::AssertionException  (constructor)  ----- */

Collector::AssertionException::AssertionException(const std::string& text)
    : std::invalid_argument(text)
{
}  /* -----  end of method AssertionException::AssertionException  (constructor)  ----- */

/*
 *--------------------------------------------------------------------------------------
 *       Class:  TimeOutException
 *      Method:  TimeOutException
 * Description:  constructor
 *--------------------------------------------------------------------------------------
 */
Collector::TimeOutException::TimeOutException(const char* text)
    : std::runtime_error(text)
{
}  /* -----  end of method TimeOutException::TimeOutException  (constructor)  ----- */

Collector::TimeOutException::TimeOutException(const std::string& text)
    : std::runtime_error(text)
{
}  /* -----  end of method TimeOutException::TimeOutException  (constructor)  ----- */


namespace boost
{
    // these functions are declared in the library headers but left to the user to define.
    // so here they are...
    //
    /* 
     * ===  FUNCTION  ======================================================================
     *         Name:  assertion_failed_mgs
     *  Description:  
     *         defined in boost header but left to us to implement.
     * =====================================================================================
     */

    void assertion_failed_msg (char const* expr, char const* msg, char const* function, char const* file, long line)
    {
        throw Collector::AssertionException(catenate("\n*** Assertion failed *** test: ", expr, " in function: ", function,
                    " from file: ", file, " at line: ", line, ".\nassertion msg: ", msg));
    }		/* -----  end of function assertion_failed_mgs  ----- */

    /* 
     * ===  FUNCTION  ======================================================================
     *         Name:  assertion_failed
     *  Description:  
     * =====================================================================================
     */
    void assertion_failed (char const* expr, char const* function, char const* file, long line )
    {
        throw Collector::AssertionException(catenate("\n*** Assertion failed *** test: ", expr, " in function: ", function,
                    " from file: ", file, " at line: ", line));
    }		/* -----  end of function assertion_failed  ----- */

}		/* -----  end of namespace boost  ----- */
