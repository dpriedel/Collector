Collector
================

Download manager for SEC EDGAR data files

May 4, 2021. Version 5.0 update:

It's been a while and things at the SEC EDGAR site have changed and so has this program.

The applications are no longer Poco application framework projects.  It's back to Boost.

There have been a number of changes due to compiler/boost/other library udates. See the 'building' file for changes.

I'm using the Boost Beast library to handle file downloads and HTTPS/SSL interactions.  Still not using
SSL Certificate validation though.

The application now supports optional concurrent downloads.  The SEC site has a limit of 10 connections per second.
The application allows you to use a higher number but you will likely be stopped the the site.

This project is part of a set of projects to make use of the SEC's EDGAR data filings available on Linux computers.
It is also a chance to explore using C++17 through 23 and to try out Test Driven Development with C++.  

This program serves to download selected EDGAR filing files from the SEC's FTP site.  The files to be downloaded
can be filtered by date range, form type (10-Q, 4, etc.) and ticker symbol.  Files are downloaded to a 
specified location on your system where they can be viewed in a browser or run through a program to extract
desired information.  (The next project in this set of projects will extract data from 10-Q and 10-K forms).

There are 2 sets of files to deal with -- Index files and Form files.  Index files can be either Daily index files
for the current quarter or Quarterly index files for any quarter going back to 1994.  The record layout in the files
is the same.  The only difference is how many records are in the index files.

EDGAR form files are identified by CIK number which is assigned by the EDGAR system.  This program will also do a
ticker to CIK conversion.  You can provide a file with a list of stock ticker symbols and the corresponding CIK will
be found.  The results are saved in file on your system so you can reuse them in future runs.  This feature used to do 
repeated queries to the SEC site for the conversions but now, it just downloads the entire file which became available some
time ago.



