// ------------------ runrecorder.c ------------------

// Include the header
#include "runrecorder.h"

// ================== Macros =========================

// Last version of the database
#define VERSION_DB "01.00.00"

// Number of tables in the database
#define NB_TABLE 5

// Default CSV separator
#define CSV_SEP '&'

// Loop from 0 to n
#define ForZeroTo(I, N) for (long I = 0; I < N; ++I)

// Polymorphic free
#define PolyFree(P) _Generic(P, \
  struct RunRecorder**: RunRecorderFree, \
  struct RunRecorderPairsRefVal**: RunRecorderPairsRefValFree, \
  struct RunRecorderMeasure**: RunRecorderMeasureFree, \
  struct RunRecorderMeasures**: RunRecorderMeasuresFree, \
  default: free)((void*)P)

// strdup freeing the assigned variable and raising exception if it fails
#define SafeStrDup(T, S)  \
  do { \
    PolyFree(T); T = strdup(S); \
    if (T == NULL) Raise(TryCatchExc_MallocFailed); \
  } while(false)

// malloc freeing the assigned variable and raising exception if it fails
#define SafeMalloc(T, S)  \
  do { \
    PolyFree(T); T = malloc(S); \
    if (T == NULL) Raise(TryCatchExc_MallocFailed); \
  } while(false)

// realloc raising exception and leaving the original pointer unchanged
// if it fails
#define SafeRealloc(T, S)  \
  do { \
    void* ptr = realloc(T, S); \
    if (ptr == NULL) Raise(TryCatchExc_MallocFailed); else T = ptr; \
  } while(false)

// sprintf at the end of a string
#define SPrintfAtEnd(S, F, ...) \
  do { \
    char* ptrEnd = strchr(S, '\0'); \
    sprintf(ptrEnd, F, __VA_ARGS__); \
  } while(false)

// Number of exceptions in RunRecorderException
#define NbExceptions RunRecorderExc_LastID - RunRecorderExc_CreateTableFailed

// ================== Static variables =========================

// Labels for the conversion of RunRecorderException to char*
static char* exceptionStr[NbExceptions] = {

  "RunRecorderExc_CreateTableFailed",
  "RunRecorderExc_OpenDbFailed",
  "RunRecorderExc_CreateCurlFailed",
  "RunRecorderExc_CurlRequestFailed",
  "RunRecorderExc_CurlSetOptFailed",
  "RunRecorderExc_SQLRequestFailed",
  "RunRecorderExc_ApiRequestFailed",
  "RunRecorderExc_InvalidProjectName",
  "RunRecorderExc_ProjectNameAlreadyUsed",
  "RunRecorderExc_FlushProjectFailed",
  "RunRecorderExc_AddProjectFailed",
  "RunRecorderExc_AddMetricFailed",
  "RunRecorderExc_UpdateViewFailed",
  "RunRecorderExc_InvalidJSON",
  "RunRecorderExc_InvalidMetricName",
  "RunRecorderExc_InvalidValue",
  "RunRecorderExc_MetricNameAlreadyUsed",
  "RunRecorderExc_AddMeasureFailed",
  "RunRecorderExc_DeleteMeasureFailed",

};

// ================== Private functions declaration =========================

// Init a struct RunRecorder using a local SQLite database
// Input:
//   that: the struct RunRecorder
// Raise:
//   RunRecorderExc_OpenDbFailed
static void InitLocal(
  struct RunRecorder* const that);

// Init a struct RunRecorder using the Web API
// Input:
//   that: the struct RunRecorder
// Raise:
//   RunRecorderExc_CreateCurlFailed
//   RunRecorderExc_CurlSetOptFailed
static void InitWebAPI(
  struct RunRecorder* const that);

// Free the error messages of a struct RunRecorder
// Input:
//   that: The struct RunRecorder to be freed
static void FreeErrMsg(
  struct RunRecorder* const that);

// Check if a struct RunRecorder uses a local SQLite database or the 
// Web API
// Input:
//   that: The struct RunRecorder
// Return:
//   Return true if the struct RunRecorder uses the Web API, else false
static bool UsesAPI(
  struct RunRecorder const* const that);

// Reset the curl reply, to be called before sending a new curl request
// or the result of the new request will be appended to the eventual
// one of the previous request
// Input:
//   The struct RunRecorder
static void ResetCurlReply(
  struct RunRecorder* const that);

// Create the RunRecorder's tables in an empty database
// Input:
//   that: The struct RunRecorder
static void CreateDb(
  struct RunRecorder* const that);

// Create the RunRecorder's tables in an empty local database
// Input:
//   that: The struct RunRecorder
// Raise:
//   RunRecorderExc_CreateTableFailed
static void CreateDbLocal(
  struct RunRecorder* const that);

// Upgrade the database
// Input:
//   that: The struct RunRecorder
static void UpgradeDb(
  struct RunRecorder* const that);

// Callback for RunRecorderGetVersionLocal
// Input:
//      data: The version
//     nbCol: Number of columns in the returned rows
//    colVal: Row values
//   colName: Columns name
// Output:
//   Return 0 if successfull, else 1
static int GetVersionLocalCb(
   void* data,
     int nbCol,
  char** colVal,
  char** colName);

// Get the version of the local database
// Input:
//   that: The struct RunRecorder
// Output:
//   Return a new string
// Raise:
//   RunRecorderExc_SQLRequestFailed
static char* GetVersionLocal(
  struct RunRecorder* const that);

// Callback to memorise the incoming data from the Web API
// Input:
//   data: incoming data
//   size: always 1
//   nmemb: number of incoming byte
//   reply: pointer to where to store the incoming data
// Output:
//   Return the number of received byte
static size_t GetReplyAPI(
   char* data,
  size_t size,
  size_t nmemb,
   void* ptr);

// Get the value of a key in a JSON encoded string
// Input:
//   json: the json encoded string
//    key: the key
// Output:
//   Return a new string, or NULL if the key doesn't exist
// Raise:
//   RunRecorderExc_ApiRequestFailed
static char* GetJSONValOfKey(
  char const* const json,
  char const* const key);

// Set the POST data in the Web API request of a struct RunRecorder
// Input:
//   that: the struct RunRecorder
//   data: the POST data
// Raise:
//   RunRecorderExc_CurlSetOptFailed
static void SetAPIReqPostVal(
  struct RunRecorder* const that,
          char const* const data);

// Get the ret code in the current JSON reply of a struct RunRecorder
// Input:
//   that: the struct RunRecorder
// Output:
//   Return a new string
// Raise:
//   RunRecorderExc_ApiRequestFailed
static char* GetAPIRetCode(
  struct RunRecorder* const that);

// Send the current request of a struct RunRecorder
// Input:
//        that: the struct RunRecorder
//   isJsonReq: flag to indicate that the request returns JSON encoded
//              data
// Raise:
//   RunRecorderExc_CurlRequestFailed
//   RunRecorderExc_ApiRequestFailed
static void SendAPIReq(
  struct RunRecorder* const that,
                 bool const isJsonReq);

// Get the version of the database via the Web API
// Input:
//   The struct RunRecorder
// Output:
//   Return a new string
static char* GetVersionAPI(
  struct RunRecorder* const that);

// Add a new project in a local database
// Input:
//   that: the struct RunRecorder
//   name: the name of the new project, it must respect the following
//         pattern: /^[a-zA-Z][a-zA-Z0-9_]*$/
// Raise:
//   RunRecorderExc_InvalidProjectName
//   RunRecorderExc_ProjectNameAlreadyUsed
static void AddProjectLocal(
  struct RunRecorder* const that,
          char const* const name);

// Add a new project through the Web API
// Input:
//   that: the struct RunRecorder
//   name: the name of the new project, it must respect the following
//         pattern: /^[a-zA-Z][a-zA-Z0-9_]*$/
// Raise:
//   RunRecorderExc_InvalidProjectName
//   RunRecorderExc_ProjectNameAlreadyUsed
static void AddProjectAPI(
  struct RunRecorder* const that,
          char const* const name);

// Check if a value is present in pairs ref/val
// Inputs:
//   that: the struct RunRecorderPairsRefVal
//    val: the value to check
// Output:
//   Return true if the value is present in the pairs, else false
static bool PairsRefValContainsVal(
  struct RunRecorderPairsRefVal const* const that,
                           char const* const val);

// Add a new pair in a struct RunRecorderPairsRefVal
// Inputs:
//   pairs: the struct RunRecorderPairsRefVal
//     ref: the reference of the pair
//     val: the val of the pair
static void PairsRefValAdd(
  struct RunRecorderPairsRefVal* const pairs,
                            long const ref,
                     char const* const val);

// Callback to receive pairs of ref/value from sqlite3
// Input:
//      data: The pairs
//     nbCol: Number of columns in the returned rows
//    colVal: Row values, the reference is expected to be in the first
//            column and the value in the second column
//   colName: Columns name
// Output:
//   Return 0 if successful, else 1
static int GetPairsLocalCb(
   void* data,
     int nbCol,
  char** colVal,
  char** colName);

// Get the list of projects in the local database
// Input:
//   that: the struct RunRecorder
// Output:
//   Return the projects' reference/label
// Raise:
//   RunRecorderExc_SQLRequestFailed
static struct RunRecorderPairsRefVal* GetProjectsLocal(
  struct RunRecorder* const that);

// Extract a struct RunRecorderPairsRefVal from a JSON string
// Input:
//   json: the JSON string, expected to be formatted as "1":"A","2":"B",...
// Output:
//   Return a new struct RunRecorderPairsRefVal
// Raise:
//   RunRecorderExc_InvalidJSON
static struct RunRecorderPairsRefVal* GetPairsRefValFromJSON(
  char const* const json);

// Get the list of projects through the Web API
// Input:
//   that: the struct RunRecorder
// Output:
//   Return the projects' reference/label
static struct RunRecorderPairsRefVal* GetProjectsAPI(
  struct RunRecorder* const that);

// Create a struct RunRecorderPairsRefVal
// Output:
//   Return the new struct RunRecorderPairsRefVal
static struct RunRecorderPairsRefVal* RunRecorderPairsRefValCreate(
  void);

// Get the list of metrics for a project from a local database
// Input:
//      that: the struct RunRecorder
//   project: the project
// Output:
//   Return the metrics' reference/label
// Raise:
//   RunRecorderExc_SQLRequestFailed
static struct RunRecorderPairsRefVal* GetMetricsLocal(
  struct RunRecorder* const that,
          char const* const project);

// Get the list of metrics for a project through the Web API
// Input:
//      that: the struct RunRecorder
//   project: the project
// Output:
//   Return the metrics' reference/label
// Raise:
//   RunRecorderExc_ApiRequestFailed
static struct RunRecorderPairsRefVal* GetMetricsAPI(
  struct RunRecorder* const that,
          char const* const project);

// Update the view for a project
// Input:
//         that: the struct RunRecorder
//      project: the name of the project
// Raise:
//   RunRecorderExc_UpdateViewFailed
static void UpdateViewProject(
  struct RunRecorder* const that,
          char const* const project);

// Add a metric to a project to a local database
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
//   RunRecorderExc_InvalidMetricName
//   RunRecorderExc_MetricNameAlreadyUsed
static void AddMetricLocal(
  struct RunRecorder* const that,
          char const* const project,
          char const* const label,
          char const* const defaultVal);

// Add a metric to a project through the Web API
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
//   RunRecorderExc_InvalidMetricName
//   RunRecorderExc_MetricNameAlreadyUsed
static void AddMetricAPI(
  struct RunRecorder* const that,
          char const* const project,
          char const* const label,
          char const* const defaultVal);

