CollectEDGARData
================

Download manager for SEC EDGAR data files

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



