// ------------------ runrecorder.c ------------------

// Include the header
#include "runrecorder.h"

// ================== Macros =========================

// Last version of the database
#define RUNRECORDER_VERSION_DB "01.00.00"

// Number of tables in the database
#define RUNRECORDER_NB_TABLE 5

// Label for the exceptions
char* RunRecorderExceptionStr[RunRecorderExc_LastID] = {
  "",
  "RunRecorderExc_Segv",
  "RunRecorderExc_CreateTableFailed",
  "RunRecorderExc_OpenDbFailed",
  "RunRecorderExc_CreateCurlFailed",
  "RunRecorderExc_CurlRequestFailed",
  "RunRecorderExc_CurlSetOptFailed",
  "RunRecorderExc_SQLRequestFailed",
  "RunRecorderExc_ApiRequestFailed",
  "RunRecorderExc_MallocFailed",
  "RunRecorderExc_InvalidProjectName",
  "RunRecorderExc_ProjectNameAlreadyUsed",
  "RunRecorderExc_AddProjectFailed",
  "RunRecorderExc_MetricNameAlreadyUsed",
  "RunRecorderExc_AddMeasureFailed",
  "RunRecorderExc_InvalidJSON",
};

// ================== Functions declaration =========================

// Return true if a struct RunRecorder uses the Web API, else false
bool RunRecorderUsesAPI(
  // The struct RunRecorder
  struct RunRecorder const* const that);