// Add a measure to a project in a local database
// Inputs:
//         that: the struct RunRecorder
//      project: the project to add the measure to
//      measure: the measure to add
// Raise:
//   RunRecorderExc_AddMeasureFailed
static void AddMeasureLocal(
               struct RunRecorder* const that,
                       char const* const project,
  struct RunRecorderMeasure const* const measure);

// Add a measure to a project through the WebAPI
// Inputs:
//         that: the struct RunRecorder
//      project: the project to add the measure to
//      measure: the measure to add
// Raise:
//   RunRecorderExc_ApiRequestFailed
static void AddMeasureAPI(
               struct RunRecorder* const that,
                       char const* const project,
  struct RunRecorderMeasure const* const measure);

// Delete a measure in a local database
// Inputs:
//          that: the struct RunRecorder
//    refMeasure: the reference of the measure to delete
// Raise:
//   RunRecorderExc_DeleteMeasureFailed
static void DeleteMeasureLocal(
  struct RunRecorder* const that,
                 long const refMeasure);

// Delete a measure through the Web API
// Inputs:
//          that: the struct RunRecorder
//    refMeasure: the reference of the measure to delete
static void DeleteMeasureAPI(
  struct RunRecorder* const that,
                 long const refMeasure);

// Callback to receive the measures from sqlite3
// Input:
//      data: The measures
//     nbCol: Number of columns in the returned rows
//    colVal: Row values
//   colName: Columns name
// Output:
//   Return 0 if successfull, else 1
static int GetMeasuresLocalCb(
   void* data,
     int nbCol,
  char** colVal,
  char** colName);

// Helper function to commonalize code between GetMeasures and
// GetLastMeasures
// Inputs:
//        that: the struct RunRecorder
//     project: the project's name
//   nbMeasure: the number of measures returned, if 0 all measures are
//              returned
// Output:
//   Set the SQL command in that->cmd to get measures as a struct
//    RunRecorderMeasures
static void SetCmdToGetMeasuresLocal(
  struct RunRecorder* const that,
          char const* const project,
                 long const nbMeasure);

// Get the measures of a project from a local database
// Inputs:
//         that: the struct RunRecorder
//      project: the project's name
// Output:
//   Return the measures as a struct RunRecorderMeasures
// Raise:
//   RunRecorderExc_SQLRequestFailed
static struct RunRecorderMeasures* GetMeasuresLocal(
  struct RunRecorder* const that,
          char const* const project);

// Helper function to commonalize code in RunRecorderCSVToData
// Inputs
//   csv: Pointer to the start of the row in CSV data
//   tgt: The array of char* where to copy the values
//   sep: The separator character
// Output:
//   Return a pointer to the end of the row
static char const* SplitCSVRowToData(
   char const* const csv,
        char** const tgt,
          char const sep);

// Convert CSV data to a new struct RunRecorderMeasures. The CSV data are
// expected to be formatted as:
// Ref&Metric1&Metric2&...
// Ref1&Value1_1&Value1_2&...
// Ref2&Value2_1&Value2_2&...
// ...
// Inputs:
//   csv: the CSV data
//   sep: the separator between columns ('&' in the example above)
// Output:
//   Return a newly allocated RunRecorderMeasures
static struct RunRecorderMeasures* CSVToData(
  char const* const csv,
         char const sep);

// Get the measures of a project through the Web API
// Inputs:
//         that: the struct RunRecorder
//      project: the project's name
// Output:
//   Return the measures as a struct RunRecorderMeasures
static struct RunRecorderMeasures* GetMeasuresAPI(
  struct RunRecorder* const that,
          char const* const project);

// Get the most recent measures of a project from a local database
// Inputs:
//        that: the struct RunRecorder
//     project: the project's name
//   nbMeasure: the number of measures to be returned
// Output:
//   Return the measures as a struct RunRecorderMeasures, ordered from the
//   most recent to the oldest
// Raise:
//   RunRecorderExc_SQLRequestFailed
static struct RunRecorderMeasures* GetLastMeasuresLocal(
  struct RunRecorder* const that,
          char const* const project,
                 long const nbMeasure);

// Get the most recent measures of a project through the Web API
// Inputs:
//        that: the struct RunRecorder
//     project: the project's name
//   nbMeasure: the number of measures to be returned
// Output:
//   Return the measures as a struct RunRecorderMeasures, ordered from the
//   most recent to the oldest
static struct RunRecorderMeasures* GetLastMeasuresAPI(
  struct RunRecorder* const that,
          char const* const project,
                 long const nbMeasure);

// Create a struct RunRecorderMeasures
// Output:
//   Return the dynamically allocated struct RunRecorderMeasures
static struct RunRecorderMeasures* RunRecorderMeasuresCreate(
  void);

// Remove a project from a local database
// Inputs:
//         that: the struct RunRecorder
//      project: the project's name
// Raise:
//   RunRecorderExc_FlushProjectFailed
static void FlushProjectLocal(
  struct RunRecorder* const that,
          char const* const project);

// Remove a project through the Web API
// Inputs:
//         that: the struct RunRecorder
//      project: the project's name
static void FlushProjectAPI(
  struct RunRecorder* const that,
          char const* const project);

// Function to convert a RunRecorder exception ID to char*
// Input:
//   exc: the exception ID
// Output:
//   Return a pointer to a static string describing the exception, or
//   NULL if the ID is not one of RunRecorderException
static char const* ExcToStr(
  int exc);

// ================== Public functions definition =========================

// Constructor for a struct RunRecorder
// Input:
//   url: Path to the SQLite database or Web API
// Output:
//  Return a new struct RunRecorder
struct RunRecorder* RunRecorderCreate(
  char const* const url) {

  // Allocate the struct RunRecorder
  struct RunRecorder* that = NULL;
  SafeMalloc(
    that,
    sizeof(struct RunRecorder));

  // Initialise the properties
  that->errMsg = NULL;
  that->db = NULL;
  that->url = NULL;
  that->curl = NULL;
  that->curlReply = NULL;
  that->cmd = NULL;
  that->sqliteErrMsg = NULL;
  that->refLastAddedMeasure = 0;

  // Copy the url
  SafeStrDup(
    that->url,
    url);

  // Return the struct RunRecorder
  return that;

}

// Initialise a struct RunRecorder
// Input:
//   that: The struct RunRecorder
void RunRecorderInit(
  struct RunRecorder* const that) {

  // Set the conversion function of RunRecorderException to char* to
  // have their name displayed instead of their id and help detecting
  // ID collision
  TryCatchAddExcToStrFun(ExcToStr);

  // Ensure the error messages are freed to avoid confusion with
  // eventual previous messages
  FreeErrMsg(that);

  // If the recorder doesn't use the API
  if (UsesAPI(that) == false) {

    // Init the local connection to the database
    InitLocal(that);

  // Else, the recorder uses the API
  } else {

    // Init the Web API
    InitWebAPI(that);

  }

  // If the version of the database is different from the last version
  // upgrade the database
  char* version = RunRecorderGetVersion(that);
  int cmpVersion =
    strcmp(
      version,
      VERSION_DB);
  PolyFree(version);
  if (cmpVersion != 0) UpgradeDb(that);

}

// Free memory used by a struct RunRecorder
// Input:
//   that: The struct RunRecorder to be freed
void RunRecorderFree(
  struct RunRecorder** const that) {

  // If it's already freed, nothing to do
  if (that == NULL || *that == NULL) return;

  // Free memory used by the properties
  PolyFree((*that)->url);
  PolyFree((*that)->errMsg);
  sqlite3_free((*that)->sqliteErrMsg);
  PolyFree((*that)->curlReply);
  PolyFree((*that)->cmd);

  // Close the connection to the local database if it was opened
  if ((*that)->db != NULL) sqlite3_close((*that)->db);

  // Clean up the curl instance if it was created
  if ((*that)->curl != NULL) {

    curl_easy_cleanup((*that)->curl);
    curl_global_cleanup();

  }

  // Free memory used by the RunRecorder
  PolyFree(*that);
  *that = NULL;

}

// Get the version of the database
// Input:
//   that: the struct RunRecorder
// Output:
//   Return the version as a new string
char* RunRecorderGetVersion(
  struct RunRecorder* const that) {

  // Ensure the error messages are freed to avoid confusion with
  // eventual previous messages
  FreeErrMsg(that);

  // If the RunRecorder uses a local database
  if (UsesAPI(that) == false) {

    return GetVersionLocal(that);

  // Else, the RunRecorder uses the Web API
  } else {

    return GetVersionAPI(that);

  }

}

// Check if a string respects the pattern /^[a-zA-Z][a-zA-Z0-9_]$*/
// Input:
//   str: the string to check
// Output:
//   Return true if the string respects the pattern, else false
bool RunRecorderIsValidLabel(
  char const* const str) {

  // Check the first character
  if (*str < 'a' && *str > 'z' &&
      *str < 'A' && *str > 'Z') return false;

  // Loop on the other characters
  char const* ptr = str + 1;
  while (*ptr != '\0') {

    // Check the character
    if (*ptr < 'a' && *ptr > 'z' &&
        *ptr < 'A' && *ptr > 'Z' &&
        *ptr != '_') return false;

    // Move to the next character
    ++ptr;

  }

  // If we reach here the string is valid
  return true;

}

// Check if a string respects the pattern /^[^"=&]+$*/
// Input:
//   str: the string to check
// Output:
//   Return true if the string respects the pattern, else false
bool RunRecorderIsValidValue(
  char const* const str) {

  // Check if the string contains at least one character
  if (*str == '\0') return false;

  // Loop on the characters
  char const* ptr = str;
  while (*ptr != '\0') {

    // Check the character
    if (*ptr == '"' || *ptr == '=' ||
        *ptr == '&') return false;

    // Move to the next character
    ++ptr;

  }

  // If we reach here the string is valid
  return true;

}

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
          char const* const name) {

  // Ensure the error messages are freed to avoid confusion with
  // eventual previous messages
  FreeErrMsg(that);

  // Check the name
  bool isValidName = RunRecorderIsValidLabel(name);
  if (isValidName == false) Raise(RunRecorderExc_InvalidProjectName);

  // Check if there is no other project with same name
  struct RunRecorderPairsRefVal* projects = RunRecorderGetProjects(that);
  bool alreadyUsed =
    PairsRefValContainsVal(
      projects,
      name);
  RunRecorderPairsRefValFree(&projects);
  if (alreadyUsed == true) Raise(RunRecorderExc_ProjectNameAlreadyUsed);

  // If the RunRecorder uses a local database
  if (UsesAPI(that) == false) {

    AddProjectLocal(
      that,
      name);

  // Else, the RunRecorder uses the Web API
  } else {

    AddProjectAPI(
      that,
      name);

  }

}

// Get the list of projects
// Input:
//   that: the struct RunRecorder
// Output:
//   Return the projects' reference/label as a new struct
//   RunRecorderPairsRefVal
struct RunRecorderPairsRefVal* RunRecorderGetProjects(
  struct RunRecorder* const that) {

  // Ensure the error messages are freed to avoid confusion with
  // eventual previous messages
  FreeErrMsg(that);

  // If the RunRecorder uses a local database
  if (UsesAPI(that) == false) {

    return GetProjectsLocal(that);

  // Else, the RunRecorder uses the Web API
  } else {

    return GetProjectsAPI(that);

  }

}

