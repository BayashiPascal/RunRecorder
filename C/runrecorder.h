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
#include <SQLite3/sqlite3.h>
#include <curl/curl.h>
#include <errno.h>
#include <TryCatchC/trycatchc.h>

// ================== RunRecorder Exceptions =========================

enum RunRecorderException {

  RunRecorderExc_CreateTableFailed = 100,
  RunRecorderExc_OpenDbFailed,
  RunRecorderExc_CreateCurlFailed,
  RunRecorderExc_CurlRequestFailed,
  RunRecorderExc_CurlSetOptFailed,
  RunRecorderExc_SQLRequestFailed,
  RunRecorderExc_ApiRequestFailed,
  RunRecorderExc_InvalidProjectName,
  RunRecorderExc_ProjectNameAlreadyUsed,
  RunRecorderExc_FlushProjectFailed,
  RunRecorderExc_AddProjectFailed,
  RunRecorderExc_AddMetricFailed,
  RunRecorderExc_UpdateViewFailed,
  RunRecorderExc_InvalidJSON,
  RunRecorderExc_InvalidMetricLabel,
  RunRecorderExc_InvalidMetricDefVal,
  RunRecorderExc_InvalidValue,
  RunRecorderExc_MetricNameAlreadyUsed,
  RunRecorderExc_AddMeasureFailed,
  RunRecorderExc_DeleteMeasureFailed,
  RunRecorderExc_LastID

};

// ================== Structures definitions =========================

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

  // String to memorise the API or SQL commands
  char* cmd;

  // Reference of the last added measure
  long refLastAddedMeasure;

};

// Structure to memorise pairs of ref/value
struct RunRecorderPairsRefVal {

  // Number of pairs
  long nb;

  // Array of reference
  long* refs;

  // Array of value as string
  char** values;

};

// Structure to memorise pairs of ref/value with their default value
struct RunRecorderPairsRefValDef {

  // Number of pairs
  long nb;

  // Array of reference
  long* refs;

  // Array of value as string
  char** values;

  // Array of default value as string
  char** defaultValues;

};

// Structure to add one measurement (i.e. a set of metrics and their
// value for one project)
struct RunRecorderMeasure {

  // Number of metrics
  long nbMetric;

  // Array of metrics label
  char** metrics;

  // Array of values as string
  char** values;

};

// Structure to memorise the measures of one project
struct RunRecorderMeasures {

  // Number of measures
  long nbMeasure;

  // Number of metrics
  long nbMetric;

  // Array of metrics label
  char** metrics;

  // Array of array of values as string, to be used as
  // values[iMeasure][iMetric]
  char*** values;

};

// ================== Public functions declarations =========================

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
void RunRecorderInit(
  struct RunRecorder* const that);

// Free memory used by a struct RunRecorder
// Input:
//   that: The struct RunRecorder to be freed
void RunRecorderFree(
  struct RunRecorder** const that);

// Get the version of the database
// Input:
//   that: the struct RunRecorder
// Output:
//   Return the version as a new string
char* RunRecorderGetVersion(
  struct RunRecorder* const that);

// Check if a string respects the pattern /^[a-zA-Z][a-zA-Z0-9_]*$/
// Input:
//   str: the string to check
// Output:
//   Return true if the string respects the pattern, else false
bool RunRecorderIsValidLabel(
  char const* const str);

// Check if a string respects the pattern /^[^"=&]+$/
// Input:
//   str: the string to check
// Output:
//   Return true if the string respects the pattern, else false
bool RunRecorderIsValidValue(
  char const* const str);

// Add a new project
// Input:
//   that: the struct RunRecorder
//   name: the name of the new project, it must respect the following
//         pattern: /^[a-zA-Z][a-zA-Z0-9_]*$/
// Raise:
//   RunRecorderExc_InvalidProjectName
//   RunRecorderExc_ProjectNameAlreadyUsed
void RunRecorderAddProject(
  struct RunRecorder* const that,
          char const* const name);

// Get the list of projects
// Input:
//   that: the struct RunRecorder
// Output:
//   Return the projects' reference/label as a new struct
//   RunRecorderPairsRefVal
struct RunRecorderPairsRefVal* RunRecorderGetProjects(
  struct RunRecorder* const that);

// Get the list of metrics for a project
// Input:
//      that: the struct RunRecorder
//   project: the project
// Output:
//   Return the metrics' reference/label/default value as a new struct
//   RunRecorderPairsRefValDef
struct RunRecorderPairsRefValDef* RunRecorderGetMetrics(
  struct RunRecorder* const that,
          char const* const project);

// Add a metric to a project
// Input:
//         that: the struct RunRecorder
//      project: the name of the project to which add to the metric
//        label: the label of the metric, it must respect the following
//               pattern: /^[a-zA-Z][a-zA-Z0-9_]*$/
//               There cannot be two metrics with the same label for the
//               same project. A metric label can't be 'action' or 'project'
//               (case sensitive, so 'Action' is fine).
//   defaultVal: the default value of the metric, it must respect the
//               following pattern: /^[^"=&]+$*/
// Raise:
//   RunRecorderExc_InvalidMetricLabel
//   RunRecorderExc_MetricNameAlreadyUsed
void RunRecorderAddMetric(
  struct RunRecorder* const that,
          char const* const project,
          char const* const label,
          char const* const defaultVal);

