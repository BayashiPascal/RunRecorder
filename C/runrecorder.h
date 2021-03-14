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
#include <time.h>
#include <sqlite3/sqlite3.h>
#include <curl/curl.h>
#include <errno.h>

// Include own modules header
#include "trycatch.h"

// Labels for the exception
char* RunRecorderExceptionStr[RunRecorderExc_LastID];

// Structure of a RunRecorder
struct RunRecorder {

  // Path to the SQLite database or Web API
  // If the url starts with 'http' the Web API located at this url
  // will be used, else url is considered to be the path to a local
  // SQLite database file.
  char* url;

  // String to memorise the error message
  char* errMsg;

  // String to memorise the SQLite3 error message from sqlite3_exec
  // (they need to be managed separately to be freed their own way)
  char* sqliteErrMsg;

  // Connection to the database if it's a local file
  sqlite3* db;

  // Curl instance if we use the Web API
  CURL* curl;

  // String to memorise the reply from Curl requests
  char* curlReply;

  // String to memorise the API or SQL commands when they are
  // dynamically allocated to make memory management easier if an
  // exception is raised
  char* cmd;

  // Reference of the last added measure
  sqlite3_int64 refLastAddedMeasure;

};

// Structure to memorise pairs of ref/value
struct RunRecorderPairsRefVal {

  // Number of pairs
  long nb;

  // Array of references
  long* refs;

  // Array of values
  char** vals;

};

// Structure to add one measurement (i.e. a set of metrics and their
// value for one project)
struct RunRecorderMeasure {

  // Number of values
  long nbVal;

  // Array of metrics label
  char** metrics;

  // Array of values
  char** vals;

};

// Constructor for a struct RunRecorder
// Input:
//   url: Path to the SQLite database or Web API
// Output:
//  Return a new struct RunRecorder
// Raise:
//   RunRecorderExc_MallocFailed
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

// Add a new project
// Input:
//   that: the struct RunRecorder
//   name: the name of the new project
// The project's name must respect the following pattern: 
// /^[a-zA-Z][a-zA-Z0-9_]*$/ .
// Raise:
//   RunRecorderExc_SQLRequestFailed
//   RunRecorderExc_CurlSetOptFailed
//   RunRecorderExc_CurlRequestFailed
//   RunRecorderExc_ApiRequestFailed
//   RunRecorderExc_MallocFailed
//   RunRecorderExc_InvalidProjectName
//   RunRecorderExc_ProjectNameAlreadyUsed
//   RunRecorderExc_AddProjectFailed
void RunRecorderAddProject(
  struct RunRecorder* const that,
  char const* const name);

// Get the list of projects
// Input:
//   that: the struct RunRecorder
// Output:
//   Return the projects' reference/label
// Raise:
//   RunRecorderExc_SQLRequestFailed
//   RunRecorderExc_CurlSetOptFailed
//   RunRecorderExc_CurlRequestFailed
//   RunRecorderExc_MallocFailed
//   RunRecorderExc_ApiRequestFailed
//   RunRecorderExc_InvalidJSON
struct RunRecorderPairsRefVal* RunRecorderGetProjects(
  struct RunRecorder* const that);

// Add a metric to a project
// Input:
//         that: the struct RunRecorder
//      project: the name of the project to which add the metric
//        label: the label of the metric. 
//   defaultVal: the default value of the metric 
// The label of the metric must respect the following pattern:
// /^[a-zA-Z][a-zA-Z0-9_]*$/.
// The default of the metric must be one character long at least.
// The double quote `"`, equal sign `=` and ampersand `&` can't be used in
// the default value. There cannot be two metrics with the same label for
// the same project. A metric label can't be 'action' or 'project' (case
//  sensitive, so 'Action' is fine).
// Raise:
//   RunRecorderExc_SQLRequestFailed
//   RunRecorderExc_MallocFailed
//   RunRecorderExc_InvalidMetricName
//   RunRecorderExc_MetricNameAlreadyUsed
void RunRecorderAddMetric(
  struct RunRecorder* const that,
          char const* const project,
          char const* const label,
          char const* const defaultVal);

// Get the list of metrics for a project
// Input:
//      that: the struct RunRecorder
//   project: the project
// Output:
//   Return the metrics' reference/label
// Raise:
//   RunRecorderExc_SQLRequestFailed
//   RunRecorderExc_CurlSetOptFailed
//   RunRecorderExc_CurlRequestFailed
//   RunRecorderExc_MallocFailed
//   RunRecorderExc_ApiRequestFailed
//   RunRecorderExc_InvalidJSON
struct RunRecorderPairsRefVal* RunRecorderGetMetrics(
  struct RunRecorder* const that,
          char const* const project);

// Add a measure to a project
// Inputs:
//       that: the struct RunRecorder
//    project: the project to add the measure to
//    measure: the measure to add
// Raise:

void RunRecorderAddMeasure(
               struct RunRecorder* const that,
                       char const* const project,
  struct RunRecorderMeasure const* const measure);

// Delete a measure
// Inputs:
//       that: the struct RunRecorder
//    measure: the measure to delete
// Raise:

void RunRecorderDeleteMeasure(
  struct RunRecorder* const that,
        sqlite3_int64 const measure);

// Create a static struct RunRecorderPairsRefVal
// Output:
//   Return the new struct RunRecorderPairsRefVal
struct RunRecorderPairsRefVal* RunRecorderPairsRefValCreate(
  void);

// Free a static struct RunRecorderPairsRefVal
// Input:
//   that: the struct RunRecorderPairsRefVal
void RunRecorderPairsRefValFree(
  struct RunRecorderPairsRefVal** that);

// Create a static struct RunRecorderMeasure
// Output:
//   Return the new struct RunRecorderMeasure
struct RunRecorderMeasure* RunRecorderMeasureCreate(
  void);

// Free a static struct RunRecorderMeasure
// Input:
//   that: the struct RunRecorderMeasure
void RunRecorderMeasureFree(
  struct RunRecorderMeasure** that);

// Add a value to a measure
// Input:
//     that: the struct RunRecorderMeasure
//   metric: the value's metric
//      val: the value
// Raise:
//   RunRecorderExc_MallocFailed
void RunRecorderMeasureAddValueStr(
  struct RunRecorderMeasure* that,
           char const* const metric,
           char const* const val);

void RunRecorderMeasureAddValueInt(
  struct RunRecorderMeasure* that,
           char const* const metric,
                  long const val);

void RunRecorderMeasureAddValueFloat(
  struct RunRecorderMeasure* that,
           char const* const metric,
                double const val);

#define RunRecorderMeasureAddValue(T, M, V) _Generic(V, \
  char*: RunRecorderMeasureAddValueStr, \
  char const*: RunRecorderMeasureAddValueStr, \
  int: RunRecorderMeasureAddValueInt, \
  unsigned int: RunRecorderMeasureAddValueInt, \
  long: RunRecorderMeasureAddValueInt, \
  float: RunRecorderMeasureAddValueFloat, \
  double: RunRecorderMeasureAddValueFloat)(T, M, V)

// End of the guard against multiple inclusion
#endif
// ------------------ runrecorder.h ------------------