// Get the list of metrics for a project
// Input:
//      that: the struct RunRecorder
//   project: the project
// Output:
//   Return the metrics' reference/label as a new struct
//   RunRecorderPairsRefVal
struct RunRecorderPairsRefVal* RunRecorderGetMetrics(
  struct RunRecorder* const that,
          char const* const project) {

  // Ensure the error messages are freed to avoid confusion with
  // eventual previous messages
  FreeErrMsg(that);

  // If the RunRecorder uses a local database
  if (UsesAPI(that) == false) {

    return GetMetricsLocal(
      that,
      project);

  // Else, the RunRecorder uses the Web API
  } else {

    return GetMetricsAPI(
      that,
      project);

  }

}

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
//   RunRecorderExc_InvalidMetricName
//   RunRecorderExc_MetricNameAlreadyUsed
void RunRecorderAddMetric(
  struct RunRecorder* const that,
          char const* const project,
          char const* const label,
          char const* const defaultVal) {

  // Ensure the error messages are freed to avoid confusion with
  // eventual previous messages
  FreeErrMsg(that);

  // Check the label
  bool isValidLabel = RunRecorderIsValidLabel(label);
  if (isValidLabel == false) Raise(RunRecorderExc_InvalidMetricName);

  // Check if the label is one of the reserved keywords
  int retCmp =
    strcmp(
      label,
      "action");
  if (retCmp == 0) Raise(RunRecorderExc_InvalidMetricName);
  retCmp =
    strcmp(
      label,
      "project");
  if (retCmp == 0) Raise(RunRecorderExc_InvalidMetricName);

  // Check if there is no other metric with same label for this project
  struct RunRecorderPairsRefVal* metrics =
    RunRecorderGetMetrics(
      that,
      project);
  bool alreadyUsed =
    PairsRefValContainsVal(
      metrics,
      label);
  RunRecorderPairsRefValFree(&metrics);
  if (alreadyUsed == true) Raise(RunRecorderExc_MetricNameAlreadyUsed);

  // If the RunRecorder uses a local database
  if (UsesAPI(that) == false) {

    return
      AddMetricLocal(
        that,
        project,
        label,
        defaultVal);

  // Else, the RunRecorder uses the Web API
  } else {

    return
      AddMetricAPI(
        that,
        project,
        label,
        defaultVal);

  }

}

// Create a new struct RunRecorderMeasure
// Output:
//   Return the new struct RunRecorderMeasure
struct RunRecorderMeasure* RunRecorderMeasureCreate(
  void) {

  // Allocate the new struct RunRecorderMeasure
  struct RunRecorderMeasure* measure = NULL;
  SafeMalloc(
    measure,
    sizeof(struct RunRecorderMeasure));

  // Initialise properties
  measure->nbMetric = 0;
  measure->metrics = NULL;
  measure->values = NULL;

  // Return the new struct RunRecorderMeasure
  return measure;

}

// Free a struct RunRecorderMeasure
// Input:
//   that: the struct RunRecorderMeasure
void RunRecorderMeasureFree(
  struct RunRecorderMeasure** const that) {

  // If it's already freed, nothing to do
  if (that == NULL || *that == NULL) return;

  // If there was metrics
  if ((*that)->metrics != NULL) {

    // Free the metrics
    ForZeroTo(iMetric, (*that)->nbMetric) PolyFree((*that)->metrics[iMetric]);

  }

  // If there was values
  if ((*that)->values != NULL) {

    // Free the values
    ForZeroTo(iVal, (*that)->nbMetric) PolyFree((*that)->values[iVal]);

  }

  // Free memory
  PolyFree((*that)->metrics);
  PolyFree((*that)->values);
  PolyFree(*that);
  *that = NULL;

}

// Add a string value to a struct RunRecorderMeasure
// Input:
//     that: the struct RunRecorderMeasure
//   metric: the value's metric
//      val: the value
// Raise
//   RunRecorderExc_InvalidValue
void RunRecorderMeasureAddValueStr(
  struct RunRecorderMeasure* const that,
                 char const* const metric,
                 char const* const val) {

  // If the value is valid
  if (RunRecorderIsValidValue(val) == false) Raise(RunRecorderExc_InvalidValue);

  // Reallocate memory for the added value and its metric
  SafeRealloc(
    that->metrics,
    sizeof(char*) * (that->nbMetric + 1));
  SafeRealloc(
    that->values,
    sizeof(char*) * (that->nbMetric + 1));
  that->metrics[that->nbMetric] = NULL;
  that->values[that->nbMetric] = NULL;

  // Update the number of metric in the measure
  ++(that->nbMetric);

  // Copy the value and its metric in the measure
  SafeStrDup(
    that->metrics[that->nbMetric - 1],
    metric);
  SafeStrDup(
    that->values[that->nbMetric - 1],
    val);

}

// Add an int value to a struct RunRecorderMeasure
// Input:
//     that: the struct RunRecorderMeasure
//   metric: the value's metric
//      val: the value
void RunRecorderMeasureAddValueInt(
  struct RunRecorderMeasure* const that,
                 char const* const metric,
                        long const val) {

  // Convert the value to a string
  int lenStr =
    snprintf(
      NULL,
      0,
      "%ld",
      val);
  char* str = NULL;
  SafeMalloc(
    str,
    lenStr + 1);
  sprintf(
    str,
    "%ld",
    val); 

  // Add the value
  Try {

    RunRecorderMeasureAddValueStr(
      that,
      metric,
      str);

    // Free memory
    PolyFree(str);

  } CatchDefault {

    PolyFree(str);
    Raise(TryCatchGetLastExc());

  } EndTryWithDefault;

}

// Add a double value to a struct RunRecorderMeasure
// Input:
//     that: the struct RunRecorderMeasure
//   metric: the value's metric
//      val: the value
void RunRecorderMeasureAddValueDouble(
  struct RunRecorderMeasure* const that,
                 char const* const metric,
                      double const val) {

  // Convert the value to a string
  int lenStr =
    snprintf(
      NULL,
      0,
      "%lf",
      val);
  char* str = NULL;
  SafeMalloc(
    str,
    lenStr + 1);
  sprintf(
    str,
    "%lf",
    val); 

  // Add the value
  Try {

    RunRecorderMeasureAddValueStr(
      that,
      metric,
      str);

    // Free memory
    PolyFree(str);

  } CatchDefault {

    PolyFree(str);
    Raise(TryCatchGetLastExc());

  } EndTryWithDefault;

}

// Add a measure to a project
// Inputs:
//         that: the struct RunRecorder
//      project: the project to add the measure to
//      measure: the measure to add
void RunRecorderAddMeasure(
               struct RunRecorder* const that,
                       char const* const project,
  struct RunRecorderMeasure const* const measure) {

  // Reset the reference of the last added measure
  that->refLastAddedMeasure = 0;

  // Ensure the error messages are freed to avoid confusion with
  // eventual previous messages
  FreeErrMsg(that);

  // If the RunRecorder uses a local database
  if (UsesAPI(that) == false) {

    AddMeasureLocal(
      that,
      project,
      measure);

  // Else, the RunRecorder uses the Web API
  } else {

    AddMeasureAPI(
      that,
      project,
      measure);

  }

}

// Delete a measure
// Inputs:
//          that: the struct RunRecorder
//    refMeasure: the reference of the measure to delete
void RunRecorderDeleteMeasure(
  struct RunRecorder* const that,
                 long const refMeasure) {

  // Ensure the error messages are freed to avoid confusion with
  // eventual previous messages
  FreeErrMsg(that);

  // If the RunRecorder uses a local database
  if (UsesAPI(that) == false) {

    DeleteMeasureLocal(
      that,
      refMeasure);

  // Else, the RunRecorder uses the Web API
  } else {

    DeleteMeasureAPI(
      that,
      refMeasure);

  }

}

// Get the measures of a project
// Inputs:
//         that: the struct RunRecorder
//      project: the project's name
// Output:
//   Return the measures as a new struct RunRecorderMeasures
struct RunRecorderMeasures* RunRecorderGetMeasures(
  struct RunRecorder* const that,
          char const* const project) {

  // Ensure the error messages are freed to avoid confusion with
  // eventual previous messages
  FreeErrMsg(that);

  // If the RunRecorder uses a local database
  if (UsesAPI(that) == false) {

    return
      GetMeasuresLocal(
        that,
        project);

  // Else, the RunRecorder uses the Web API
  } else {

    return
      GetMeasuresAPI(
        that,
        project);

  }

}

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
                 long const nbMeasure) {

  // Ensure the error messages are freed to avoid confusion with
  // eventual previous messages
  FreeErrMsg(that);

  // If the RunRecorder uses a local database
  if (UsesAPI(that) == false) {

    return
      GetLastMeasuresLocal(
        that,
        project,
        nbMeasure);

  // Else, the RunRecorder uses the Web API
  } else {

    return
      GetLastMeasuresAPI(
        that,
        project,
        nbMeasure);

  }

}

// Free a struct RunRecorderMeasures
// Input:
//   that: the struct RunRecorderMeasures
void RunRecorderMeasuresFree(
  struct RunRecorderMeasures** that) {

  // If it's already freed, nothing to do
  if (that == NULL || *that == NULL) return;

  // If there was metrics
  if ((*that)->metrics != NULL) {

    // Free the metrics label
    ForZeroTo(iMetric, (*that)->nbMetric) PolyFree((*that)->metrics[iMetric]);

  }

  // If there was measures
  if ((*that)->values != NULL) {

    // Loop on the measures
    ForZeroTo(iMeasure, (*that)->nbMeasure) {

      // If there was values for the measure
      if ((*that)->values[iMeasure] != NULL) {

        // Free the values
        ForZeroTo(iMetric, (*that)->nbMetric)
          PolyFree((*that)->values[iMeasure][iMetric]);

        // Free the measure
        PolyFree((*that)->values[iMeasure]);

      }

    }

  }

  // Free memory
  PolyFree((*that)->metrics);
  PolyFree((*that)->values);
  PolyFree(*that);
  *that = NULL;

}

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
                          FILE* const stream) {

  // If there are metrics
  if (that->metrics != NULL) {

    // Declare a variable to memorise the separator after the column
    char sep = CSV_SEP;

    // Loop on the metrics
    ForZeroTo(iMetric, that->nbMetric) {

      // If it's the last column, change the separator to line return
      if (iMetric == that->nbMetric - 1) sep = '\n';

      // Print the column value and its separator
      fprintf(
        stream,
        "%s%c",
        that->metrics[iMetric],
        sep);

    }

  }

  // If there are measures
  if (that->values != NULL) {

    // Loop on the measures
    ForZeroTo(iMeasure, that->nbMeasure) {

      // Declare a variable to memorise the separator after the column
      char sep = CSV_SEP;

      // Loop on the metrics
      ForZeroTo(iMetric, that->nbMetric) {

        // If it's the last column, change the separator to line return
        if (iMetric == that->nbMetric - 1) sep = '\n';

        // Print the column value and its separator
        fprintf(
          stream,
          "%s%c",
          that->values[iMeasure][iMetric],
          sep);

      }

    }

  }

}

// Remove a project
// Inputs:
//         that: the struct RunRecorder
//      project: the project's name
void RunRecorderFlushProject(
  struct RunRecorder* const that,
          char const* const project) {

  // Ensure the error messages are freed to avoid confusion with
  // eventual previous messages
  FreeErrMsg(that);

  // If the RunRecorder uses a local database
  if (UsesAPI(that) == false) {

    FlushProjectLocal(
      that,
      project);

  // Else, the RunRecorder uses the Web API
  } else {

    FlushProjectAPI(
      that,
      project);

  }

}

// Free a struct RunRecorderPairsRefVal
// Input:
//   that: the struct RunRecorderPairsRefVal
void RunRecorderPairsRefValFree(
  struct RunRecorderPairsRefVal** const that) {

  // If it's already freed, nothing to do
  if (that == NULL || *that == NULL) return;

  // If there values
  if ((*that)->values != NULL) {

    // Free the values
    ForZeroTo(iPair, (*that)->nb) PolyFree((*that)->values[iPair]);

  }

  // Free memory
  PolyFree((*that)->values);
  PolyFree((*that)->refs);
  PolyFree(*that);
  *that = NULL;

}