// Upgrade the database
void RunRecorderUpgradeDb(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Reset the curl reply, to be called before sending a new curl request
// or the result of the new request will be appended to the eventual
// one of the previous request
void RunRecorderResetCurlReply(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Create the database locally
// Raise: RunRecorderExc_CreateTableFailed,
// RunRecorderExc_CurlRequestFailed
void RunRecorderCreateDb(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Create the database locally
// Raise: RunRecorderExc_CreateTableFailed
void RunRecorderCreateDbLocal(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Callback for RunRecorderGetVersionLocal
// Input:
//      data: The version
//     nbCol: Number of columns in the returned rows
//    colVal: Row values
//   colName: Columns name
// Output:
//   Return 0 if successfull, else 1
static int RunRecorderGetVersionLocalCb(
   void* data,
     int nbCol,
  char** colVal,
  char** colName);

// Get the version of the local database
// Return a new string
char* RunRecorderGetVersionLocal(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Get the version of the database via the Web API
// Return a new string
// Raise: 
//   RunRecorderExc_CurlSetOptFailed
//   RunRecorderExc_CurlRequestFailed
//   RunRecorderExc_ApiRequestFailed
char* RunRecorderGetVersionAPI(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Callback for RunRecorderGetVersionLocal
size_t RunRecorderGetReplyAPI(
  char* data,
  size_t size,
  size_t nmemb,
  void* reply);

// ================== Functions definition =========================

// Init a struct RunRecorder using a local SQLite database
// Input:
//   that: the struct RunRecorder
// Raise:
//   RunRecorderExc_OpenDbFailed
void RunRecorderInitLocal(
  struct RunRecorder* const that) {

  // Try to open the file in reading mode to check if it exists
  FILE* fp =
    fopen(
      that->url,
      "r");

  // Open the connection to the local database
  int ret =
    sqlite3_open(
      that->url,
      &(that->db));
  if (ret != 0) {

    if (fp != NULL) fclose(fp);
    free(that->errMsg);
    that->errMsg = strdup(sqlite3_errmsg(that->db));
    Raise(RunRecorderExc_OpenDbFailed);

  }

  // If the SQLite database local file doesn't exists
  if (fp == NULL) {

    // Create the database
    RunRecorderCreateDb(that);

  } else {

    // Close the FILE*
    fclose(fp);

  }

}

// Init a struct RunRecorder using the Web API
// Input:
//   that: the struct RunRecorder
// Raise:
//   RunRecorderExc_CreateCurlFailed
//   RunRecorderExc_CurlSetOptFailed
void RunRecorderInitWebAPI(
  struct RunRecorder* const that) {

  // Create the curl instance
  curl_global_init(CURL_GLOBAL_ALL);
  that->curl = curl_easy_init();
  if (that->curl == NULL) {

    curl_global_cleanup();
    Raise(RunRecorderExc_CreateCurlFailed);

  }

  // Set the url of the Web API
  CURLcode res = curl_easy_setopt(that->curl, CURLOPT_URL, that->url);
  if (res != CURLE_OK) {

    curl_easy_cleanup(that->curl);
    curl_global_cleanup();
    free(that->errMsg);
    that->errMsg = strdup(curl_easy_strerror(res));
    Raise(RunRecorderExc_CurlSetOptFailed);

  }

  // Set the callback to receive data
  res =
    curl_easy_setopt(
      that->curl,
      CURLOPT_WRITEDATA,
      &(that->curlReply));
  if (res != CURLE_OK) {

    curl_easy_cleanup(that->curl);
    curl_global_cleanup();
    free(that->errMsg);
    that->errMsg = strdup(curl_easy_strerror(res));
    Raise(RunRecorderExc_CurlSetOptFailed);

  }
  res =
    curl_easy_setopt(
      that->curl,
      CURLOPT_WRITEFUNCTION,
      RunRecorderGetReplyAPI);
  if (res != CURLE_OK) {

    curl_easy_cleanup(that->curl);
    curl_global_cleanup();
    free(that->errMsg);
    that->errMsg = strdup(curl_easy_strerror(res));
    Raise(RunRecorderExc_CurlSetOptFailed);

  }

}

// Constructor for a struct RunRecorder
// Input:
//   url: Path to the SQLite database or Web API
// Output:
//  Return a new struct RunRecorder
// Raise:
//   RunRecorderExc_MallocFailed
struct RunRecorder* RunRecorderCreate(
  char const* const url) {

  // Declare the struct RunRecorder
  struct RunRecorder* that = malloc(sizeof(struct RunRecorder));

  // Initialise the other properties
  that->errMsg = NULL;
  that->db = NULL;
  that->curl = NULL;
  that->curlReply = NULL;
  that->cmd = NULL;
  that->sqliteErrMsg = NULL;
  that->refLastAddedMeasure = 0;

  // Duplicate the url
  that->url = strdup(url);
  if (that->url == NULL) Raise(RunRecorderExc_MallocFailed);

  // Return the struct RunRecorder
  return that;

}

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
  struct RunRecorder* const that) {

  // If the recorder doesn't use the API
  if (RunRecorderUsesAPI(that) == false) {

    // Init the local connection to the database
    RunRecorderInitLocal(that);

  // Else, the recorder uses the API
  } else {

    // Init the Web API
    RunRecorderInitWebAPI(that);

  }

  // If the version of the database is different from the last version
  // upgrade the database
  char* version = RunRecorderGetVersion(that);
  int cmpVersion =
    strcmp(
      version,
      RUNRECORDER_VERSION_DB);
  free(version);
  if (cmpVersion != 0) RunRecorderUpgradeDb(that);

}

// Destructor for a struct RunRecorder
// Input:
//   that: The struct RunRecorder to be freed
void RunRecorderFree(
  struct RunRecorder** const that) {

  // If the struct RunRecorder is not already freed
  if (*that != NULL) {

    // Free memory used by the properties
    free((*that)->url);
    free((*that)->errMsg);
    sqlite3_free((*that)->sqliteErrMsg);
    free((*that)->curlReply);
    free((*that)->cmd);

    // Close the connection to the local database if it was opened
    if ((*that)->db != NULL) sqlite3_close((*that)->db);

    // Clean up the curl instance if it was created
    if ((*that)->curl != NULL) {

      curl_easy_cleanup((*that)->curl);
      curl_global_cleanup();

    }

    // Free memory used by the RunRecorder
    free(*that);
    *that = NULL;

  }

}

// Check if a struct RunRecorder uses a local SQLite database or the 
// Web API
// Input:
//   that: The struct RunRecorder
// Return:
//   Return true if the struct RunRecorder uses the Web API, else false
bool RunRecorderUsesAPI(
  struct RunRecorder const* const that) {

  // If the url starts with http the struct RunRecorder uses the Web API
  char const* http =
    strstr(
      that->url,
      "http");
  return (http == that->url);

}

// Reset the curl reply, to be called before sending a new curl request
// or the result of the new request will be appended to the eventual
// one of the previous request
// Input:
//   The struct RunRecorder
void RunRecorderResetCurlReply(
  struct RunRecorder* const that) {

  free(that->curlReply);
  that->curlReply = NULL;

}

// Create the database
// Input:
//   that: The struct RunRecorder
// Raise:
//   RunRecorderExc_CreateTableFailed
//   RunRecorderExc_CurlRequestFailed
void RunRecorderCreateDb(
  struct RunRecorder* const that) {

  // If the RunRecorder uses a local database
  if (RunRecorderUsesAPI(that) == false) RunRecorderCreateDbLocal(that);

  // If the RunRecorder uses the Web API there is
  // nothing to do as the remote API will take care of creating the
  // database when necessary

}

// Create the database locally
// Input:
//   that: The struct RunRecorder
// Raise:
//   RunRecorderExc_CreateTableFailed
void RunRecorderCreateDbLocal(
  struct RunRecorder* const that) {

  // List of commands to create the table
  char* sqlCmd[RUNRECORDER_NB_TABLE + 1] = {

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
    "VALUES (NULL, '" RUNRECORDER_VERSION_DB "')"

  };

  // Loop on the commands
  for (
    int iCmd = 0;
    iCmd < RUNRECORDER_NB_TABLE + 1;
    ++iCmd) {

    // Ensure errMsg is freed
    sqlite3_free(that->sqliteErrMsg);

    // Execute the command to create the table
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
void RunRecorderUpgradeDb(
  struct RunRecorder* const that) {

  // Placeholder
  (void)that;

}

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
  struct RunRecorder* const that) {

  // If the RunRecorder uses a local database
  if (RunRecorderUsesAPI(that) == false) {

    return RunRecorderGetVersionLocal(that);

  // Else, the RunRecorder uses the Web API
  } else {

    return RunRecorderGetVersionAPI(that);

  }

}

// Callback for RunRecorderGetVersionLocal
// Input:
//      data: The version
//     nbCol: Number of columns in the returned rows
//    colVal: Row values
//   colName: Columns name
// Output:
//   Return 0 if successfull, else 1
static int RunRecorderGetVersionLocalCb(
   void* data,
     int nbCol,
  char** colVal,
  char** colName) {

  // Unused argument
  (void)colName;

  // Cast the data
  char** ptrVersion = (char**)data;

  // If the arguments are invalid
  // Return non zero to trigger SQLITE_ABORT in the calling function
  if (nbCol != 1 || colVal == NULL || *colVal == NULL) return 1;

  // Memorise the returned value
  *ptrVersion = strdup(*colVal);
  if (*ptrVersion == NULL) return 1;

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
char* RunRecorderGetVersionLocal(
  struct RunRecorder* const that) {

  // Ensure errMsg is freed
  sqlite3_free(that->sqliteErrMsg);

  // Declare a variable to memeorise the version
  char* version = NULL;

  // Execute the command to get the version
  char* sqlCmd = "SELECT Label FROM _Version LIMIT 1";
  int retExec =
    sqlite3_exec(
      that->db,
      sqlCmd,
      RunRecorderGetVersionLocalCb,
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
//   reply: pointer to memory where to store the incoming data
// Output:
//   Return the number of received byte
// Raise:
//   RunRecorderExc_MallocFailed
size_t RunRecorderGetReplyAPI(
   char* data,
  size_t size,
  size_t nmemb,
   void* ptr) {

  // Get the size in byte of the received data
  size_t dataSize = size * nmemb;

  // Cast
  char** reply = (char**)ptr;

  // If there is received data
  if (dataSize != 0) {

    // Variable to memorise the current length of the buffer for the reply
    size_t replyLength = 0;

    // If the current buffer for the reply is not empty
    // Get the current length of the reply
    if (*reply != NULL) replyLength = strlen(*reply);

    // Allocate memory for current data, the incoming data and the
    // terminating '\0'
    *reply =
      realloc(
        *reply,
        replyLength + dataSize + 1);

    // If the allocation failed
    if (reply == NULL) Raise(RunRecorderExc_MallocFailed);

    // Copy the incoming data and the end of the current buffer
    memcpy(
      *reply + replyLength,
      data,
      dataSize);
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
//   RunRecorderExc_MallocFailed
char* RunRecoderGetJSONValOfKey(
  char const* const json,
  char const* const key) {

  // If the json or key is null or empty
  // Nothing to do
  if (json == NULL || key == NULL ||
      *json == '\0' || *key == '\0') return NULL;

  // Variable to memorise the value
  char* val = NULL;

  // Get the length of the key
  size_t lenKey = strlen(key);

  // Create the key decorated with it's syntax
  char* keyDecorated = malloc(lenKey + 4);
  sprintf(
    keyDecorated,
    "\"%s\":",
    key);
  if (keyDecorated == NULL) Raise(RunRecorderExc_MallocFailed);

  // Search the key in the JSON encoded string
  char const* ptrKey =
    strstr(
      json,
      keyDecorated);
  if (ptrKey != NULL) {

    // Variable to memorise the start of the value
    char const* ptrVal = ptrKey + strlen(keyDecorated) + 1;

    // Declare a pointer to loop on the string until the end of the value
    char const* ptr = ptrVal;
    if (ptrKey[strlen(keyDecorated)] == '"') {

      // Loop on the characters of the value until the next double quote
      while (*ptr != '\0' && *ptr != '"') {

        ++ptr;

        // Skip the escaped character
        if (*ptr != '\0' && *ptr == '\\') ++ptr;

      }

    } else if (ptrKey[strlen(keyDecorated)] == '{') {

      // Loop on the characters of the value until the closing curly brace
      while (*ptr != '\0' && *ptr != '}') ++ptr;

    }

    // If we have found the end of the value
    if (*ptr != '\0') {

      // Get the length of the value
      size_t lenVal = ptr - ptrVal;

      // Allocate memory for the value
      val = malloc(lenVal + 1);
      if (val == NULL) {

        free(keyDecorated);
        Raise(RunRecorderExc_MallocFailed);

      }

      // Copy the value
      memcpy(
        val,
        ptrVal,
        lenVal);
      val[lenVal] = '\0';

    }

  }

  // Free memory
  free(keyDecorated);

  // Return the value
  return val;

}

// Set the POST data in the Web API request of a struct RunRecorder
// Input:
//   that: the struct RunRecorder
//   data: the POST data
// Raise:
//   RunRecorderExc_CurlSetOptFailed
void RunRecorderSetAPIReqPostVal(
  struct RunRecorder* const that,
  char const* const data) {

  // Set the data in the Curl fields
  CURLcode res =
    curl_easy_setopt(
      that->curl,
      CURLOPT_POSTFIELDS,
      data);
  if (res != CURLE_OK) {

    free(that->errMsg);
    that->errMsg = strdup(curl_easy_strerror(res));
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
char* RunRecorderGetAPIRetCode(
  struct RunRecorder* const that) {

  // Extract the return code from the JSON reply
  char* retCode =
    RunRecoderGetJSONValOfKey(
      that->curlReply,
      "ret");
  if (retCode == NULL) {

    free(that->errMsg);
    that->errMsg = strdup("'ret' key missing in API reply");
    Raise(RunRecorderExc_ApiRequestFailed);

  }

  // Return the code
  return retCode;

}

// Send the current request of a struct RunRecorder
// Input:
//   that: the struct RunRecorder
// Raise:
//   RunRecorderExc_CurlRequestFailed
void RunRecorderSendAPIReq(
  struct RunRecorder* const that) {

  // Send the request
  RunRecorderResetCurlReply(that);
  CURLcode res = curl_easy_perform(that->curl);
  if (res != CURLE_OK) {

    free(that->errMsg);
    that->errMsg = strdup(curl_easy_strerror(res));
    Raise(RunRecorderExc_CurlRequestFailed);

  }

  // Check the return code from the JSON reply
  char* retCode = RunRecorderGetAPIRetCode(that);
  int cmpRet =
    strcmp(
      retCode,
      "0");
  free(retCode);
  if (cmpRet != 0) {

    free(that->errMsg);
    that->errMsg =
      RunRecoderGetJSONValOfKey(
        that->curlReply,
        "errMsg");
    Raise(RunRecorderExc_ApiRequestFailed);

  }

}

// Get the version of the database via the Web API
// Input:
//   The struct RunRecorder
// Output:
//   Return a new string
// Raise:
//   RunRecorderExc_CurlSetOptFailed,
//   RunRecorderExc_CurlRequestFailed
//   RunRecorderExc_ApiRequestFailed
//   RunRecorderExc_MallocFailed
char* RunRecorderGetVersionAPI(
  struct RunRecorder* const that) {

  // Create the request to the Web API
  RunRecorderSetAPIReqPostVal(
    that,
    "action=version");

  // Send the request to the API
  RunRecorderSendAPIReq(that);

  // Extract the version number from the JSON reply
  char* version =
    RunRecoderGetJSONValOfKey(
      that->curlReply,
      "version");

  // Return the version
  return version;

}

// Add a new projet in a local database
// Input:
//   that: the struct RunRecorder
//   name: the name of the new project
// The project's name must respect the following pattern: 
// /^[a-zA-Z][a-zA-Z0-9_]*$/ .
// Raise:
//   RunRecorderExc_SQLRequestFailed
//   RunRecorderExc_MallocFailed
void RunRecorderAddProjectLocal(
  struct RunRecorder* const that,
  char const* const name) {

  // Ensure errMsg is freed
  sqlite3_free(that->sqliteErrMsg);

  // Create the SQL command
  char* cmdBase =
    "INSERT INTO _Project (Ref, Label) VALUES (NULL, \"%s\")";
  free(that->cmd);
  that->cmd = malloc(strlen(cmdBase) + strlen(name) - 1);
  if (that->cmd == NULL) Raise(RunRecorderExc_MallocFailed);
  sprintf(
    that->cmd,
    cmdBase,
    name);

  // Execute the command to add the project
  int retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      // No callback
      NULL,
      // No user data
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_AddProjectFailed);

}

// Add a new projet in a remote database
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
//   RunRecorderExc_AddProjectFailed
void RunRecorderAddProjectAPI(
  struct RunRecorder* const that,
  char const* const name) {

  // Create the request to the Web API
  char* cmdBase = "action=add_project&label=";
  free(that->cmd);
  that->cmd = malloc(strlen(cmdBase) + strlen(name) + 1);
  if (that->cmd == NULL) Raise(RunRecorderExc_MallocFailed);
  sprintf(
    that->cmd,
    "%s%s",
    cmdBase,
    name);
  RunRecorderSetAPIReqPostVal(
    that,
    that->cmd);

  // Send the request to the API
  RunRecorderSendAPIReq(that);

}

// Check if a string respect the pattern /[a-zA-Z][a-zA-Z0-9_]*/
// Input:
//   str: the string to check
// Output:
//   Return true if the string respect the pattern, else false
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

// Check if a value is present in pairs ref/val
// Inputs:
//   that: the struct RunRecorderPairsRefVal
//    val: the value to check
// Output:
//   Return true if the value is present in the pairs, else false
bool RunRecorderPairsRefValContainsVal(
  struct RunRecorderPairsRefVal const* const that,
                           char const* const val) {

  // Loop on the pairs
  for (
    long iPair = 0;
    iPair < that->nb;
    ++iPair) {

    // If the pair's value is the checked value
    int retCmp =
      strcmp(
        val,
        that->vals[iPair]);
    if (retCmp == 0) {

      // Return true
      return true;

    }

  }

  // If we reach here, we haven't found the checked value, return false
  return false;

}

// Add a new project
// Input:
//   that: the struct RunRecorder
//   name: the name of the new project
// The project's name must respect the following pattern: 
// /^[a-zA-Z][a-zA-Z0-9_]*$/ .
// Output:
//   Return the reference of the new project
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
  char const* const name) {

  // Check the name
  bool isValidName = RunRecorderIsValidLabel(name);
  if (isValidName == false) Raise(RunRecorderExc_InvalidProjectName);

  // Check if there is no other metric with same name for this project
  struct RunRecorderPairsRefVal* projects = RunRecorderGetProjects(that);
  bool alreadyUsed =
    RunRecorderPairsRefValContainsVal(
      projects,
      name);
  RunRecorderPairsRefValFree(&projects);
  if (alreadyUsed == true) {

    Raise(RunRecorderExc_ProjectNameAlreadyUsed);

  }

  // If the RunRecorder uses a local database
  if (RunRecorderUsesAPI(that) == false) {

    RunRecorderAddProjectLocal(
      that,
      name);

  // Else, the RunRecorder uses the Web API
  } else {

    RunRecorderAddProjectAPI(
      that,
      name);

  }

}

// Add a new pair in a struct RunRecorderPairsRefVal
// Inputs:
//   pairs: the struct RunRecorderPairsRefVal
//     ref: the reference of the pair
//     val: the val of the pair
// Raise:
//   RunRecorderExc_MallocFailed
void RunRecorderPairsRefValAdd(
  struct RunRecorderPairsRefVal* const pairs,
                            long const ref,
                     char const* const val) {

  // Allocate memory
  long* refs =
    realloc(
      pairs->refs,
      sizeof(long) * (pairs->nb + 1));
  if (refs == NULL) Raise(RunRecorderExc_MallocFailed);
  char** vals =
    realloc(
      pairs->vals,
      sizeof(char*) * (pairs->nb + 1));
  if (vals == NULL) {

    free(refs);
    Raise(RunRecorderExc_MallocFailed);

  }

  pairs->refs = refs;
  pairs->vals = vals;
  pairs->vals[pairs->nb] = NULL;

  // Update the number of pairs
  ++(pairs->nb);

  // Set the reference and value of the pair
  pairs->refs[pairs->nb - 1] = ref;
  pairs->vals[pairs->nb - 1] = strdup(val);
  if (pairs->vals[pairs->nb - 1] == NULL) Raise(RunRecorderExc_MallocFailed);

}

// Callback to receive pairs of ref/value from sqlite3
// Input:
//      data: The pairs
//     nbCol: Number of columns in the returned rows
//    colVal: Row values
//   colName: Columns name
// Output:
//   Return 0 if successfull, else 1
// Raise:
//   RunRecorderExc_MallocFailed
static int RunRecorderGetPairsLocalCb(
   void* data,
     int nbCol,
  char** colVal,
  char** colName) {

  // Unused argument
  (void)colName;

  // Cast the data
  struct RunRecorderPairsRefVal* pairs =
    (struct RunRecorderPairsRefVal*)data;

  // If the arguments are invalid
  // Return non zero to trigger SQLITE_ABORT in the calling function
  if (nbCol != 2 || colVal == NULL ||
      colVal[0] == NULL || colVal[1] == NULL) return 1;

  // Add the pair to the pairs
  errno = 0;
  long ref =
    strtol(
      colVal[0],
      NULL,
      10);
  if (errno != 0) return 1;
  Try {

    RunRecorderPairsRefValAdd(
      pairs,
      ref,
      colVal[1]);

  } Catch(RunRecorderExc_MallocFailed) {

    return 1;

  } EndTry;

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
//   RunRecorderExc_MallocFailed
struct RunRecorderPairsRefVal* RunRecorderGetProjectsLocal(
  struct RunRecorder* const that) {

  // Ensure errMsg is freed
  sqlite3_free(that->sqliteErrMsg);

  // Declare a variable to memorise the projects
  struct RunRecorderPairsRefVal* projects = RunRecorderPairsRefValCreate();

  // Execute the command to get the version
  char* sqlCmd = "SELECT Ref, Label FROM _Project";
  int retExec =
    sqlite3_exec(
      that->db,
      sqlCmd,
      RunRecorderGetPairsLocalCb,
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
//   json: the JSON string, such as "1":"A","2":"B"
// Output:
//   Return a new struct RunRecorderPairsRefVal
// Raise:
//   RunRecorderExc_MallocFailed
//   RunRecorderExc_InvalidJSON
struct RunRecorderPairsRefVal* RunRecorderGetPairsRefValFromJSON(
  char const* json) {

  // Create the pairs
  struct RunRecorderPairsRefVal* pairs = RunRecorderPairsRefValCreate();

  // Declare a pointer to loop on the json string
  char const* ptr = json;

  // Loop until the end of the json string
  while (*ptr != '\0') {

    // Go to the next double quote
    while (*ptr != '\0' && *ptr != '"') ++ptr;

    if (*ptr != '\0') {

      // Skip the opening double quote for the reference
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
      if (*ptrEndVal == '\0' ||
          ptrEndVal == ptr) Raise(RunRecorderExc_InvalidJSON);

      // Get the value
      size_t lenVal = ptrEndVal - ptr;
      char* val = malloc(lenVal + 1);
      if (val == NULL) Raise(RunRecorderExc_MallocFailed);
      memcpy(
        val,
        ptr,
        lenVal);
      val[lenVal] = '\0';

      // Move to the character after the closing double quote of the value
      ptr = ptrEndVal + 1;

      // Add the pair
      Try {

        RunRecorderPairsRefValAdd(
          pairs,
          ref,
          val);

      } Catch (RunRecorderExc_MallocFailed) {

        free(val);
        Raise(TryCatchGetLastExc());

      } EndTry;

      free(val);

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
// Raise:
//   RunRecorderExc_CurlSetOptFailed
//   RunRecorderExc_CurlRequestFailed
//   RunRecorderExc_MallocFailed
//   RunRecorderExc_ApiRequestFailed
//   RunRecorderExc_InvalidJSON
struct RunRecorderPairsRefVal* RunRecorderGetProjectsAPI(
  struct RunRecorder* const that) {

  // Create the request to the Web API
  RunRecorderSetAPIReqPostVal(
    that,
    "action=projects");

  // Send the request to the API
  RunRecorderSendAPIReq(that);

  // Get the projects list in the JSON reply
  char* json =
    RunRecoderGetJSONValOfKey(
      that->curlReply,
      "projects");
  if (json == NULL) Raise(RunRecorderExc_ApiRequestFailed);

  // Extract the projects
  struct RunRecorderPairsRefVal* projects = NULL;
  Try {

    projects = RunRecorderGetPairsRefValFromJSON(json);

  } Catch(RunRecorderExc_MallocFailed) {

    free(json);
    Raise(TryCatchGetLastExc());
    
  } EndTry;

  // Free memory
  free(json);

  // Return the projects
  return projects;

}

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
  struct RunRecorder* const that) {

  // If the RunRecorder uses a local database
  if (RunRecorderUsesAPI(that) == false) {

    return RunRecorderGetProjectsLocal(that);

  // Else, the RunRecorder uses the Web API
  } else {

    return RunRecorderGetProjectsAPI(that);

  }

}

// Create a static struct RunRecorderPairsRefVal
// Output:
//   Return the new struct RunRecorderPairsRefVal
// Raise:
//   RunRecorderExc_MallocFailed
struct RunRecorderPairsRefVal* RunRecorderPairsRefValCreate(
  void) {

  // Declare the new struct RunRecorderPairsRefVal
  struct RunRecorderPairsRefVal* pairs =
    malloc(sizeof(struct RunRecorderPairsRefVal));
  if (pairs == NULL) Raise(RunRecorderExc_MallocFailed);

  // Initialise properties
  pairs->nb = 0;
  pairs->refs = NULL;
  pairs->vals = NULL;

  // Return the new struct RunRecorderPairsRefVal
  return pairs;

}

// Free a static struct RunRecorderPairsRefVal
// Input:
//   that: the struct RunRecorderPairsRefVal
void RunRecorderPairsRefValFree(
  struct RunRecorderPairsRefVal** that) {

  // If the struct is not already freed
  if (that != NULL && *that != NULL) {

    if ((*that)->vals != NULL) {

      // Loop on the pairs
      for (
        long iPair = 0;
        iPair < (*that)->nb;
        ++iPair) {

        // Free the value
        free((*that)->vals[iPair]);

      }

    }

    // Free memory
    free((*that)->vals);
    free((*that)->refs);
    free(*that);
    *that = NULL;

  }

}

// Get the list of metrics for a project from a local database
// Input:
//      that: the struct RunRecorder
//   project: the project
// Output:
//   Return the metrics' reference/label
// Raise:
//   RunRecorderExc_SQLRequestFailed
//   RunRecorderExc_MallocFailed
struct RunRecorderPairsRefVal* RunRecorderGetMetricsLocal(
  struct RunRecorder* const that,
          char const* const project) {

  // Create the request
  char* cmdBase =
    "SELECT _Metric.Ref, _Metric.Label FROM _Metric, _Project "
    "WHERE _Metric.RefProject = _Project.Ref AND "
    "_Project.Label = \"%s\"";
  free(that->cmd);
  that->cmd = malloc(strlen(cmdBase) + strlen(project) + 1);
  if (that->cmd == NULL) Raise(RunRecorderExc_MallocFailed);
  sprintf(
    that->cmd,
    cmdBase,
    project);

  // Declare a struct RunRecorderPairsRefVal to memorise the metrics
  struct RunRecorderPairsRefVal* metrics = RunRecorderPairsRefValCreate();

  // Execute the request
  int retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      RunRecorderGetPairsLocalCb,
      metrics,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) {

    RunRecorderPairsRefValFree(&metrics);
    Raise(RunRecorderExc_SQLRequestFailed);

  }

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
//   RunRecorderExc_CurlSetOptFailed
//   RunRecorderExc_CurlRequestFailed
//   RunRecorderExc_MallocFailed
//   RunRecorderExc_ApiRequestFailed
//   RunRecorderExc_InvalidJSON
struct RunRecorderPairsRefVal* RunRecorderGetMetricsAPI(
  struct RunRecorder* const that,
          char const* const project) {

  // Create the request to the Web API
  // Create the request
  char* cmdBase = "action=metrics&project=%s";
  free(that->cmd);
  that->cmd = malloc(strlen(cmdBase) + strlen(project) + 1);
  if (that->cmd == NULL) Raise(RunRecorderExc_MallocFailed);
  sprintf(
    that->cmd,
    cmdBase,
    project);
  RunRecorderSetAPIReqPostVal(
    that,
    that->cmd);

  // Send the request to the API
  RunRecorderSendAPIReq(that);

  // Get the projects list in the JSON reply
  char* json =
    RunRecoderGetJSONValOfKey(
      that->curlReply,
      "metrics");
  if (json == NULL) Raise(RunRecorderExc_ApiRequestFailed);
  // Extract the metrics
  struct RunRecorderPairsRefVal* metrics = NULL;
  Try {

    metrics = RunRecorderGetPairsRefValFromJSON(json);

  } Catch(RunRecorderExc_MallocFailed) {

    free(json);
    Raise(TryCatchGetLastExc());
    
  } EndTry;

  // Free memory
  free(json);

  // Return the metrics
  return metrics;

}

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
          char const* const project) {

  // If the RunRecorder uses a local database
  if (RunRecorderUsesAPI(that) == false) {

    return RunRecorderGetMetricsLocal(
      that,
      project);

  // Else, the RunRecorder uses the Web API
  } else {

    return RunRecorderGetMetricsAPI(
      that,
      project);

  }

}

// Add a metric to a project to a local database
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
void RunRecorderAddMetricLocal(
  struct RunRecorder* const that,
          char const* const project,
          char const* const label,
          char const* const defaultVal) {

  // Ensure errMsg is freed
  sqlite3_free(that->sqliteErrMsg);

  // Create the SQL command
  char* cmdBase =
    "INSERT INTO _Metric (Ref, RefProject, Label, DefaultValue) "
    "SELECT NULL, _Project.Ref, \"%s\", \"%s\" FROM _Project "
    "WHERE _Project.Label = \"%s\"";
  free(that->cmd);
  that->cmd = malloc(
    strlen(cmdBase) + strlen(label) +
    strlen(defaultVal) + strlen(project) + 1);
  if (that->cmd == NULL) Raise(RunRecorderExc_MallocFailed);
  sprintf(
    that->cmd,
    cmdBase,
    label,
    defaultVal,
    project);

  // Execute the command to add the metric
  int retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      // No callback
      NULL,
      // No user data
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_AddProjectFailed);

}

// Add a metric to a project through the Web API
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

void RunRecorderAddMetricAPI(
  struct RunRecorder* const that,
          char const* const project,
          char const* const label,
          char const* const defaultVal) {

  // Create the request to the Web API
  char* cmdBase = "action=add_metric&project=%s&label=%s&default=%s";
  free(that->cmd);
  that->cmd = malloc(
    strlen(cmdBase) + strlen(label) + strlen(project) +
    strlen(defaultVal) + 1);
  if (that->cmd == NULL) Raise(RunRecorderExc_MallocFailed);
  sprintf(
    that->cmd,
    cmdBase,
    project,
    label,
    defaultVal);
  RunRecorderSetAPIReqPostVal(
    that,
    that->cmd);

  // Send the request to the API
  RunRecorderSendAPIReq(that);

}

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
          char const* const defaultVal) {

  // Check the label
  bool isValidLabel = RunRecorderIsValidLabel(label);
  if (isValidLabel == false) Raise(RunRecorderExc_InvalidMetricName);

  // Check if there is no other metric with same name for this project
  struct RunRecorderPairsRefVal* metrics =
    RunRecorderGetMetrics(
      that,
      project);
  bool alreadyUsed =
    RunRecorderPairsRefValContainsVal(
      metrics,
      label);
  RunRecorderPairsRefValFree(&metrics);
  if (alreadyUsed == true) {

    Raise(RunRecorderExc_MetricNameAlreadyUsed);

  }

  // If the RunRecorder uses a local database
  if (RunRecorderUsesAPI(that) == false) {

    return
      RunRecorderAddMetricLocal(
        that,
        project,
        label,
        defaultVal);

  // Else, the RunRecorder uses the Web API
  } else {

    return
      RunRecorderAddMetricAPI(
        that,
        project,
        label,
        defaultVal);

  }

}

// Create a struct RunRecorderMeasure
// Output:
//   Return the new struct RunRecorderMeasure
// Raise:
//   RunRecorderExc_MallocFailed
struct RunRecorderMeasure* RunRecorderMeasureCreate(
  void) {

  // Declare the new struct RunRecorderMeasure
  struct RunRecorderMeasure* measure =
    malloc(sizeof(struct RunRecorderMeasure));
  if (measure == NULL) Raise(RunRecorderExc_MallocFailed);

  // Initialise properties
  measure->nbVal = 0;
  measure->metrics = NULL;
  measure->vals = NULL;

  // Return the new struct RunRecorderMeasure
  return measure;

}

// Free a struct RunRecorderMeasure
// Input:
//   that: the struct RunRecorderMeasure
void RunRecorderMeasureFree(
  struct RunRecorderMeasure** that) {

  // If the struct is not already freed
  if (that != NULL && *that != NULL) {

    if ((*that)->metrics != NULL) {

      // Loop on the measure
      for (
        long iVal = 0;
        iVal < (*that)->nbVal;
        ++iVal) {

        // Free the value
        free((*that)->metrics[iVal]);

      }

    }

    if ((*that)->vals != NULL) {

      // Loop on the measure
      for (
        long iVal = 0;
        iVal < (*that)->nbVal;
        ++iVal) {

        // Free the value
        free((*that)->vals[iVal]);

      }

    }

    // Free memory
    free((*that)->metrics);
    free((*that)->vals);
    free(*that);
    *that = NULL;

  }

}

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
           char const* const val) {

  // Allocate memory
  char** metrics =
    realloc(
      that->metrics,
      sizeof(char*) * (that->nbVal + 1));
  if (metrics == NULL) Raise(RunRecorderExc_MallocFailed);
  char** vals =
    realloc(
      that->vals,
      sizeof(char*) * (that->nbVal + 1));
  if (vals == NULL) {

    free(metrics);
    Raise(RunRecorderExc_MallocFailed);

  }

  that->metrics = metrics;
  that->vals = vals;
  that->metrics[that->nbVal] = NULL;
  that->vals[that->nbVal] = NULL;

  // Update the number of values
  ++(that->nbVal);

  // Set the reference and value of the measure
  that->metrics[that->nbVal - 1] = strdup(metric);
  if (that->metrics[that->nbVal - 1] == NULL) Raise(RunRecorderExc_MallocFailed);
  that->vals[that->nbVal - 1] = strdup(val);
  if (that->vals[that->nbVal - 1] == NULL) Raise(RunRecorderExc_MallocFailed);

}

void RunRecorderMeasureAddValueInt(
  struct RunRecorderMeasure* that,
           char const* const metric,
                  long const val) {

  // Convert the value to a string
  int lenStr = snprintf(NULL, 0, "%ld", val);
  char* str = malloc(lenStr + 1);
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

    } Catch (RunRecorderExc_MallocFailed) {

      free(str);
      Raise(TryCatchGetLastExc());

    } EndTry;

  // Free memory
  free(str);

}

void RunRecorderMeasureAddValueFloat(
  struct RunRecorderMeasure* that,
           char const* const metric,
                double const val) {

  // Convert the value to a string
  int lenStr = snprintf(NULL, 0, "%lf", val);
  char* str = malloc(lenStr + 1);
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

    } Catch (RunRecorderExc_MallocFailed) {

      free(str);
      Raise(TryCatchGetLastExc());

    } EndTry;

  // Free memory
  free(str);

}

// Add a measure to a project in a local database
// Inputs:
//         that: the struct RunRecorder
//      project: the project to add the measure to
//      measure: the measure to add
// Raise:
//   RunRecorderExc_AddMeasureFailed
//   RunRecorderExc_MallocFailed
void RunRecorderAddMeasureLocal(
               struct RunRecorder* const that,
                       char const* const project,
  struct RunRecorderMeasure const* const measure) {

  // Ensure errMsg is freed
  sqlite3_free(that->sqliteErrMsg);

  // Get the date of the record (use the current local date)
  time_t mytime = time(NULL);
  char* dateStr = ctime(&mytime);
  dateStr[strlen(dateStr)-1] = '\0';

  // Create the SQL command
  char* cmdBase =
    "INSERT INTO _Measure (RefProject, DateMeasure) "
    "SELECT _Project.Ref, \"%s\" FROM _Project "
    "WHERE _Project.Label = \"%s\"";
  free(that->cmd);
  that->cmd = malloc(
    strlen(cmdBase) + strlen(dateStr) + strlen(project) + 1);
  if (that->cmd == NULL) Raise(RunRecorderExc_MallocFailed);
  sprintf(
    that->cmd,
    cmdBase,
    dateStr,
    project);

  // Execute the command to add the measure
  int retExec =
    sqlite3_exec(
      that->db,
      that->cmd,
      // No callback
      NULL,
      // No user data
      NULL,
      &(that->sqliteErrMsg));
  if (retExec != SQLITE_OK) Raise(RunRecorderExc_AddMeasureFailed);

  // Get the reference of the measure
  that->refLastAddedMeasure = sqlite3_last_insert_rowid(that->db);
  int lenRefMeasureStr =
    snprintf(
    NULL,
    0,
    "%lld",
    that->refLastAddedMeasure);
  char* refMeasureStr = malloc(lenRefMeasureStr + 1);
  if (refMeasureStr == NULL) Raise(RunRecorderExc_MallocFailed);
  sprintf(
    refMeasureStr,
    "%lld",
    that->refLastAddedMeasure);

  // Declare a variable to memorise an eventual failure
  // The policy here is to try to save has much data has possible
  // even if some fails, inform the user and let him/her take
  // appropriate action
  bool hasFailed = false;

  // Loop on the values in the measure
  for (
    long iVal = 0;
    iVal < measure->nbVal;
    ++iVal) {

    // Create the SQL command
    char* cmdValBase =
      "INSERT INTO _Value (RefMeasure, RefMetric, Value) "
      "SELECT \"%s\", _Metric.Ref, \"%s\" FROM _Metric "
      "WHERE _Metric.Label = \"%s\"";
    free(that->cmd);
    that->cmd = malloc(
      strlen(cmdValBase) + strlen(refMeasureStr) +
      strlen(measure->vals[iVal]) + strlen(measure->metrics[iVal]) + 1);
    if (that->cmd == NULL) {

      hasFailed = true;

    } else {

      sprintf(
        that->cmd,
        cmdBase,
        refMeasureStr,
        measure->vals[iVal],
        measure->metrics[iVal]);

      // Execute the command to add the value
      int retExec =
        sqlite3_exec(
          that->db,
          that->cmd,
          // No callback
          NULL,
          // No user data
          NULL,
          &(that->sqliteErrMsg));
      if (retExec != SQLITE_OK) hasFailed = true;

    }

  }

  // Free memory
  free(refMeasureStr);

  // If there has been a failure
  if (hasFailed == true) Raise(RunRecorderExc_AddMeasureFailed);

}

// Add a measure to a project through the WebAPI
// Inputs:
//         that: the struct RunRecorder
//      project: the project to add the measure to
//      measure: the measure to add
// Raise:

void RunRecorderAddMeasureAPI(
               struct RunRecorder* const that,
                       char const* const project,
  struct RunRecorderMeasure const* const measure) {

}

// Add a measure to a project
// Inputs:
//         that: the struct RunRecorder
//      project: the project to add the measure to
//      measure: the measure to add
// Raise:

void RunRecorderAddMeasure(
               struct RunRecorder* const that,
                       char const* const project,
  struct RunRecorderMeasure const* const measure) {

  // Reset the reference of the last added measure
  that->refLastAddedMeasure = 0;

  // If the RunRecorder uses a local database
  if (RunRecorderUsesAPI(that) == false) {

    RunRecorderAddMeasureLocal(
      that,
      project,
      measure);

  // Else, the RunRecorder uses the Web API
  } else {

    RunRecorderAddMeasureAPI(
      that,
      project,
      measure);

  }

}

// ------------------ runrecorder.c ------------------

