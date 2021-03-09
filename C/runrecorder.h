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

};

// Constructor for a struct RunRecorder
// Raise: TryCatchException_CreateTableFailed,
// TryCatchException_OpenDbFailed,
// TryCatchException_CreateCurlFailed,
// TryCatchException_CurlRequestFailed,
// TryCatchException_CurlSetOptFailed,
// TryCatchException_SQLRequestFailed
struct RunRecorder* RunRecorderCreate(
  // Path to the SQLite database or Web API
  char const* const url);

// Destructor for a struct RunRecorder
void RunRecorderFree(
  // The struct RunRecorder to be freed
  struct RunRecorder** that);

// Get the version of the database
// Return a new string
char* RunRecorderGetVersion(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Create a new project
// Return true on success, else return false and set that->errMsg
bool RunRecorderAddProject(
  // The struct RunRecorder
  struct RunRecorder* const that,
  // The project's name. The double quote `"`, equal sign `=` and
  // ampersand `&` can't be used in the project's name.
  char const* const name);

// End of the guard against multiple inclusion
#endif
// ------------------ runrecorder.h ------------------