// ================== Private functions definition =========================

// Init a struct RunRecorder using a local SQLite database
// Input:
//   that: the struct RunRecorder
// Raise:
//   RunRecorderExc_OpenDbFailed
static void InitLocal(
  struct RunRecorder* const that) {

  // Try to open the file in reading mode to check if it exists
  FILE* fp =
    fopen(
      that->url,
      "r");

  // Open the connection to the local database, if the database doesn't
  // exist an empty one will be created automatically by SQLite3
  int ret =
    sqlite3_open(
      that->url,
      &(that->db));

  // If the database couldn't be opened/created
  if (ret != SQLITE_OK) {

    // If it could be opend through fopen, close it
    if (fp != NULL) fclose(fp);

    // Copy the error message and raise an exception
    SafeStrDup(
      that->errMsg,
      sqlite3_errmsg(that->db));
    Raise(RunRecorderExc_OpenDbFailed);

  // Else, the database could be opened/created
  } else {

    // If we couldn't open it with fopen before opening it with
    // sqlite3_open it means the database has just been created by
    // SQLite3
    if (fp == NULL) {

      // Create the RunRecorder's tables in the database
      CreateDb(that);

    // Else, the database could be opened and is ready to use by RunRecorder
    } else {

      // Close the FILE*
      fclose(fp);

    }

  }

}

// Init a struct RunRecorder using the Web API
// Input:
//   that: the struct RunRecorder
// Raise:
//   RunRecorderExc_CreateCurlFailed
//   RunRecorderExc_CurlSetOptFailed
static void InitWebAPI(
  struct RunRecorder* const that) {

  // Create the Curl instance
  curl_global_init(CURL_GLOBAL_ALL);
  that->curl = curl_easy_init();
  if (that->curl == NULL) {

    curl_global_cleanup();
    Raise(RunRecorderExc_CreateCurlFailed);

  }

  // Set the url of the Web API in the Curl instance
  CURLcode res =
    curl_easy_setopt(
      that->curl,
      CURLOPT_URL,
      that->url);
  if (res != CURLE_OK) {

    curl_easy_cleanup(that->curl);
    curl_global_cleanup();
    SafeStrDup(
      that->errMsg,
      curl_easy_strerror(res));
    Raise(RunRecorderExc_CurlSetOptFailed);

  }

  // Set the pointer where to memorise received data
  res =
    curl_easy_setopt(
      that->curl,
      CURLOPT_WRITEDATA,
      &(that->curlReply));
  if (res != CURLE_OK) {

    curl_easy_cleanup(that->curl);
    curl_global_cleanup();
    SafeStrDup(
      that->errMsg,
      curl_easy_strerror(res));
    Raise(RunRecorderExc_CurlSetOptFailed);

  }

  // Set the callback to receive data
  res =
    curl_easy_setopt(
      that->curl,
      CURLOPT_WRITEFUNCTION,
      GetReplyAPI);
  if (res != CURLE_OK) {

    curl_easy_cleanup(that->curl);
    curl_global_cleanup();
    SafeStrDup(
      that->errMsg,
      curl_easy_strerror(res));
    Raise(RunRecorderExc_CurlSetOptFailed);

  }

}

// Free the error messages of a struct RunRecorder
// Input:
//   that: The struct RunRecorder to be freed
static void FreeErrMsg(
  struct RunRecorder* const that) {

  // Free the error messages
  // SQLite3 error message needs to be freed its own way
  PolyFree(that->errMsg);
  that->errMsg = NULL;
  sqlite3_free(that->sqliteErrMsg);
  that->sqliteErrMsg = NULL;

}

// Check if a struct RunRecorder uses a local SQLite database or the 
// Web API
// Input:
//   that: The struct RunRecorder
// Return:
//   Return true if the struct RunRecorder uses the Web API, else false
static bool UsesAPI(
  struct RunRecorder const* const that) {

  // If the url starts with http the struct RunRecorder uses the Web API
  char const* http =
    strstr(
      that->url,
      "http");
  return (http == that->url);

}

// Reset the Curl reply, to be called before sending a new curl request
// or the result of the new request will be appended to the eventual
// one of the previous request
// Input:
//   The struct RunRecorder
static void ResetCurlReply(
  struct RunRecorder* const that) {

  // Free the Curl reply
  PolyFree(that->curlReply);
  that->curlReply = NULL;

}

// Create the RunRecorder's tables in an empty database
// Input:
//   that: The struct RunRecorder
static void CreateDb(
  struct RunRecorder* const that) {

  // If the RunRecorder uses a local database, create the local database
  if (UsesAPI(that) == false) CreateDbLocal(that);

  // If the RunRecorder uses the Web API there is nothing to do as the
  // remote API will take care of creating the  database as necessary

}

// Create the RunRecorder's tables in an empty local database
// Input:
//   that: The struct RunRecorder
// Raise:
//   RunRecorderExc_CreateTableFailed
static void CreateDbLocal(
  struct RunRecorder* const that) {

  // Ensure the error messages are freed to avoid confusion with
  // eventual previous messages
  FreeErrMsg(that);

  // List of commands to create the tables
  char* sqlCmd[NB_TABLE + 1] = {

    "CREATE TABLE _Version ("
    "  Ref INTEGER PRIMARY KEY,"
    "  Label TEXT NOT NULL)",
    "CREATE TABLE _Project ("
    "  Ref INTEGER PRIMARY KEY,"
    "  Label TEXT UNIQUE NOT NULL)",
    "CREATE TABLE _Measure ("
    "  Ref INTEGER PRIMARY KEY,"
    "  RefProject INTEGER NOT NULL,"
    "  DateMeasure DATETIME NOT NULL)",
    "CREATE TABLE _Value ("
    "  Ref INTEGER PRIMARY KEY,"
    "  RefMeasure INTEGER NOT NULL,"
    "  RefMetric INTEGER NOT NULL,"
    "  Value TEXT NOT NULL)",
    "CREATE TABLE _Metric ("
    "  Ref INTEGER PRIMARY KEY,"
    "  RefProject INTEGER NOT NULL,"
    "  Label TEXT NOT NULL,"
    "  DefaultValue TEXT NOT NULL)",
    "INSERT INTO _Version (Ref, Label) "
    "VALUES (NULL, '" VERSION_DB "')"

  };

  // Loop on the commands
  ForZeroTo(iCmd, NB_TABLE + 1) {

    // Execute the command
    int retExec =
      sqlite3_exec(
        that->db,
        sqlCmd[iCmd],
        // No callback
        NULL,
        // No user data
        NULL,
        &(that->sqliteErrMsg));
    if (retExec != SQLITE_OK) Raise(RunRecorderExc_CreateTableFailed);

  }

}

// Upgrade the database
// Input:
//   that: The struct RunRecorder
static void UpgradeDb(
  struct RunRecorder* const that) {

  // Unused argument
  (void)that;

  // Just a placeholder, nothing to do yet as there is currently only
  // one version of the database

}

// Callback for GetVersionLocal
// Input:
//      data: The version
//     nbCol: Number of columns in the returned rows
//    colVal: Row values
//   colName: Columns name
// Output:
//   Return 0 if successfull, else 1
static int GetVersionLocalCb(
   void* data,
     int nbCol,
  char** colVal,
  char** colName) {

  // Unused argument
  (void)colName;

  // If the arguments are invalid
  // Return non zero to trigger SQLITE_ABORT in the calling function
  if (nbCol != 1 || colVal == NULL || *colVal == NULL) return 1;

  // Cast the data
  char** ptrVersion = (char**)data;

  // Memorise the returned value
  Try {

    SafeStrDup(
      *ptrVersion,
      *colVal);

  } CatchDefault {

    // Return non zero to trigger SQLITE_ABORT in the calling function
    return 1;

  } EndTryWithDefault;

  // Return success code
  return 0;

}

// Get the version of the local database
// Input:
//   that: The struct RunRecorder
// Output:
//   Return a new string
// Raise:
//   RunRecorderExc_SQLRequestFailed
static char* GetVersionLocal(
  struct RunRecorder* const that) {

  // Declare a variable to memorise the version
  char* version = NULL;

  // Execute the command to get the version
  char* sqlCmd = "SELECT Label FROM _Version LIMIT 1";
  int retExec =
    sqlite3_exec(
      that->db,
      sqlCmd,
      GetVersionLocalCb,
      &version,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_SQLRequestFailed);

  // Return the version
  return version;

}

// Callback to memorise the incoming data from the Web API
// Input:
//   data: incoming data
//   size: always 1
//   nmemb: number of incoming byte
//   reply: pointer to where to store the incoming data
// Output:
//   Return the number of received byte
static size_t GetReplyAPI(
   char* data,
  size_t size,
  size_t nmemb,
   void* ptr) {

  // Get the size in byte of the received data
  size_t dataSize = size * nmemb;

  // If there is received data
  if (dataSize != 0) {

    // Cast the data
    char** reply = (char**)ptr;

    // Variable to memorise the current length of the buffer for the reply
    size_t replyLength = 0;

    // If the current buffer for the reply is not empty, get the current
    // length of the reply
    if (*reply != NULL) replyLength = strlen(*reply);

    // Allocate memory for current data, the incoming data and the
    // terminating '\0'
    SafeRealloc(
      *reply,
      replyLength + dataSize + 1);

    // Copy the incoming data at the end of the current buffer
    memcpy(
      *reply + replyLength,
      data,
      dataSize);

    // Set the terminating NULL character
    (*reply)[replyLength + dataSize] = '\0';

  }

  // Return the number of byte received;
  return dataSize;

}

// Get the value of a key in a JSON encoded string
// Input:
//   json: the json encoded string
//    key: the key
// Output:
//   Return a new string, or NULL if the key doesn't exist
// Raise:
//   RunRecorderExc_ApiRequestFailed
static char* GetJSONValOfKey(
  char const* const json,
  char const* const key) {

  // If the json or key is null or empty
  // Nothing to do
  if (json == NULL || key == NULL ||
      *json == '\0' || *key == '\0') return NULL;

  // Variable to memorise the value
  char* val = NULL;

  // Variable to memorise the key decorated with it's syntax
  char* keyDecorated = NULL;

  Try {

    // Create the key decorated with it's syntax
    char* keyFormat = "\"%s\":";
    size_t lenKeyDecorated = strlen(keyFormat) - 2 + strlen(key) + 1;
    SafeMalloc(
      keyDecorated,
      lenKeyDecorated);
    sprintf(
      keyDecorated,
      keyFormat,
      key);

    // If the key can be find in the JSON encoded string
    char const* ptrKey =
      strstr(
        json,
        keyDecorated);
    if (ptrKey != NULL) {

      // Variable to memorise the start of the value
      char const* ptrVal = ptrKey + strlen(keyDecorated) + 1;

      // Declare a pointer to loop on the string until the end of the value
      char const* ptr = ptrVal;

      // If the value is a string
      if (ptrKey[strlen(keyDecorated)] == '"') {

        // Loop on the characters of the value until the next double quote
        // or end of string
        while (*ptr != '\0' && *ptr != '"') {

          // Move to the next character
          ++ptr;

          // Skip the escaped character
          if (*ptr == '\\') ++ptr;

        }

      // Else, if the value is an object
      } else if (ptrKey[strlen(keyDecorated)] == '{') {

        // Loop on the characters of the value until the closing curly
        // brace or end of string
        // Presume the data are well formatted here, if there are curly
        // braces in string in the object definition, or sub-object
        // definition, or any other special case, it means the data
        // returned by the API are invalid and it will be catched later.
        while (*ptr != '\0' && *ptr != '}') ++ptr;

      // Else, this JSON encoded string is not supported, the API request
      // has returned invalid data
      } else {

        Raise(RunRecorderExc_ApiRequestFailed);

      }

      // If we have found the end of the value
      if (*ptr != '\0') {

        // Get the length of the value
        size_t lenVal = ptr - ptrVal;

        // Allocate memory for the value
        SafeMalloc(
          val,
          lenVal + 1);

        // Copy the value
        memcpy(
          val,
          ptrVal,
          lenVal);
        val[lenVal] = '\0';

      // Else, this JSON encoded string is not supported, the API request
      // has returned invalid data
      } else {

        Raise(RunRecorderExc_ApiRequestFailed);

      }

    }

    // Free memory
    PolyFree(keyDecorated);

  } CatchDefault {
    
    PolyFree(keyDecorated);
    PolyFree(val);
    Raise(TryCatchGetLastExc());

  } EndTryWithDefault;

  // Return the value
  return val;

}

