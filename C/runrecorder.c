// ------------------ runrecorder.c ------------------

// Include the header
#include "runrecorder.h"

// ================== Macros =========================

// Last version of the database
#define RUNRECORDER_VERSION_DB "01.00.00"

// Number of tables in the database
#define RUNRECORDER_NB_TABLE 5

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
// Raise: TryCatchException_CurlSetOptFailed,
// TryCatchException_CurlRequestFailed,
// TryCatchException_ApiRequestFailed
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

// Constructor for a struct RunRecorder
// Input:
//   url: Path to the SQLite database or Web API
// Output:
//  Return a new struct RunRecorder
// Raise: 
//   TryCatchException_CreateTableFailed
//   TryCatchException_OpenDbFailed
//   TryCatchException_CreateCurlFailed
//   TryCatchException_CurlSetOptFailed
//   TryCatchException_SQLRequestFailed
//   TryCatchException_ApiRequestFailed
//   TryCatchException_MallocFailed
struct RunRecorder* RunRecorderCreate(
  char const* const url) {

  // Declare the struct RunRecorder
  struct RunRecorder* that = malloc(sizeof(struct RunRecorder));

  // Duplicate the url
  that->url = strdup(url);

  // Initialise the other properties
  that->errMsg = NULL;
  that->db = NULL;
  that->curlReply = NULL;

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

      (fp != NULL ? fclose(fp) : 0);
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
    if (that->curl == NULL) {

      curl_global_cleanup();
      Raise(TryCatchException_CreateCurlFailed);

    }

    // Set the url of the Web API
    CURLcode res = curl_easy_setopt(that->curl, CURLOPT_URL, url);
    if (res != CURLE_OK) {

      curl_easy_cleanup(that->curl);
      curl_global_cleanup();
      that->errMsg = strdup(curl_easy_strerror(res));
      Raise(TryCatchException_CurlSetOptFailed);

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
      that->errMsg = strdup(curl_easy_strerror(res));
      Raise(TryCatchException_CurlSetOptFailed);

    }
    res =
      curl_easy_setopt(
        that->curl,
        CURLOPT_WRITEFUNCTION,
        RunRecorderGetReplyAPI);
    if (res != CURLE_OK) {

      curl_easy_cleanup(that->curl);
      curl_global_cleanup();
      that->errMsg = strdup(curl_easy_strerror(res));
      Raise(TryCatchException_CurlSetOptFailed);

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
// Input:
//   that: The struct RunRecorder to be freed
void RunRecorderFree(
  struct RunRecorder** const that) {

  // If the struct RunRecorder is not already freed
  if (*that != NULL) {

    // Free memory used by the properties
    free((*that)->url);
    free((*that)->errMsg);

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
//   TryCatchException_CreateTableFailed,
//   TryCatchException_CurlRequestFailed
void RunRecorderCreateDb(
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
// Input:
//   that: The struct RunRecorder
// Raise:
//   TryCatchException_CreateTableFailed
void RunRecorderCreateDbLocal(
  struct RunRecorder* const that) {

  // List of commands to create the table
  char* sqlCmd[RUNRECORDER_NB_TABLE + 1] = {

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
    iCmd < RUNRECORDER_NB_TABLE + 1;
    ++iCmd) {

    // Ensure errMsg is freed
    free(that->errMsg);

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
// Input:
//   that: The struct RunRecorder
void RunRecorderUpgradeDb(
  struct RunRecorder* const that) {

  // Placeholder
  (void)that;

}

// Get the version of the database
// Input:
//   The struct RunRecorder
// Output:
//   Return a new string
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
//   ptrVersion: char** to memorise the version
//        nbCol: Number of columns in the returned rows
//       colVal: Rows values
//      colName: Columns name
// Output:
//   Return 0 if successfull, else 1
static int RunRecorderGetVersionLocalCb(
   void* ptrVersion,
     int nbCol,
  char** colVal,
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
// Input:
//   that: The struct RunRecorder
// Raise:
//   TryCatchException_SQLRequestFailed
// Output:
//   Return a new string
char* RunRecorderGetVersionLocal(
  struct RunRecorder* const that) {

  // Ensure errMsg is freed
  free(that->errMsg);

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
// Input:
//   data: incoming data
//   size: always 1
//   nmemb: number of incoming byte
//   reply: pointer to memory where to store the incoming data
// Output:
//   Return the number of received byte
// Raise:
//   TryCatchException_MallocFailed
size_t RunRecorderGetReplyAPI(
   char* data,
  size_t size,
  size_t nmemb,
   void* reply) {

  // Get the size in byte of the received data
  size_t dataSize = size * nmemb;

  // If there is received data
  if (dataSize != 0) {

    // Variable to memorise the current length of the buffer for the reply
    size_t replyLength = 0;

    // If the current buffer for the reply is not empty
    if (*(char**)reply != NULL) {

      // Get the current length of the reply
      replyLength = strlen(*(char**)reply);

    }

    // Allocate memory for current data, the incoming data and the
    // terminating '\0'
    *(char**)reply =
      realloc(
        *(char**)reply,
        replyLength + dataSize + 1);

    // If the allocation failed
    if (reply == NULL) {

      Raise(TryCatchException_MallocFailed);

    }

    // Copy the incoming data and the end of the current buffer
    memcpy(
      *(char**)reply + replyLength,
      data,
      dataSize);
    (*(char**)reply)[replyLength + dataSize] = '\0';

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
//   TryCatchException_MallocFailed
char* RunRecoderGetJSONValOfKey(
  char const* const json,
  char const* const key) {

  // If the json or key is null or empty
  if (json == NULL || key == NULL || *json == '\0' || *key == '\0') {

    // Nothing to do
    return NULL;

  }

  // Variable to memorise the value
  char* val = NULL;

  // Get the length of the key
  size_t lenKey = strlen(key);

  // Create the key decorated with it's syntax
  char* keyDecorated = malloc(lenKey + 5);
  sprintf(
    keyDecorated,
    "\"%s\":\"",
    key);
  if (keyDecorated == NULL) {

    Raise(TryCatchException_MallocFailed);

  }

  // Search the key in the JSON encoded string
  char const* ptrKey =
    strstr(
      json,
      keyDecorated);
  if (ptrKey != NULL) {

    // Variable to memorise the start of the value
    char const* ptrVal = ptrKey + strlen(keyDecorated);
    // Loop on the characters of the value until the next double quote
    char const* ptr = ptrVal;
    while (ptr != NULL && *ptr != '"') {

      ++ptr;

      // Skip the escaped character
      if (ptr != NULL && *ptr == '\\') {

        ++ptr;

      }

    }

    // If we have found the closing double quote
    if (ptr != NULL) {

      // Get the length of the value
      size_t lenVal = ptr - ptrVal;

      // Allocate memory for the value
      val = malloc(lenVal + 1);
      if (val == NULL) {

        free(keyDecorated);
        Raise(TryCatchException_MallocFailed);

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

// Get the version of the database via the Web API
// Input:
//   The struct RunRecorder
// Output:
//   Return a new string
// Raise:
//   TryCatchException_CurlSetOptFailed,
//   TryCatchException_CurlRequestFailed,
//   TryCatchException_ApiRequestFailed
//   TryCatchException_MallocFailed
char* RunRecorderGetVersionAPI(
  struct RunRecorder* const that) {

  // Ensure errMsg is freed
  free(that->errMsg);

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

  // Send the request to the API
  RunRecorderResetCurlReply(that);
  res = curl_easy_perform(that->curl);
  if (res != CURLE_OK) {

    that->errMsg = strdup(curl_easy_strerror(res));
    Raise(TryCatchException_CurlRequestFailed);

  }

  // Extract the return code from the JSON reply
  char* retCode =
    RunRecoderGetJSONValOfKey(
      that->curlReply,
      "ret");
  if (retCode == NULL) {

    that->errMsg = strdup("'err' missing in API reply");
    Raise(TryCatchException_ApiRequestFailed);

  }

  // If the request failed
  int cmpRet =
    strcmp(
      retCode,
      "0");
  if (cmpRet != 0) {

    that->errMsg =
      RunRecoderGetJSONValOfKey(
        that->curlReply,
        "errMsg");
    Raise(TryCatchException_ApiRequestFailed);

  }

  free(retCode);

  // Extract the version number from the JSON reply
  char* version =
    RunRecoderGetJSONValOfKey(
      that->curlReply,
      "version");

  // Return the version
  return version;

}

// ------------------ runrecorder.c ------------------

