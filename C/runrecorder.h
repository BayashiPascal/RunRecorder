// ------------------ runrecorder.h ------------------

// Guard against multiple inclusions
#ifndef RUNRECORDER_H
#define RUNRECORDER_H

// Include external modules header
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <sqlite3/sqlite3.h>
#include <curl/curl.h>
#include <errno.h>

// Include own mdules header
#include "trycatch.h"

// Structure 
struct RunRecorder {

  // Path to the SQLite database or Web API
  // If the url starts with 'http' the Web API located at this url
  // will be used, else url is considered to be the path to a local
  // SQLite database file.
  char* url;

  // String to memorise the eventual error message
  char* errMsg;

  // Connection to the database if it's a local file
  sqlite3* db;

  // Curl instance if we use the Web API
  CURL* curl;

  // String to memorise the reply from Curl requests
  char* curlReply;

};

// Constructor for a struct RunRecorder
// Input:
//   url: Path to the SQLite database or Web API
// Output:
//  Return a new struct RunRecorder
struct RunRecorder* RunRecorderCreate(
  char const* const url);

// Initialise a struct RunRecorder
// Input:
//   that: The struct RunRecorder
// Raise: 
//   RunRecorderExc_CreateTableFailed
//   RunRecorderExc_OpenDbFailed
//   RunRecorderExc_CreateCurlFailed
//   RunRecorderExc_CurlSetOptFailed
//   RunRecorderExc_SQLRequestFailed
//   RunRecorderExc_ApiRequestFailed
//   RunRecorderExc_MallocFailed
void RunRecorderInit(
  struct RunRecorder* const that);

// Destructor for a struct RunRecorder
// Input:
//   that: The struct RunRecorder to be freed
void RunRecorderFree(
  struct RunRecorder** that);

// Get the version of the database
// Input:
//   that: the struct RunRecorder
// Output:
//   Return a new string
// Raise:
//   RunRecorderExc_SQLRequestFailed
//   RunRecorderExc_CurlSetOptFailed
//   RunRecorderExc_CurlRequestFailed
//   RunRecorderExc_ApiRequestFailed
//   RunRecorderExc_MallocFailed
char* RunRecorderGetVersion(
  struct RunRecorder* const that);

// Add a new projet
// Input:
//   that: the struct RunRecorder
//   name: the name of the new project, double quote `"`, equal sign `=` and
//         ampersand `&` can't be used in the project's name
// Output:
//   Return the reference of the new project
// Raise:
//   RunRecorderExc_SQLRequestFailed
//   RunRecorderExc_CurlSetOptFailed
//   RunRecorderExc_CurlRequestFailed
//   RunRecorderExc_ApiRequestFailed
//   RunRecorderExc_MallocFailed
//   RunRecorderExc_InvalidProjectName
//   RunRecorderExc_AddProjectFailed
long RunRecorderAddProject(
  struct RunRecorder* const that,
  char const* const name);

// End of the guard against multiple inclusion
#endif
// ------------------ runrecorder.h ------------------