// Set the POST data in the Web API request of a struct RunRecorder
// Input:
//   that: the struct RunRecorder
//   data: the POST data
// Raise:
//   RunRecorderExc_CurlSetOptFailed
static void SetAPIReqPostVal(
  struct RunRecorder* const that,
  char const* const data) {

  // Set the data in the Curl fields
  CURLcode res =
    curl_easy_setopt(
      that->curl,
      CURLOPT_POSTFIELDS,
      data);
  if (res != CURLE_OK) {

    SafeStrDup(
      that->errMsg,
      curl_easy_strerror(res));
    Raise(RunRecorderExc_CurlSetOptFailed);

  }

}

// Get the ret code in the current JSON reply of a struct RunRecorder
// Input:
//   that: the struct RunRecorder
// Output:
//   Return a new string
// Raise:
//   RunRecorderExc_ApiRequestFailed
static char* GetAPIRetCode(
  struct RunRecorder* const that) {

  // Extract the return code from the JSON reply
  char* retCode =
    GetJSONValOfKey(
      that->curlReply,
      "ret");
  if (retCode == NULL) {

    SafeStrDup(
      that->errMsg,
      "'ret' key missing in API reply");
    Raise(RunRecorderExc_ApiRequestFailed);

  }

  // Return the code
  return retCode;

}

// Send the current request of a struct RunRecorder
// Input:
//        that: the struct RunRecorder
//   isJsonReq: flag to indicate that the request returns JSON encoded
//              data
// Raise:
//   RunRecorderExc_CurlRequestFailed
//   RunRecorderExc_ApiRequestFailed
static void SendAPIReq(
  struct RunRecorder* const that,
                 bool const isJsonReq) {

  // Reset the Curl reply or the reply of this request will get appended
  // to the one of the previous request
  ResetCurlReply(that);

  // Send the request
  CURLcode res = curl_easy_perform(that->curl);
  if (res != CURLE_OK) {

    SafeStrDup(
      that->errMsg,
      curl_easy_strerror(res));
    Raise(RunRecorderExc_CurlRequestFailed);

  }

  // If the request returns JSON encoded data
  if (isJsonReq == true) {

    // If the returned code is not '0'
    char* retCode = GetAPIRetCode(that);
    int cmpRet =
      strcmp(
        retCode,
        "0");
    PolyFree(retCode);
    if (cmpRet != 0) {

      PolyFree(that->errMsg);
      that->errMsg =
        GetJSONValOfKey(
          that->curlReply,
          "errMsg");
      Raise(RunRecorderExc_ApiRequestFailed);

    }

  }

}

// Get the version of the database via the Web API
// Input:
//   The struct RunRecorder
// Output:
//   Return a new string
static char* GetVersionAPI(
  struct RunRecorder* const that) {

  // Create the request to the Web API
  SetAPIReqPostVal(
    that,
    "action=version");

  // Send the request to the API
  bool isJsonReq = true;
  SendAPIReq(
    that,
    isJsonReq);

  // Extract the version number from the JSON reply
  char* version =
    GetJSONValOfKey(
      that->curlReply,
      "version");

  // Return the version
  return version;

}

// Add a new project in a local database
// Input:
//   that: the struct RunRecorder
//   name: the name of the new project, it must respect the following
//         pattern: /^[a-zA-Z][a-zA-Z0-9_]*$/
// Raise:
//   RunRecorderExc_InvalidProjectName
//   RunRecorderExc_ProjectNameAlreadyUsed
static void AddProjectLocal(
  struct RunRecorder* const that,
  char const* const name) {

  // Create the SQL command
  char* cmdFormat =
    "INSERT INTO _Project (Ref, Label) VALUES (NULL, \"%s\")";
  PolyFree(that->cmd);
  size_t lenCmd = strlen(cmdFormat) - 2 + strlen(name) + 1;
  SafeMalloc(
    that->cmd,
    lenCmd);
  sprintf(
    that->cmd,
    cmdFormat,
    name);

  // Execute the command to add the project
  int retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      NULL,
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_AddProjectFailed);

}

// Add a new project through the Web API
// Input:
//   that: the struct RunRecorder
//   name: the name of the new project, it must respect the following
//         pattern: /^[a-zA-Z][a-zA-Z0-9_]*$/
// Raise:
//   RunRecorderExc_InvalidProjectName
//   RunRecorderExc_ProjectNameAlreadyUsed
static void AddProjectAPI(
  struct RunRecorder* const that,
  char const* const name) {

  // Create the request to the Web API
  char* cmdFormat = "action=add_project&label=%s";
  PolyFree(that->cmd);
  size_t lenCmd = strlen(cmdFormat) - 2 + strlen(name) + 1;
  SafeMalloc(
    that->cmd,
    lenCmd);
  sprintf(
    that->cmd,
    cmdFormat,
    name);
  SetAPIReqPostVal(
    that,
    that->cmd);

  // Send the request to the API
  bool isJsonReq = true;
  SendAPIReq(
    that,
    isJsonReq);

}

// Check if a value is present in pairs ref/val
// Inputs:
//   that: the struct RunRecorderPairsRefVal
//    val: the value to check
// Output:
//   Return true if the value is present in the pairs, else false
static bool PairsRefValContainsVal(
  struct RunRecorderPairsRefVal const* const that,
                           char const* const val) {

  // Loop on the pairs
  ForZeroTo(iPair, that->nb) {

    // If the pair's value is the checked value, return true
    int retCmp =
      strcmp(
        val,
        that->values[iPair]);
    if (retCmp == 0) return true;

  }

  // If we reach here, we haven't found the checked value, return false
  return false;

}

// Add a new pair in a struct RunRecorderPairsRefVal
// Inputs:
//   pairs: the struct RunRecorderPairsRefVal
//     ref: the reference of the pair
//     val: the val of the pair
static void PairsRefValAdd(
  struct RunRecorderPairsRefVal* const pairs,
                            long const ref,
                     char const* const val) {

  // Allocate memory for the new value
  SafeRealloc(
    pairs->refs,
    sizeof(long) * (pairs->nb + 1));
  SafeRealloc(
    pairs->values,
    sizeof(char*) * (pairs->nb + 1));
  pairs->values[pairs->nb] = NULL;

  // Update the number of pairs
  ++(pairs->nb);

  // Set the reference and value of the pair
  pairs->refs[pairs->nb - 1] = ref;
  SafeStrDup(
    pairs->values[pairs->nb - 1],
    val);

}

// Callback to receive pairs of ref/value from sqlite3
// Input:
//      data: The pairs
//     nbCol: Number of columns in the returned rows
//    colVal: Row values, the reference is expected to be in the first
//            column and the value in the second column
//   colName: Columns name
// Output:
//   Return 0 if successful, else 1
static int GetPairsLocalCb(
   void* data,
     int nbCol,
  char** colVal,
  char** colName) {

  // Unused argument
  (void)colName;

  // If the arguments are invalid
  // Return non zero to trigger SQLITE_ABORT in the calling function
  if (nbCol != 2 || colVal == NULL ||
      colVal[0] == NULL || colVal[1] == NULL) return 1;

  // Cast the data
  struct RunRecorderPairsRefVal* pairs =
    (struct RunRecorderPairsRefVal*)data;

  // Convert the reference contained in the first column from char* to long
  errno = 0;
  long ref =
    strtol(
      colVal[0],
      NULL,
      10);

  // If the conversion failed, return non zero to trigger SQLITE_ABORT
  // in the calling function
  if (errno != 0) return 1;

  // Add the pair ref/value to the pairs
  Try {

    PairsRefValAdd(
      pairs,
      ref,
      colVal[1]);

  } CatchDefault {

    return 1;

  } EndTryWithDefault;

  // Return success code
  return 0;

}

// Get the list of projects in the local database
// Input:
//   that: the struct RunRecorder
// Output:
//   Return the projects' reference/label
// Raise:
//   RunRecorderExc_SQLRequestFailed
static struct RunRecorderPairsRefVal* GetProjectsLocal(
  struct RunRecorder* const that) {

  // Declare a variable to memorise the projects
  struct RunRecorderPairsRefVal* projects = RunRecorderPairsRefValCreate();

  // Execute the command to get the version
  char* sqlCmd = "SELECT Ref, Label FROM _Project";
  int retExec =
    sqlite3_exec(
      that->db,
      sqlCmd,
      GetPairsLocalCb,
      projects,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) {

    RunRecorderPairsRefValFree(&projects);
    Raise(RunRecorderExc_SQLRequestFailed);

  }

  // Return the projects
  return projects;

}

// Extract a struct RunRecorderPairsRefVal from a JSON string
// Input:
//   json: the JSON string, expected to be formatted as "1":"A","2":"B",...
// Output:
//   Return a new struct RunRecorderPairsRefVal
// Raise:
//   RunRecorderExc_InvalidJSON
static struct RunRecorderPairsRefVal* GetPairsRefValFromJSON(
  char const* const json) {

  // Create the pairs
  struct RunRecorderPairsRefVal* pairs = RunRecorderPairsRefValCreate();

  // Declare a pointer to loop on the json string
  char const* ptr = json;

  // Loop until the end of the json string
  while (*ptr != '\0') {

    // Go to the next double quote or end of string
    while (*ptr != '\0' && *ptr != '"') ++ptr;

    // If we have found the double quote
    if (*ptr == '"') {

      // Skip the opening double quote of the reference
      ++ptr;

      // Get the reference
      char* ptrEnd = NULL;
      errno = 0;
      long ref =
        strtol(
          ptr,
          &ptrEnd,
          10);
      if (errno != 0 || *ptrEnd != '"') Raise(RunRecorderExc_InvalidJSON);

      // Move to the character after the closing double quote
      ptr = ptrEnd + 1;

      // Go to the opening double quote for the value
      while (*ptr != '\0' && *ptr != '"') ++ptr;

      // If we couldn't find the opening double quote for the value
      if (*ptr == '\0') Raise(RunRecorderExc_InvalidJSON);

      // Skip the opening double quote for the value
      ++ptr;

      // Search the closing double quote for the value
      char const* ptrEndVal = ptr;
      while (*ptrEndVal != '\0' && *ptrEndVal != '"') ++ptrEndVal;

      // If we couldn't find the closing double quote for the value
      // or the value is null
      if (*ptrEndVal == '\0' || ptrEndVal == ptr)
        Raise(RunRecorderExc_InvalidJSON);

      // Variable to memorise the value
      char* val = NULL;

      Try {

        // Get the value
        size_t lenVal = ptrEndVal - ptr;
        SafeMalloc(
          val,
          lenVal + 1);
        memcpy(
          val,
          ptr,
          lenVal);
        val[lenVal] = '\0';

        // Move to the character after the closing double quote of the value
        ptr = ptrEndVal + 1;

        // Add the pair
        PairsRefValAdd(
          pairs,
          ref,
          val);

        // Free the temporary copy of the value
        PolyFree(val);

      } CatchDefault {

        PolyFree(val);
        Raise(TryCatchGetLastExc());

      } EndTryWithDefault;

    }

  }

  // Return the pairs
  return pairs;

}

