// ------------------ runrecorder.c ------------------

// Include the header
#include "runrecorder.h"

// Last version of the database
#define RUNRECORDER_VERSION_DB "01.00.00"

// Return true if a struct RunRecorder uses the Web API, else false
bool RunRecorderUsesAPI(
  // The struct RunRecorder
  struct RunRecorder const* const that);

// Upgrade the database
void RunRecorderUpgradeDb(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Create the database locally
// Raise: TryCatchException_CreateTableFailed,
// TryCatchException_CurlRequestFailed
void RunRecorderCreateDb(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Create the database locally
// Raise: TryCatchException_CreateTableFailed
void RunRecorderCreateDbLocal(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Callback for RunRecorderGetVersionLocal
static int RunRecorderGetVersionLocalCb(
  // char** to memorise the version
   void* ptrVersion,
  // Number of columns in the returned rows
     int nbCol,
  // Rows values
  char** colVal,
  // Columns name
  char** colName);

// Get the version of the local database
// Return a new string
char* RunRecorderGetVersionLocal(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Get the version of the database via the Web API
// Return a new string
char* RunRecorderGetVersionAPI(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Constructor for a struct RunRecorder
// Raise: TryCatchException_CreateTableFailed,
// TryCatchException_OpenDbFailed,
// TryCatchException_CreateCurlFailed,
// TryCatchException_CurlSetOptFailed,
// TryCatchException_SQLRequestFailed
struct RunRecorder* RunRecorderCreate(
  // Path to the SQLite database or Web API
  char const* const url) {

  // Declare the struct RunRecorder
  struct RunRecorder* that = malloc(sizeof(struct RunRecorder));

  // Duplicate the url
  that->url = strdup(url);

  // Initialise the other properties
  that->errMsg = NULL;
  that->db = NULL;

  // If the recorder doesn't use the API
  if (RunRecorderUsesAPI(that) == false) {

    // Try to open the file in reading mode to check if it exists
    FILE* fp =
      fopen(
        url,
        "r");

    // Open the connection to the local database
    int ret =
      sqlite3_open(
        url,
        &(that->db));
    if (ret != 0) {

      that->errMsg = strdup(sqlite3_errmsg(that->db));
      Raise(TryCatchException_OpenDbFailed);

    }

    // If the SQLite database local file doesn't exists
    if (fp == NULL) {

      // Create the database
      RunRecorderCreateDb(that);

    } else {

      fclose(fp);

    }

  // Else, the recorder uses the API
  } else {

    // Create the curl instance
    curl_global_init(CURL_GLOBAL_ALL);
    that->curl = curl_easy_init();
    if (that->curl != NULL) {

      CURLcode res = curl_easy_setopt(that->curl, CURLOPT_URL, url);
      if (res != CURLE_OK) {

        that->errMsg = strdup(curl_easy_strerror(res));
        Raise(TryCatchException_CurlSetOptFailed);

      }

    } else {

      Raise(TryCatchException_CreateCurlFailed);

    }

  }

  // If the version of the database is different from the last version
  char* version = RunRecorderGetVersion(that);
  int cmpVersion =
    strcmp(
      version,
      RUNRECORDER_VERSION_DB);
  free(version);
  if (cmpVersion != 0) {

    // Upgrade the database
    RunRecorderUpgradeDb(that);

  }

  // Return the struct RunRecorder
  return that;

}

// Destructor for a struct RunRecorder
void RunRecorderFree(
  // The struct RunRecorder to be freed
  struct RunRecorder** const that) {

  // Free memory used by the properties
  free((*that)->url);
  if ((*that)->errMsg != NULL) {

    free((*that)->errMsg);

  }

  // Close the connection to the local database if it was opened
  if ((*that)->db != NULL) {

    sqlite3_close((*that)->db);

  }

  // Clean up the curl instance if it was created
  if ((*that)->curl != NULL) {

    curl_easy_cleanup((*that)->curl);
    curl_global_cleanup();

  }

  // Free memory used by the RunRecorder
  free(*that);
  *that = NULL;

}

// Return true if a struct RunRecorder uses the Web API, else false
bool RunRecorderUsesAPI(
  // The struct RunRecorder
  struct RunRecorder const* const that) {

  // If the url starts with http
  char const* http =
    strstr(
      that->url,
      "http");
  if (http == that->url) {

    return true;

  // Else, the url doesn't start with http
  } else {

    return false;

  }

}

// Create the database
// Raise: TryCatchException_CreateTableFailed,
// TryCatchException_CurlRequestFailed
void RunRecorderCreateDb(
  // The struct RunRecorder
  struct RunRecorder* const that) {

  // If the RunRecorder uses a local database
  if (RunRecorderUsesAPI(that) == false) {

    RunRecorderCreateDbLocal(that);

  }
  // If the RunRecorder uses the Web API there is
  // nothing to do as the remote API will take care of creating the
  // database when necessary

}

// Create the database locally
// Raise: TryCatchException_CreateTableFailed
void RunRecorderCreateDbLocal(
  // The struct RunRecorder
  struct RunRecorder* const that) {

  // List of commands to create the table
  #define RUNRECORDER_NB_TABLE 6
  char* sqlCmd[RUNRECORDER_NB_TABLE] = {

    "CREATE TABLE Version ("
    "  Ref INTEGER PRIMARY KEY,"
    "  Label TEXT NOT NULL)",
    "CREATE TABLE Project ("
    "  Ref INTEGER PRIMARY KEY,"
    "  Label TEXT NOT NULL)",
    "CREATE TABLE Measure ("
    "  Ref INTEGER PRIMARY KEY,"
    "  RefProject INTEGER NOT NULL,"
    "  DateMeasure DATETIME NOT NULL)",
    "CREATE TABLE Value ("
    "  Ref INTEGER PRIMARY KEY,"
    "  RefMeasure INTEGER NOT NULL,"
    "  RefMetric INTEGER NOT NULL,"
    "  Value TEXT NOT NULL)",
    "CREATE TABLE Metric ("
    "  Ref INTEGER PRIMARY KEY,"
    "  RefProject INTEGER NOT NULL,"
    "  Label TEXT NOT NULL,"
    "  DefaultValue TEXT NOT NULL)",
    "INSERT INTO Version (Ref, Label) "
    "VALUES (NULL, '" RUNRECORDER_VERSION_DB "')"

  };

  // Loop on the commands
  for (
    int iCmd = 0;
    iCmd < RUNRECORDER_NB_TABLE;
    ++iCmd) {

    // Ensure errMsg is freed
    if (that->errMsg != NULL) {

      free(that->errMsg);

    }

    // Execute the command to create the table
    int retExec =
      sqlite3_exec(
        that->db,
        sqlCmd[iCmd],
        // No callback
        NULL,
        // No user data
        NULL,
        &(that->errMsg));
    if (retExec != SQLITE_OK) {

      Raise(TryCatchException_CreateTableFailed);

    }

  }

}

// Upgrade the database
void RunRecorderUpgradeDb(
  // The struct RunRecorder
  struct RunRecorder* const that) {

  // Placeholder
  (void)that;

}

// Get the version of the database
// Return a new string
char* RunRecorderGetVersion(
  // The struct RunRecorder
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
static int RunRecorderGetVersionLocalCb(
  // char** to memorise the version
   void* ptrVersion,
  // Number of columns in the returned rows
     int nbCol,
  // Rows values
  char** colVal,
  // Columns name
  char** colName) {

  // Unused argument
  (void)colName;

  // If the arguments are invalid
  if (nbCol != 1 || colVal == NULL || *colVal == NULL) {

    // Return non zero to trigger SQLITE_ABORT in the calling function
    return 1;

  }

  // Memorise the returned value
  *(char**)ptrVersion = strdup(*colVal);

  // Return success code
  return 0;

}

// Get the version of the local database
// Return a new string
// Raise: TryCatchException_SQLRequestFailed
char* RunRecorderGetVersionLocal(
  // The struct RunRecorder
  struct RunRecorder* const that) {

  // Ensure errMsg is freed
  if (that->errMsg != NULL) {

    free(that->errMsg);

  }

  // Declare a variable to memeorise the version
  char* version = NULL;

  // Execute the command to get the version
  char* sqlCmd = "SELECT Label FROM Version LIMIT 1";
  int retExec =
    sqlite3_exec(
      that->db,
      sqlCmd,
      RunRecorderGetVersionLocalCb,
      &version,
      &(that->errMsg));
  if (retExec != SQLITE_OK) {

    Raise(TryCatchException_SQLRequestFailed);

  }

  // Return the version
  return version;

}

// Callback for RunRecorderGetVersionLocal
size_t RunRecorderGetVersionAPICb(
  char* data,
  size_t size,
  size_t nmemb,
  void* version) {

  // Get the size in byte of the received data
  size_t dataSize = size * nmemb;

  // Ensure version is freed
  if (*(char**)version != NULL) {

    free(*(char**)version);

  }

  // Allocate memory to copy the incoming data and the terminating '\0'
  *(char**)version = malloc(dataSize + 1);

  // Copy the incoming data
  memcpy(
    *(char**)version,
    data,
    dataSize);
  (*(char**)version)[dataSize] = '\0';

  // Return the number of byte received;
  return dataSize;

}

// Get the version of the database via the Web API
// Return a new string
// Raise: TryCatchException_CurlSetOptFailed,
// TryCatchException_CurlRequestFailed
char* RunRecorderGetVersionAPI(
  // The struct RunRecorder
  struct RunRecorder* const that) {

  // Ensure errMsg is freed
  if (that->errMsg != NULL) {

    free(that->errMsg);

  }

  // Declare a variable to memeorise the version
  char* version = NULL;

  // Create the request to the Web API
  CURLcode res =
    curl_easy_setopt(
      that->curl,
      CURLOPT_POSTFIELDS,
      " action=version");
  if (res != CURLE_OK) {

    that->errMsg = strdup(curl_easy_strerror(res));
    Raise(TryCatchException_CurlSetOptFailed);

  }

  // Set the callback
  res =
    curl_easy_setopt(
      that->curl,
      CURLOPT_WRITEDATA,
      &version);
  if (res != CURLE_OK) {

    that->errMsg = strdup(curl_easy_strerror(res));
    Raise(TryCatchException_CurlSetOptFailed);

  }
  res =
    curl_easy_setopt(
      that->curl,
      CURLOPT_WRITEFUNCTION,
      RunRecorderGetVersionAPICb);
  if (res != CURLE_OK) {

    that->errMsg = strdup(curl_easy_strerror(res));
    Raise(TryCatchException_CurlSetOptFailed);

  }

  // Send the request to the API
  res = curl_easy_perform(that->curl);
  if (res != CURLE_OK) {

    that->errMsg = strdup(curl_easy_strerror(res));
    Raise(TryCatchException_CurlRequestFailed);

  }

  // Extract the version number from the JSON reply
  // TODO

  // Return the version
  return version;
}

// ------------------ runrecorder.c ------------------

