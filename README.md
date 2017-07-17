CollectEDGARData
================

Download manager for SEC EDGAR data files

July 17, 2017. Version 1.9 update:

It's been a while and things at the EDGAR site have changed.

The applications are now Poco application framework projects (pocoproject.org).

There have been a number of changes due to compiler/boost/Poco udates. Every thing is current right now -- gcc 7.1,
boost 1-64, Poco 1.7.8p3.

Most importantly, the EDGAR site has changed from FTP access to HTTPS access.  I have used the Poco library to
implement support for HTTPS access with SSL verification.  (The SSL verification is turned off for now because
my local test setup does not have a 'valid' SSL cert.)

Also, I have refactored a lot of the code related to file downloads in preparation for the main feature to be
added next -- concurrent downloads.  According the the EDGAR web site, one is allowed up to 10 requests per second.
I will be adding code to support this feature.  This will make the code version 2.0.


This project is part of a set of projects to make use of the SEC's EDGAR data filings available on Linux computers.
It is also a chance to explore using C++11 and to try out Test Driven Development with C++.  

This program serves to download selected EDGAR filing files from the SEC's FTP site.  The files to be downloaded
can be filtered by date range, form type (10-Q, 4, etc.) and ticker symbol.  Files are downloaded to a 
specified location on your system where they can be viewed in a browser or run through a program to extract
desired information.  (The next project in this set of projects will extract data from 10-Q and 10-K forms).

There are 2 sets of files to deal with -- Index files and Form files.  Index files can be either Daily index files
for the current quarter or Quarterly index files for any quarter going back to 1994.  The record layout in the files
is the same.  The only difference is how many records are in the index files.

EDGAR form files are identified by CIK number which is assigned by the EDGAR system.  This program will also do a
ticker to CIK conversion.  You can provide a file with a list of stock ticker symbols and the corresponding CIK will
be found.  The results are saved in file on your system so you can reuse them in future runs.