// Get the list of projects through the Web API
// Input:
//   that: the struct RunRecorder
// Output:
//   Return the projects' reference/label
static struct RunRecorderPairsRefVal* GetProjectsAPI(
  struct RunRecorder* const that) {

  // Create the request to the Web API
  SetAPIReqPostVal(
    that,
    "action=projects");

  // Send the request to the API
  bool isJsonReq = true;
  SendAPIReq(
    that,
    isJsonReq);

  // Variable to memorise the value of the 'projects' key
  char* json = NULL;

  // Variable to memorise the projects
  struct RunRecorderPairsRefVal* projects = NULL;
  Try {

    // Get the projects list in the JSON reply
    char* json =
      GetJSONValOfKey(
        that->curlReply,
        "projects");
    if (json == NULL) Raise(RunRecorderExc_ApiRequestFailed);

    // Extract the projects
    projects = GetPairsRefValFromJSON(json);

    // Free memory
    PolyFree(json);

  } CatchDefault {

    PolyFree(json);
    Raise(TryCatchGetLastExc());
    
  } EndTryWithDefault;

  // Return the projects
  return projects;

}

// Create a struct RunRecorderPairsRefVal
// Output:
//   Return the new struct RunRecorderPairsRefVal
static struct RunRecorderPairsRefVal* RunRecorderPairsRefValCreate(
  void) {

  // Declare the new struct RunRecorderPairsRefVal
  struct RunRecorderPairsRefVal* pairs = NULL;
  SafeMalloc(
    pairs,
    sizeof(struct RunRecorderPairsRefVal));

  // Initialise properties
  pairs->nb = 0;
  pairs->refs = NULL;
  pairs->values = NULL;

  // Return the new struct RunRecorderPairsRefVal
  return pairs;

}

// Get the list of metrics for a project from a local database
// Input:
//      that: the struct RunRecorder
//   project: the project
// Output:
//   Return the metrics' reference/label
// Raise:
//   RunRecorderExc_SQLRequestFailed
static struct RunRecorderPairsRefVal* GetMetricsLocal(
  struct RunRecorder* const that,
          char const* const project) {

  // Create the request
  char* cmdFormat =
    "SELECT _Metric.Ref, _Metric.Label FROM _Metric, _Project "
    "WHERE _Metric.RefProject = _Project.Ref AND "
    "_Project.Label = \"%s\"";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormat) - 2 + strlen(project) + 1);
  sprintf(
    that->cmd,
    cmdFormat,
    project);

  // Declare a variable to memorise the metrics
  struct RunRecorderPairsRefVal* metrics = NULL;
  Try {

    // Create the struct RunRecorderPairsRefVal to memorise the metrics
    metrics = RunRecorderPairsRefValCreate();

    // Execute the request
    int retExec =
      sqlite3_exec(
        that->db,
        that->cmd,
        GetPairsLocalCb,
        metrics,
        &(that->sqliteErrMsg));
    if (retExec != SQLITE_OK) Raise(RunRecorderExc_SQLRequestFailed);

  } CatchDefault {

      RunRecorderPairsRefValFree(&metrics);
      Raise(TryCatchGetLastExc());

  } EndTryWithDefault;

  // Return the metrics
  return metrics;

}

// Get the list of metrics for a project through the Web API
// Input:
//      that: the struct RunRecorder
//   project: the project
// Output:
//   Return the metrics' reference/label
// Raise:
//   RunRecorderExc_ApiRequestFailed
static struct RunRecorderPairsRefVal* GetMetricsAPI(
  struct RunRecorder* const that,
          char const* const project) {

  // Create the request to the Web API
  char* cmdFormat = "action=metrics&project=%s";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormat) - 2 + strlen(project) + 1);
  sprintf(
    that->cmd,
    cmdFormat,
    project);
  SetAPIReqPostVal(
    that,
    that->cmd);

  // Send the request to the API
  bool isJsonReq = true;
  SendAPIReq(
    that,
    isJsonReq);

  // Variable to memorise the value of the 'metrics' key
  char* json = NULL;

  // Variable to memorise the metrics
  struct RunRecorderPairsRefVal* metrics = NULL;
  Try {

    // Get the metrics list in the JSON reply
    json =
      GetJSONValOfKey(
        that->curlReply,
        "metrics");
    if (json == NULL) Raise(RunRecorderExc_ApiRequestFailed);

    // Extract the metrics
    metrics = GetPairsRefValFromJSON(json);

    // Free memory
    PolyFree(json);

  } CatchDefault {

    PolyFree(json);
    RunRecorderPairsRefValFree(&metrics);
    Raise(TryCatchGetLastExc());
    
  } EndTryWithDefault;

  // Return the metrics
  return metrics;

}

// Update the view for a project
// Input:
//         that: the struct RunRecorder
//      project: the name of the project
// Raise:
//   RunRecorderExc_UpdateViewFailed
static void UpdateViewProject(
  struct RunRecorder* const that,
          char const* const project) {

  // Create the SQL command to delete the view
  char* cmdDelFormat = "DROP VIEW IF EXISTS %s";
  SafeMalloc(
    that->cmd,
    strlen(cmdDelFormat) - 2 + strlen(project) + 1);
  sprintf(
    that->cmd,
    cmdDelFormat,
    project);

  // Execute the command to delete the view
  int retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      NULL,
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_UpdateViewFailed);

  // Get the list of metrics for the project
  struct RunRecorderPairsRefVal* metrics =
    RunRecorderGetMetrics(
      that,
      project);
  Try {

    // Declare the format strings to create the SQL command to add the view
    char* cmdAddFormatHead = "CREATE VIEW %s (Ref";
    char* cmdAddFormatBody = ") AS SELECT _Measure.Ref ";
    char* cmdAddFormatVal =
      ",IFNULL((SELECT Value FROM _Value "
      "WHERE RefMeasure=_Measure.Ref AND RefMetric=%ld),"
      "(SELECT DefaultValue FROM _Metric WHERE Ref=%ld)) ";

    // Create the head of the command
    SafeMalloc(
      that->cmd,
      strlen(cmdAddFormatHead) - 2 + strlen(project) +
      strlen(cmdAddFormatBody) + 1);
    sprintf(
      that->cmd,
      cmdAddFormatHead,
      project);

    // Loop on the metrics
    ForZeroTo(iMetric, metrics->nb) {

      // Extend the command with the metric label
      SafeRealloc(
        that->cmd,
        strlen(that->cmd) + 1 + strlen(metrics->values[iMetric]) + 1);
      SPrintfAtEnd(
        that->cmd,
        ",%s",
        metrics->values[iMetric]);

    }

    // Extend the command with the body
    SafeRealloc(
      that->cmd,
      strlen(that->cmd) + strlen(cmdAddFormatBody) + 1);
    SPrintfAtEnd(
      that->cmd,
      "%s",
      cmdAddFormatBody);

    // Loop on the metrics
    ForZeroTo(iMetric, metrics->nb) {

      // Get the length of the metric reference as a string
      size_t lenRefMetricStr =
        snprintf(
          NULL,
          0,
          "%ld",
          metrics->refs[iMetric]);

      // Extend the command with the metric related body part
      SafeRealloc(
        that->cmd,
        strlen(that->cmd) + strlen(cmdAddFormatVal) - 3 + 
        lenRefMetricStr - 3 + lenRefMetricStr + 1);
      SPrintfAtEnd(
        that->cmd,
        cmdAddFormatVal,
        metrics->refs[iMetric],
        metrics->refs[iMetric]);

    }

    // Free memory
    RunRecorderPairsRefValFree(&metrics);

  } CatchDefault {

    RunRecorderPairsRefValFree(&metrics);
    Raise(TryCatchGetLastExc());

  } EndTryWithDefault;

  // Extend the command with the tail
  char* cmdAddFormatTail =
    "FROM _Measure, _Project WHERE _Measure.RefProject = _Project.Ref AND "
    "_Project.Label = \"%s\" ORDER BY _Measure.DateMeasure, _Measure.Ref";
  SafeRealloc(
    that->cmd,
    strlen(that->cmd) + strlen(cmdAddFormatTail) - 2 + strlen(project) + 1);
  SPrintfAtEnd(
    that->cmd,
    cmdAddFormatTail,
    project);

  // Execute the command to add the view
  retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      NULL,
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_UpdateViewFailed);

}

// Add a metric to a project to a local database
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
//   RunRecorderExc_InvalidMetricName
//   RunRecorderExc_MetricNameAlreadyUsed
static void AddMetricLocal(
  struct RunRecorder* const that,
          char const* const project,
          char const* const label,
          char const* const defaultVal) {

  // Create the SQL command
  char* cmdFormat =
    "INSERT INTO _Metric (Ref, RefProject, Label, DefaultValue) "
    "SELECT NULL, _Project.Ref, \"%s\", \"%s\" FROM _Project "
    "WHERE _Project.Label = \"%s\"";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormat) - 2 + strlen(label) - 2 +
    strlen(defaultVal) - 2 + strlen(project) + 1);
  sprintf(
    that->cmd,
    cmdFormat,
    label,
    defaultVal,
    project);

  // Execute the command to add the metric
  int retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      NULL,
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_AddMetricFailed);

  // Update the view for this project
  UpdateViewProject(
    that,
    project);

}

// Add a metric to a project through the Web API
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
//   RunRecorderExc_InvalidMetricName
//   RunRecorderExc_MetricNameAlreadyUsed
static void AddMetricAPI(
  struct RunRecorder* const that,
          char const* const project,
          char const* const label,
          char const* const defaultVal) {

  // Create the request to the Web API
  char* cmdFormat = "action=add_metric&project=%s&label=%s&default=%s";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormat) - 2 + strlen(label) - 2 +
    strlen(project) - 2 + strlen(defaultVal) + 1);
  sprintf(
    that->cmd,
    cmdFormat,
    project,
    label,
    defaultVal);
  SetAPIReqPostVal(
    that,
    that->cmd);

  // Send the request to the API
  bool isJsonReq = true;
  SendAPIReq(
    that,
    isJsonReq);

}

