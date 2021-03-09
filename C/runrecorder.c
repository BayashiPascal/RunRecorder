// ------------------ runrecorder.c ------------------

// Include the header
#include "runrecorder.h"

// Last version of the database
char const* const lastVersionDb = "01.00.00";

// Return true if a struct RunRecorder uses the Web API, else false
bool RunRecorderUsesAPI(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Get the version of the database
// Return a new string containg the version
char* RunRecorderGetVersion(
  // The struct RunRecorder
  struct RunRecorder const* const that);

// Upgrade the database
void RunRecorderUpgradeDb(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Create the database locally
void RunRecorderCreateDb(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Create the database locally
void RunRecorderCreateDbLocal(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Create the database through the Web API
void RunRecorderCreateDbAPI(
  // The struct RunRecorder
  struct RunRecorder* const that);

// Constructor for a struct RunRecorder
struct RunRecorder RunRecorderCreate(
  // Path to the SQLite database or Web API
  char const* const url) {

  // Declare the struct RunRecorder
  struct RunRecorder that;

  // Duplicate the url
  that.url = strdup(url);

  // Initialise the other properties
  that.errMsg = NULL;
  that.db = NULL;

  // If the recorder doesn't use the API
  if (RunRecorderUsesAPI(that) == false) {

    // If the SQLite database local file doesn't exists
    FILE* fp =
      fopen(
        url,
        "r");
    if (fp == NULL) {

      // Create the database
      RunRecorderCreateDb(that);

    } else {

      fclose(fp);

    }

    // Open the connection to the local database
    int ret =
      sqlite3_open(
        url,
        &(that.db));
    if (ret) {

      fprintf(
        stderr,
        "Can't open database: %s\n",
        sqlite3_errmsg(that.db));
      sqlite3_close(that.db);
      return 1;

    }

  }

  // If the version of the database is different from the last version
  char* version = RunRecorderGetVersion(&that);
  int cmpVersion =
    strcmp(
      version,
      lastVersionDb);
  free(version);
  if (cmpVersion != 0) {

    // Upgrade the database
    RunRecorderUpgradeDb(&that);

  }

  // Return the struct RunRecorder
  return that;

}

// Destructor for a struct RunRecorder
void RunRecorderFree(
  // The struct RunRecorder to be freed
  struct RunRecorder* const that) {

  // Free memory
  free(that->url);
  if (that->errMsg != NULL) {

    free(that->errMsg);

  }

  // Close the connection to the local database if it was opened
  if (that->db != NULL) {

    sqlite3_close(that->db);

  }

}

// Return true if a struct RunRecorder uses the Web API, else false
bool RunRecorderUsesAPI(
  // The struct RunRecorder
  struct RunRecorder* const that) {

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
void RunRecorderCreateDb(
  // The struct RunRecorder
  struct RunRecorder* const that) {

  // If the RunRecorder uses the Web API
  if (RunRecorderUsesAPI(that) == true) {

    RunRecorderCreateDbAPI(that);

  // Else, the RunRecorder uses a local database
  } else {

    RunRecorderCreateDbLocal(that);

  }

}

// Create the database locally
void RunRecorderCreateDbLocal(
  // The struct RunRecorder
  struct RunRecorder* const that) {

  #define RUNRECORDER_NB_TABLE 5
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

  };
  for (
    int iCmd = 0;
    iCmd < RUNRECORDER_NB_TABLE;
    ++iCmd) {

    int retExec =
      sqlite3_exec(
        db,
        sqlCmd[iCmd],
        // No callback
        NULL,
        // No user data
        NULL,
        &(that->errMsg));
    if (retExec != SQLITE_OK) {

      Raise(TryCatchException_CreateTable);

    }

  }

}

// Create the database through the Web API
void RunRecorderCreateDbAPI(
  // The struct RunRecorder
  struct RunRecorder* const that) {
  // TODO
}

// Upgrade the database
void RunRecorderUpgradeDb(
  // The struct RunRecorder
  struct RunRecorder* const that) {

  // Placeholder
  (void)that;

}

// Get the version of the database
char const* RunRecorderGetVersion(
  // The struct RunRecorder
  struct RunRecorder const* const that) {

  // Return the version of the database
  return NULL; // TODO

}

// ------------------ runrecorder.c ------------------





/*

// Include the header
#include "runrecorder.h"

static int callback(
   void* userData,
     int nbCol,
  char** colVal,
  char** colName) {

  (void)userData;
  for(
    int i = 0;
    i < nbCol;
    ++i) {

    printf(
      "%s = %s\n",
      colName[i],
      colVal[i] ? colVal[i] : "NULL");

  }

  return 0;

}

int toto(
  void) {

  sqlite3* db = NULL;

  int retOpen =
    sqlite3_open(
      "./test.db",
      &db);
  if (retOpen) {

    fprintf(
      stderr,
      "Can't open database: %s\n",
      sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;

  }

  char* errMsg = NULL;
  void* userData = NULL;
  char* sqlCmd[6] = {

      "CREATE TABLE t (a,b)",
      "INSERT INTO t VALUES(1,2)",
      "INSERT INTO t VALUES(3,4)",
      "SELECT a as first_col, b as snd_col FROM t",
      "DELETE FROM t WHERE a = 1",
      "SELECT * FROM t"

  };
  for (
    int iCmd = 0;
    iCmd < 6;
    ++iCmd) {

    int retExec =
      sqlite3_exec(
        db,
        sqlCmd[iCmd],
        callback,
        userData,
        &errMsg);
    if (retExec != SQLITE_OK) {

      fprintf(
        stderr,
        "SQL error: %s\n",
        errMsg);
      sqlite3_free(errMsg);

    }

  }

  sqlite3_stmt* stmt = NULL;
  int readUpToTheEnd = -1;
  const char *hasReadUpToHere = NULL;
  int retPrepare =
    sqlite3_prepare_v2(
      db,
      "SELECT * FROM t WHERE b = ?",
      readUpToTheEnd,
      &stmt,
      &hasReadUpToHere);
  if (retPrepare != SQLITE_OK) {

    fprintf(
      stderr,
      "Prepare failed: %d\n",
      retPrepare);

  }

  int iCol = 1; // Starts from 1 !!
  for (
    int colVal = 0;
    colVal < 5;
    ++colVal) {

    int retBind =
      sqlite3_bind_int(
        stmt,
        iCol,
        colVal);
    if (retBind != SQLITE_OK) {

      fprintf(
        stderr,
        "Bind failed: %d\n",
        retBind);
      sqlite3_free(errMsg);

    }

    int retStep = sqlite3_step(stmt);
    while (retStep != SQLITE_DONE) {

      if (retStep == SQLITE_ROW) {

        int nbCol = sqlite3_column_count(stmt);
        for (
          int jCol = 0;
          jCol < nbCol;
          ++jCol) {

          int val =
            sqlite3_column_int(
              stmt,
              jCol);
          const char* colName =
            sqlite3_column_name(
              stmt,
              jCol);
          printf(
            "%s = %d, ",
            colName,
            val);

        }

        printf("\n");

      } else {

        fprintf(
          stderr,
          "Step failed: %d\n",
          retStep);
        sqlite3_free(errMsg);

      }

      retStep = sqlite3_step(stmt);

    }

    int retClear = sqlite3_clear_bindings(stmt);
    if (retClear != SQLITE_OK) {

      fprintf(
        stderr,
        "Clear binding error: %d\n",
        retClear);

    }

    int retReset = sqlite3_reset(stmt);
    if (retReset != SQLITE_OK) {

      fprintf(
        stderr,
        "Reset error: %d\n",
        retReset);

    }

  }

  int retFinalize = sqlite3_finalize(stmt);
  if (retFinalize != SQLITE_OK) {

    fprintf(
      stderr,
      "Finalize error: %d\n",
      retFinalize);

  }

  int retExec =
    sqlite3_exec(
      db,
      "DROP TABLE t",
      NULL,
      userData,
      &errMsg);
  if (retExec != SQLITE_OK) {

    fprintf(
      stderr,
      "SQL error: %s\n",
      errMsg);
    sqlite3_free(errMsg);

  }

  retExec =
    sqlite3_exec(
      db,
      "VACUUM",
      NULL,
      userData,
      &errMsg);
  if (retExec != SQLITE_OK) {

    fprintf(
      stderr,
      "SQL error: %s\n",
      errMsg);
    sqlite3_free(errMsg);

  }

  sqlite3_close(db);
  return 0;
}

*/
