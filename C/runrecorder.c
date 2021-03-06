// *************** RUNRECORDER.C ***************

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