// Add a measure to a project in a local database
// Inputs:
//         that: the struct RunRecorder
//      project: the project to add the measure to
//      measure: the measure to add
// Raise:
//   RunRecorderExc_AddMeasureFailed
static void AddMeasureLocal(
               struct RunRecorder* const that,
                       char const* const project,
  struct RunRecorderMeasure const* const measure) {

  // Reset the reference of the last added measure
  that->refLastAddedMeasure = 0;

  // Get the date of the record (use the current local date)
  time_t mytime = time(NULL);
  char* dateStr = ctime(&mytime);

  // Remove the line return at the end of the date
  dateStr[strlen(dateStr) - 1] = '\0';

  // Create the SQL command
  char* cmdFormat =
    "INSERT INTO _Measure (RefProject, DateMeasure) "
    "SELECT _Project.Ref, \"%s\" FROM _Project "
    "WHERE _Project.Label = \"%s\"";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormat) - 2 + strlen(dateStr) - 2 + strlen(project) + 1);
  sprintf(
    that->cmd,
    cmdFormat,
    dateStr,
    project);

  // Execute the command to add the measure
  int retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      NULL,
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_AddMeasureFailed);

  // Get the reference of the measure
  that->refLastAddedMeasure = sqlite3_last_insert_rowid(that->db);
  int lenRefMeasureStr =
    snprintf(
    NULL,
    0,
    "%ld",
    that->refLastAddedMeasure);

  // Declare a variable to memorise an eventual failure
  // The policy here is to try to save has much data has possible
  // even if some fails, inform the user and let him/her take
  // appropriate action
  bool hasFailed = false;

  // Loop on the values in the measure
  ForZeroTo(iVal, measure->nbMetric) {

    // Create the SQL command
    char* cmdValBase =
      "INSERT INTO _Value (RefMeasure, RefMetric, Value) "
      "SELECT %ld, _Metric.Ref, \"%s\" FROM _Metric "
      "WHERE _Metric.Label = \"%s\"";
    Try {

      SafeMalloc(
        that->cmd,
        strlen(cmdValBase) + lenRefMeasureStr - 3 +
        strlen(measure->values[iVal]) - 2 +
        strlen(measure->metrics[iVal]) - 2 + 1);

    } CatchDefault {

      hasFailed = true;

    } EndTryWithDefault;

    if (that->cmd != NULL) {

      sprintf(
        that->cmd,
        cmdValBase,
        that->refLastAddedMeasure,
        measure->values[iVal],
        measure->metrics[iVal]);

      // Execute the command to add the value
      int retExec =
        sqlite3_exec(
          that->db,
          that->cmd,
          NULL,
          NULL,
          &(that->sqliteErrMsg));
      if (retExec != SQLITE_OK) hasFailed = true;

    }

  }

  // If there has been a failure, raise an exception
  if (hasFailed == true) Raise(RunRecorderExc_AddMeasureFailed);

}

// Add a measure to a project through the WebAPI
// Inputs:
//         that: the struct RunRecorder
//      project: the project to add the measure to
//      measure: the measure to add
// Raise:
//   RunRecorderExc_ApiRequestFailed
static void AddMeasureAPI(
               struct RunRecorder* const that,
                       char const* const project,
  struct RunRecorderMeasure const* const measure) {

  // Reset the reference of the last added measure
  that->refLastAddedMeasure = 0;

  // Format of the command for one value
  char* cmdFormatVal = "&%s=%s";

  // Variable to memorise the size of the values in the command string
  size_t lenStrValues = 0;

  // Loop on the values and add the size necessary for each value and its
  // header
  ForZeroTo(iVal, measure->nbMetric)
    lenStrValues += strlen(cmdFormatVal) - 2 +
      strlen(measure->metrics[iVal]) - 2 +
      strlen(measure->values[iVal]);

  // Create the request to the Web API
  char* cmdFormat = "action=add_measure&project=%s";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormat) - 2 + strlen(project) + lenStrValues + 1);
  sprintf(
    that->cmd,
    cmdFormat,
    project);
  ForZeroTo(iVal, measure->nbMetric)
    SPrintfAtEnd(
      that->cmd,
      cmdFormatVal,
      measure->metrics[iVal],
      measure->values[iVal]);
  SetAPIReqPostVal(
    that,
    that->cmd);

  // Send the request to the API
  bool isJsonReq = true;
  SendAPIReq(
    that,
    isJsonReq);

  // Extract the reference of the measure from the JSON reply
  char* version =
    GetJSONValOfKey(
      that->curlReply,
      "refMeasure");
  if (version == NULL) Raise(RunRecorderExc_ApiRequestFailed);
  errno = 0;
  that->refLastAddedMeasure = 
    strtol(
      version,
      NULL,
      10);
  PolyFree(version);
  if (errno != 0) Raise(RunRecorderExc_ApiRequestFailed);

}

// Delete a measure in a local database
// Inputs:
//       that: the struct RunRecorder
//    measure: the reference of the measure to delete
// Raise:
//   RunRecorderExc_DeleteMeasureFailed
static void DeleteMeasureLocal(
  struct RunRecorder* const that,
                 long const refMeasure) {

  // Create the SQL command to delete the measure's values
  size_t lenMeasureStr =
    snprintf(
      NULL,
      0,
      "%ld",
      refMeasure);
  char* cmdFormatVal = "DELETE FROM _Value WHERE RefMeasure = %ld";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormatVal) - 3 + lenMeasureStr + 1);
  sprintf(
    that->cmd,
    cmdFormatVal,
    refMeasure);

  // Execute the command to delete the measure's values
  int retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      NULL,
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_DeleteMeasureFailed);

  // Create the SQL command to delete the measure
  char* cmdFormat = "DELETE FROM _Measure WHERE Ref = %ld ; VACUUM";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormat) - 3 + lenMeasureStr + 1);
  sprintf(
    that->cmd,
    cmdFormat,
    refMeasure);

  // Execute the command to delete the measure
  retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      NULL,
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_DeleteMeasureFailed);

}

// Delete a measure through the Web API
// Inputs:
//          that: the struct RunRecorder
//    refMeasure: the reference of the measure to delete
static void DeleteMeasureAPI(
  struct RunRecorder* const that,
        long const refMeasure) {

  // Create the request to the Web API
  size_t lenMeasureStr =
    snprintf(
      NULL,
      0,
      "%ld",
      refMeasure);
  char* cmdFormat = "action=delete_measure&measure=%ld";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormat) - 3 + lenMeasureStr + 1);
  sprintf(
    that->cmd,
    cmdFormat,
    refMeasure);
  SetAPIReqPostVal(
    that,
    that->cmd);

  // Send the request to the API
  bool isJsonReq = true;
  SendAPIReq(
    that,
    isJsonReq);

}

// Callback to receive the measures from sqlite3
// Input:
//      data: The measures
//     nbCol: Number of columns in the returned rows
//    colVal: Row values
//   colName: Columns name
// Output:
//   Return 0 if successfull, else 1
static int GetMeasuresLocalCb(
   void* data,
     int nbCol,
  char** colVal,
  char** colName) {

  // Unused argument
  (void)colName;

  // Cast the data
  struct RunRecorderMeasures** measures = (struct RunRecorderMeasures**)data;

  // If the arguments are invalid
  // Return non zero to trigger SQLITE_ABORT in the calling function
  if (nbCol == 0 || colVal == NULL || colName == NULL ) return 1;

  Try {

    // If the measures are not allocated yet
    if (*measures == NULL) {

      // Allocate memory for the measures
      *measures = RunRecorderMeasuresCreate();

      // Copy the metrics label
      (*measures)->nbMetric = nbCol;
      SafeMalloc(
        (*measures)->metrics,
        sizeof(char*) * nbCol);
      ForZeroTo(iMetric, (*measures)->nbMetric)
        (*measures)->metrics[iMetric] = NULL;
      ForZeroTo(iMetric, (*measures)->nbMetric)
        SafeStrDup(
          (*measures)->metrics[iMetric],
          colName[iMetric]);

    // Else, the measures are already allocated
    } else {

      // If the number of columns in data doesn't match the number of
      // metrics in the struct RunRecordMeasures in argument,
      // return non zero to trigger SQLITE_ABORT in the calling function
      if (nbCol != (*measures)->nbMetric) return 1;

    }

    // Allocate memory for the received measure
    SafeRealloc(
      (*measures)->values,
      sizeof(char**) * ((*measures)->nbMeasure + 1));
    (*measures)->values[(*measures)->nbMeasure] = NULL;

    // Allocate memory for the received measure's values
    SafeMalloc(
      (*measures)->values[(*measures)->nbMeasure],
      sizeof(char*) * nbCol);
    ForZeroTo(iMetric, (*measures)->nbMetric)
      (*measures)->values[(*measures)->nbMeasure][iMetric] = NULL;

    // Update the number of measure
    ++((*measures)->nbMeasure);

    // For each metric, copy the value of the received measure
    ForZeroTo(iMetric, (*measures)->nbMetric)
      SafeStrDup(
        (*measures)->values[(*measures)->nbMeasure - 1][iMetric],
        colVal[iMetric]);

  } CatchDefault {

    return 1;

  } EndTryWithDefault;

  // Return success code
  return 0;

}

// Helper function to commonalize code between GetMeasures and
// GetLastMeasures
// Inputs:
//        that: the struct RunRecorder
//     project: the project's name
//   nbMeasure: the number of measures returned, if 0 all measures are
//              returned
// Output:
//   Set the SQL command in that->cmd to get measures as a struct
//    RunRecorderMeasures
static void SetCmdToGetMeasuresLocal(
  struct RunRecorder* const that,
          char const* const project,
                 long const nbMeasure) {

  // Get the list of metrics for the project
  struct RunRecorderPairsRefVal* metrics =
    RunRecorderGetMetrics(
      that,
      project);

  Try {

    // Create the head of the command
    char* cmdFormatHead = "SELECT Ref,";
    SafeMalloc(
      that->cmd,
      strlen(cmdFormatHead) + 1);
    sprintf(
      that->cmd,
      "%s",
      cmdFormatHead);

    // For each metric
    ForZeroTo(iMetric, metrics->nb) {

      // Append the metric label to the command
      size_t len = strlen(that->cmd) + strlen(metrics->values[iMetric]) + 1 + 1;
      SafeRealloc(
        that->cmd,
        len);
      char sep = ',';
      if (iMetric == metrics->nb - 1) sep = ' ';
      SPrintfAtEnd( 
        that->cmd,
        "%s%c",
        metrics->values[iMetric],
        sep);

    }

    // Free memory
    RunRecorderPairsRefValFree(&metrics);

    // Append the tail of the command
    char* cmdFormatTail = "FROM %s";
    SafeRealloc(
      that->cmd,
      strlen(that->cmd) +
      strlen(cmdFormatTail) + strlen(project) - 2 + 1);
    SPrintfAtEnd(
      that->cmd,
      cmdFormatTail,
      project);

    // If there is a limit on the number of measures to be returned
    if (nbMeasure > 0) {

      // Get the length of nbMeasure converted as a string
      size_t lenNbMeasureStr =
        snprintf(
          NULL,
          0,
          "%ld",
          nbMeasure);

      // Append the limit at the end of the command
      char* cmdLimit = " ORDER BY Ref DESC LIMIT %ld";
      SafeRealloc(
        that->cmd,
        strlen(that->cmd) +
        strlen(cmdLimit) + lenNbMeasureStr - 3 + 1);
      SPrintfAtEnd(
        that->cmd,
        cmdLimit,
        nbMeasure);

    }

  } CatchDefault {

    RunRecorderPairsRefValFree(&metrics);
    Raise(TryCatchGetLastExc());

  } EndTryWithDefault;

}

// Get the measures of a project from a local database
// Inputs:
//         that: the struct RunRecorder
//      project: the project's name
// Output:
//   Return the measures as a struct RunRecorderMeasures
// Raise:
//   RunRecorderExc_SQLRequestFailed
static struct RunRecorderMeasures* GetMeasuresLocal(
  struct RunRecorder* const that,
          char const* const project) {

  // Declare the struct RunRecorderMeasures to memorise the measures
  struct RunRecorderMeasures* measures = NULL;

  // Create the request with no limit on the number of returned measures
  long nbMeasure = 0;
  SetCmdToGetMeasuresLocal(
    that,
    project,
    nbMeasure);

  // Execute the request
  int retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      GetMeasuresLocalCb,
      &measures,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_SQLRequestFailed);

  // Return the measures
  return measures;

}

