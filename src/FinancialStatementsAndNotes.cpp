// =====================================================================================
//
//       Filename:  FinancialStatementsAndNotes.cpp
//
//    Description:  Module to manage download of SEC Financial Statement and Notes data files.
//
//        Version:  1.0
//        Created:  05/04/2021 02:32:42 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================
//

//--------------------------------------------------------------------------------------
//       Class:  FinancialStatementsAndNotes_gen
//      Method:  FinancialStatementsAndNotes_gen
// Description:  constructor
//--------------------------------------------------------------------------------------

#include <iostream>

#include <fmt/format.h>

#include "FinancialStatementsAndNotes.h"
#include "HTTPS_Downloader.h"

FinancialStatementsAndNotes_gen::FinancialStatementsAndNotes_gen (date::year_month_day start_date, date::year_month_day end_date)
{
    // when in quarterly mode, the 'month' value will actually be the 'quarter' value between 1 and 4.
    // maybe there's a better way to do this. Nonetheless, it keeps the code conistent.

    date::year_month temp_s{start_date.year(), start_date.month()};
    if (temp_s > last_quarterly_)
    {
        monthly_mode_ = true;
	    start_date_ = temp_s;
    }
    else
    {
        // convert to our faux quarterly value
        start_date_ = date::year_month{start_date.year(), date::month{start_date.month().operator unsigned int() / 3 + (start_date.month().operator unsigned int() % 3 == 0 ? 0 : 1)}};
    }

    date::year_month temp_e{end_date.year(), end_date.month()};
    if (temp_e > last_quarterly_)
    {
        end_date_ = temp_e;
    }
    else
    {
        // convert to our faux quarterly value
	    end_date_ = date::year_month{end_date.year(), date::month{end_date.month().operator unsigned int() / 3 + (end_date.month().operator unsigned int() % 3 == 0 ? 0 : 1)}};
    }
    current_date_ = start_date_;

    format_current_value();

}  // -----  end of method FinancialStatementsAndNotes_gen::FinancialStatementsAndNotes_gen  (constructor)  ----- 


FinancialStatementsAndNotes_gen& FinancialStatementsAndNotes_gen::operator++ ()
{
    // see if we're done

    if ( current_date_ == date::year_month{})
    {
        return *this;
    }

    if (! monthly_mode_)
    {
        // we need to do 'quarterly' arithmetic here.

        current_date_ += a_month;
        if (current_date_.month().operator unsigned int() > 4)
        {
            current_date_ = {current_date_.year() + a_year, date::January};
        }
        if (current_date_ > last_quarterly_qtr)
        {
            current_date_ = first_monthly_;
            monthly_mode_ = true;
        }
    }
    else
    {
        current_date_ += a_month;
    }

    if ( current_date_ >= end_date_)
    {
        current_date_ = date::year_month();
        current_value_ = {"", ""};
        return *this;
    }

    format_current_value();

    return *this;
}		// -----  end of method FinancialStatementsAndNotes_gen::operator++  ----- 


void FinancialStatementsAndNotes_gen::format_current_value ()
{
    if (! monthly_mode_)
    {
        current_value_.first = fmt::format("{}q{}_notes.zip", current_date_.year().operator int(), current_date_.month().operator unsigned int());
        current_value_.second = fmt::format("{}_{}", current_date_.year().operator int(), current_date_.month().operator unsigned int());
    }
    else
    {
        current_value_.first = fmt::format("{}_{:>02d}_notes.zip", current_date_.year().operator int(), current_date_.month().operator unsigned int());
        current_value_.second = fmt::format("{}_{}", current_date_.year().operator int(), current_date_.month().operator unsigned int());
    }
}		// -----  end of method FinancialStatementsAndNotes_gen::format_current_value  ----- 


//--------------------------------------------------------------------------------------
//       Class:  FinancialStatementsAndNotes
//      Method:  FinancialStatementsAndNotes
// Description:  constructor
//--------------------------------------------------------------------------------------
FinancialStatementsAndNotes::FinancialStatementsAndNotes (date::year_month_day start_date, date::year_month_day end_date)
    : start_date_{start_date}, end_date_{end_date}
{
}  // -----  end of method FinancialStatementsAndNotes::FinancialStatementsAndNotes  (constructor)  ----- 


    // files/dera/data/financial-statement-and-notes-data-sets/
void FinancialStatementsAndNotes::download_files (const std::string& server_name, const std::string& port, const fs::path& download_destination)
{
    // for now, just download

    HTTPS_Downloader fin_statement_downloader{server_name, port};
    if (! fs::exists(download_destination))
    {
        fs::create_directories(download_destination);
    }

    return ;
}		// -----  end of method FinancialStatementsAndNotes::download_files  ----- 

