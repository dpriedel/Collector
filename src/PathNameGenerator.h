// =====================================================================================
//
//       Filename:  PathNameGenerator.h
//
//    Description:  Header for class which knows how to generate EDGAR index file path names
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

#ifndef PATHNAMEGENERATOR_H_
#define PATHNAMEGENERATOR_H_

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


#include <string>
#include <experimental/filesystem>

// #include <boost/filesystem.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace bg = boost::gregorian;
namespace fs = std::experimental::filesystem;

class PathNameGenerator : public boost::iterator_facade
<
	PathNameGenerator,
	fs::path const,
	boost::forward_traversal_tag
>
{
public:

	PathNameGenerator(void);
	PathNameGenerator(const fs::path& prefix, const bg::date& start_date, const bg::date& end_date);

private:

	friend class boost::iterator_core_access;

	void increment();

	bool equal(PathNameGenerator const& other) const
	{
		return this->EDGAR_path_ == other.EDGAR_path_;
	}

	fs::path const& dereference() const { return EDGAR_path_; }

	bg::date start_date_;
	bg::date end_date_;
	bg::date working_date_;

	bg::greg_year start_year_;
	bg::greg_year end_year_;
	bg::greg_year working_year_;
	bg::greg_month start_month_;
	bg::greg_month end_month_;
	bg::greg_month working_month_;

	fs::path remote_directory_prefix_;
	fs::path EDGAR_path_;
};

#endif /* PATHNAMEGENERATOR_H_ */