// Helper function to commonalize code in RunRecorderCSVToData
// Inputs
//   csv: Pointer to the start of the row in CSV data
//   tgt: The array of char* where to copy the values
//   sep: The separator character
// Output:
//   Return a pointer to the end of the row
static char const* SplitCSVRowToData(
   char const* const csv,
        char** const tgt,
          char const sep) {

  // Loop on one line of the CSV data
  long iCol = 0;
  char const* ptr = csv;
  while (*ptr != '\n') {

    // Get the next separator
    char const* ptrSep = ptr;
    if (ptr != csv) ++ptrSep;
    while (*ptrSep != sep && *ptrSep != '\n' && *ptrSep != '\0') ++ptrSep;

    // If the next separator is the current position or the next character
    if (ptrSep == ptr || ptrSep == ptr + 1) {

      // If the current position is a separator
      if (*ptr == sep) {

        // The column is empty
        SafeMalloc(
          tgt[iCol],
          1);
        tgt[iCol][0] = '\0';

      // Else, the current position is not a separator 
      } else {

        // The column is one character wide
        SafeMalloc(
          tgt[iCol],
          2);
        tgt[iCol][0] = *ptr;
        tgt[iCol][1] = '\0';

      }

    // Else the next separator is more than 1 character away
    } else {

      // If the current position is still on the last separator, increment it
      if (*ptr == sep || *ptr == '\n') ++ptr;

      // Get the length of the column
      size_t len = ptrSep - ptr;

      // Allocate memory for the value
      SafeMalloc(
        tgt[iCol],
        len + 1);

      // Copy the value
      memcpy(
        tgt[iCol],
        ptr,
        len);
      tgt[iCol][len] = '\0';

    }

    // Move to the next column
    ++iCol;
    ptr = ptrSep;

    // To escape an empty first column
    if (ptr == csv) ++ptr;

  }

  // Skip the line return
  if (*ptr == '\n') ++ptr;

  // Return the pointer to the end of the row
  return ptr;

}

// Convert CSV data to a new struct RunRecorderMeasures. The CSV data are
// expected to be formatted as:
// Ref&Metric1&Metric2&...
// Ref1&Value1_1&Value1_2&...
// Ref2&Value2_1&Value2_2&...
// ...
// Inputs:
//   csv: the CSV data
//   sep: the separator between columns ('&' in the example above)
// Output:
//   Return a newly allocated RunRecorderMeasures
static struct RunRecorderMeasures* CSVToData(
  char const* const csv,
         char const sep) {

  // Calculate the number of columns by counting the separator in the first
  // row
  long nbCol = 1;
  char const* ptr = csv;
  while (*ptr != '\n' && *ptr != '\0') {

    if (*ptr == sep) ++nbCol;
    ++ptr;

  }

  // Skip the line return of the first line
  if (*ptr == '\n') ++ptr;

  // Calculate the number of measures by counting the number of line return
  // in the remaining rows
  long nbMeasure = 0;
  do {

    if (*ptr == '\n') ++nbMeasure;
    ++ptr;

  } while (*ptr != '\0');

  // Allocate memory for the result struct RunRecorderMeasures
  struct RunRecorderMeasures* measures = RunRecorderMeasuresCreate();
  Try {

    // Set the number of metrics and measure
    measures->nbMetric = nbCol;
    measures->nbMeasure = nbMeasure;

    // Allocate memory for the metrics
    SafeMalloc(
      measures->metrics,
      sizeof(char*) * measures->nbMetric);
    ForZeroTo(iMetric, measures->nbMetric)
      measures->metrics[iMetric] = NULL;

    // Allocate memory for the measures
    SafeMalloc(
      measures->values,
      sizeof(char**) * nbMeasure);
    ForZeroTo(iMeasure, nbMeasure) measures->values[iMeasure] = NULL;

    // Allocate memory for the values
    ForZeroTo(iMeasure, nbMeasure) {

      SafeMalloc(
        measures->values[iMeasure],
        sizeof(char*) * nbCol);
      ForZeroTo(iMetric, measures->nbMetric)
        measures->values[iMeasure][iMetric] = NULL;

    }

    // Extract the metrics label
    ptr =
      SplitCSVRowToData(
        csv,
        measures->metrics,
        sep);

    // Extract the measures
    long iMeasure = 0;
    while (*ptr != '\0') {

      ptr =
        SplitCSVRowToData(
          ptr,
          measures->values[iMeasure],
          sep);
      ++iMeasure;

    }

  } CatchDefault {

    RunRecorderMeasuresFree(&measures);
    Raise(TryCatchGetLastExc());

  } EndTryWithDefault;

  // Return the result struct RunRecorderMeasures
  return measures;

}

// Get the measures of a project through the Web API
// Inputs:
//         that: the struct RunRecorder
//      project: the project's name
// Output:
//   Return the measures as a struct RunRecorderMeasures
static struct RunRecorderMeasures* GetMeasuresAPI(
  struct RunRecorder* const that,
          char const* const project) {

  // Create the request to the Web API
  char* cmdFormat = "action=csv&project=%s";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormat) - 2 + strlen(project) + 1);
  sprintf(
    that->cmd,
    cmdFormat,
    project);
  SetAPIReqPostVal(
    that,
    that->cmd);

  // Send the request to the API
  bool isJsonReq = false;
  SendAPIReq(
    that,
    isJsonReq);

  // Convert the CSV data into a struct RunRecorderMeasures
  struct RunRecorderMeasures* data =
    CSVToData(
      that->curlReply,
      CSV_SEP);

  // Return the struct RunRecorderMeasures
  return data;

}

// Get the most recent measures of a project from a local database
// Inputs:
//        that: the struct RunRecorder
//     project: the project's name
//   nbMeasure: the number of measures to be returned
// Output:
//   Return the measures as a struct RunRecorderMeasures, ordered from the
//   most recent to the oldest
// Raise:
//   RunRecorderExc_SQLRequestFailed
static struct RunRecorderMeasures* GetLastMeasuresLocal(
  struct RunRecorder* const that,
          char const* const project,
                 long const nbMeasure) {

  // Declate the struct RunRecorderMeasures to memorise the measures
  struct RunRecorderMeasures* measures = NULL;

  // Create the request with the requested limit
  SetCmdToGetMeasuresLocal(
    that,
    project,
    nbMeasure);

  // Execute the request
  int retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      GetMeasuresLocalCb,
      &measures,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_SQLRequestFailed);

  // Return the measures
  return measures;

}

// Get the most recent measures of a project through the Web API
// Inputs:
//        that: the struct RunRecorder
//     project: the project's name
//   nbMeasure: the number of measures to be returned
// Output:
//   Return the measures as a struct RunRecorderMeasures, ordered from the
//   most recent to the oldest
static struct RunRecorderMeasures* GetLastMeasuresAPI(
  struct RunRecorder* const that,
          char const* const project,
                 long const nbMeasure) {

  // Get the length of nbMeasure converted as a string
  size_t lenNbMeasureStr =
    snprintf(
      NULL,
      0,
      "%ld",
      nbMeasure);

  // Create the request to the Web API
  char* cmdFormat = "action=csv&project=%s&last=%ld";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormat) - 2 + strlen(project) - 3 + lenNbMeasureStr + 1);
  sprintf(
    that->cmd,
    cmdFormat,
    project,
    nbMeasure);
  SetAPIReqPostVal(
    that,
    that->cmd);

  // Send the request to the API
  bool isJsonReq = false;
  SendAPIReq(
    that,
    isJsonReq);

  // Convert the CSV data into a struct RunRecorderMeasures
  struct RunRecorderMeasures* data =
    CSVToData(
      that->curlReply,
      CSV_SEP);

  // Return the struct RunRecorderMeasures
  return data;

}

// Create a struct RunRecorderMeasures
// Output:
//   Return the dynamically allocated struct RunRecorderMeasures
static struct RunRecorderMeasures* RunRecorderMeasuresCreate(
  void) {

  // Declare the new struct RunRecorderMeasures
  struct RunRecorderMeasures* that = NULL;
  SafeMalloc(
    that,
    sizeof(struct RunRecorderMeasures));

  // Init properties
  that->nbMetric = 0;
  that->nbMeasure = 0;
  that->metrics = NULL;
  that->values = NULL;

  // Return the new struct RunRecorderMeasures
  return that;

}

// Remove a project from a local database
// Inputs:
//         that: the struct RunRecorder
//      project: the project's name
// Raise:
//   RunRecorderExc_FlushProjectFailed
static void FlushProjectLocal(
  struct RunRecorder* const that,
          char const* const project) {

  // Create the SQL command to delete values
  char* cmdFormatValue =
    "DELETE FROM _Value WHERE RefMeasure IN "
    "(SELECT _Measure.Ref FROM _Measure, _Project "
    "WHERE _Measure.RefProject = _Project.Ref "
    "AND _Project.Label = \"%s\")";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormatValue) - 2 + strlen(project) + 1);
  sprintf(
    that->cmd,
    cmdFormatValue,
    project);

  // Execute the command to delete values
  int retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      NULL,
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_FlushProjectFailed);

  // Create the SQL command to delete measures
  char* cmdFormatMeasure =
    "DELETE FROM _Measure WHERE Ref IN "
    "(SELECT _Measure.Ref FROM _Measure, _Project "
    "WHERE _Measure.RefProject = _Project.Ref "
    "AND _Project.Label = \"%s\")";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormatMeasure) - 2 + strlen(project) + 1);
  sprintf(
    that->cmd,
    cmdFormatMeasure,
    project);

  // Execute the command to delete measures
  retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      NULL,
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_FlushProjectFailed);

  // Create the SQL command to delete metrics
  char* cmdFormatMetric =
    "DELETE FROM _Metric WHERE RefProject = "
    "(SELECT Ref FROM _Project "
    "WHERE _Project.Label = \"%s\")";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormatMetric) - 2 + strlen(project) + 1);
  sprintf(
    that->cmd,
    cmdFormatMetric,
    project);

  // Execute the command to delete metrics
  retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      NULL,
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_FlushProjectFailed);

  // Create the SQL command to delete the project
  char* cmdFormatProject =
    "DELETE FROM _Project "
    "WHERE _Project.Label = \"%s\"";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormatProject) - 2 + strlen(project) + 1);
  sprintf(
    that->cmd,
    cmdFormatProject,
    project);

  // Execute the command to delete the project
  retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      NULL,
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_FlushProjectFailed);

}

// Remove a project through the Web API
// Inputs:
//         that: the struct RunRecorder
//      project: the project's name
static void FlushProjectAPI(
  struct RunRecorder* const that,
          char const* const project) {

  // Create the request to the Web API
  char* cmdFormat = "action=flush&project=%s";
  SafeMalloc(
    that->cmd,
    strlen(cmdFormat) - 2 + strlen(project) + 1);
  sprintf(
    that->cmd,
    cmdFormat,
    project);
  SetAPIReqPostVal(
    that,
    that->cmd);

  // Send the request to the API
  bool isJsonReq = true;
  SendAPIReq(
    that,
    isJsonReq);

}

// Function to convert a RunRecorder exception ID to char*
// Input:
//   exc: the exception ID
// Output:
//   Return a pointer to a static string describing the exception, or
//   NULL if the ID is not one of RunRecorderException
static char const* ExcToStr(
  int exc) {

  // If the exception ID is one of RunRecorderException
  if (
    exc >= RunRecorderExc_CreateTableFailed &&
    exc < RunRecorderExc_LastID) {

    // Return the conversion
    return exceptionStr[exc - RunRecorderExc_CreateTableFailed];

  // Else, the exception ID is not one of RunRecorderException
  } else {

    // Return NULL to indicate there is no conversion
    return NULL;

  }

}

// ------------------ runrecorder.c ------------------