// Create a new struct RunRecorderMeasure
// Output:
//   Return the new struct RunRecorderMeasure
struct RunRecorderMeasure* RunRecorderMeasureCreate(
  void);

// Free a struct RunRecorderMeasure
// Input:
//   that: the struct RunRecorderMeasure
void RunRecorderMeasureFree(
  struct RunRecorderMeasure** const that);

// Add a string value to a struct RunRecorderMeasure if there is not yet a
// value for the metric, or replace its value else
// Input:
//     that: the struct RunRecorderMeasure
//   metric: the value's metric
//      val: the value
// Raise
//   RunRecorderExc_InvalidValue
void RunRecorderMeasureAddValueStr(
  struct RunRecorderMeasure* const that,
                 char const* const metric,
                 char const* const val);

// Add an int value to a struct RunRecorderMeasure if there is not yet a
// value for the metric, or replace its value else
// Input:
//     that: the struct RunRecorderMeasure
//   metric: the value's metric
//      val: the value
void RunRecorderMeasureAddValueInt(
  struct RunRecorderMeasure* const that,
                 char const* const metric,
                        long const val);

// Add a double value to a struct RunRecorderMeasure if there is not yet a
// value for the metric, or replace its value else
// Input:
//     that: the struct RunRecorderMeasure
//   metric: the value's metric
//      val: the value
void RunRecorderMeasureAddValueDouble(
  struct RunRecorderMeasure* const that,
                 char const* const metric,
                      double const val);

// Add a measure to a project
// Inputs:
//         that: the struct RunRecorder
//      project: the project to add the measure to
//      measure: the measure to add
void RunRecorderAddMeasure(
               struct RunRecorder* const that,
                       char const* const project,
  struct RunRecorderMeasure const* const measure);

// Delete a measure
// Inputs:
//          that: the struct RunRecorder
//    refMeasure: the reference of the measure to delete
void RunRecorderDeleteMeasure(
  struct RunRecorder* const that,
                 long const refMeasure);

// Get the measures of a project
// Inputs:
//         that: the struct RunRecorder
//      project: the project's name
// Output:
//   Return the measures as a new struct RunRecorderMeasures
struct RunRecorderMeasures* RunRecorderGetMeasures(
  struct RunRecorder* const that,
          char const* const project);

// Get the most recent measures of a project
// Inputs:
//        that: the struct RunRecorder
//     project: the project's name
//   nbMeasure: the number of measures to be returned
// Output:
//   Return the measures as a new struct RunRecorderMeasures, ordered
//   from the most recent to the oldest
struct RunRecorderMeasures* RunRecorderGetLastMeasures(
  struct RunRecorder* const that,
          char const* const project,
                 long const nbMeasure);

// Free a struct RunRecorderMeasures
// Input:
//   that: the struct RunRecorderMeasures
void RunRecorderMeasuresFree(
  struct RunRecorderMeasures** that);

// Print a struct RunRecorderMeasures on a stream in CSV format as:
// Metric1&Metric2&...
// Value1_1&Value1_2&...
// Value2_1&Value2_2&...
// ...
// Inputs:
//     that: the struct RunRecorderMeasures
//   stream: the stream to write on
void RunRecorderMeasuresPrintCSV(
  struct RunRecorderMeasures const* const that,
                              FILE* const stream);

// Remove a project
// Inputs:
//         that: the struct RunRecorder
//      project: the project's name
void RunRecorderFlushProject(
  struct RunRecorder* const that,
          char const* const project);

// Free a struct RunRecorderPairsRefVal
// Input:
//   that: the struct RunRecorderPairsRefVal
void RunRecorderPairsRefValFree(
  struct RunRecorderPairsRefVal** const that);

// Free a struct RunRecorderPairsRefValDef
// Input:
//   that: the struct RunRecorderPairsRefValDef
void RunRecorderPairsRefValDefFree(
  struct RunRecorderPairsRefValDef** const that);

// ================== Macros =========================

// Polymorphic RunRecorderMeasureAddValue
#define RunRecorderMeasureAddValue(T, M, V) _Generic(V, \
  char*: RunRecorderMeasureAddValueStr, \
  char const*: RunRecorderMeasureAddValueStr, \
  int: RunRecorderMeasureAddValueInt, \
  unsigned int: RunRecorderMeasureAddValueInt, \
  long: RunRecorderMeasureAddValueInt, \
  float: RunRecorderMeasureAddValueDouble, \
  double: RunRecorderMeasureAddValueDouble)(T, M, V)

// End of the guard against multiple inclusion
#endif

// ------------------ runrecorder.h ------------------
