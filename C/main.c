#include <stdio.h>
#include "runrecorder.h"

// Main function
int main() {

  // Path to the SQLite database local file or Web API
  char const* pathDb = "./runrecorder.db";
  //char const* pathApi = "https://localhost/RunRecorder/api.php";
  char const* pathApi = "http://www.bayashiinjapan.net/RunRecorder/api.php";

  // Create the RunRecorder instance
  struct RunRecorder* recorder = NULL;
  Try {

    // Give pathApi in argument if you want to use the Web API instead
    // of a local file
    //recorder = RunRecorderCreate(pathDb);
    recorder = RunRecorderCreate(pathApi);

    // Initialise the struct RunRecorder
    RunRecorderInit(recorder);

  } Catch(RunRecorderExc_CreateTableFailed)
    CatchAlso(RunRecorderExc_OpenDbFailed)
    CatchAlso(RunRecorderExc_CreateCurlFailed)
    CatchAlso(RunRecorderExc_CurlSetOptFailed)
    CatchAlso(RunRecorderExc_CurlRequestFailed)
    CatchAlso(RunRecorderExc_SQLRequestFailed)
    CatchAlso(RunRecorderExc_ApiRequestFailed)
    CatchAlso(RunRecorderExc_MallocFailed) {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderInit.\n",
      RunRecorderExceptionStr[TryCatchGetLastExc()]);
    if (recorder->errMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->errMsg);

    }

    if (recorder->sqliteErrMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->sqliteErrMsg);

    }

    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTry;

  // Get the version of the database
  Try {

    char* version = RunRecorderGetVersion(recorder);
    printf(
      "version: %s\n",
      version);
    free(version);

  } Catch(RunRecorderExc_SQLRequestFailed)
    CatchAlso(RunRecorderExc_CurlSetOptFailed)
    CatchAlso(RunRecorderExc_CurlRequestFailed)
    CatchAlso(RunRecorderExc_ApiRequestFailed)
    CatchAlso(RunRecorderExc_MallocFailed) {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderGetVersion.\n",
      RunRecorderExceptionStr[TryCatchGetLastExc()]);
    if (recorder->errMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->errMsg);

    }

    if (recorder->sqliteErrMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->sqliteErrMsg);

    }

    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTry;

  // Create a new project
  long refProject = 0;
  Try {

    refProject =
      RunRecorderAddProject(
        recorder,
        "RoomTemperature");
    printf(
      "refProject: %ld\n",
      refProject);

  } Catch(RunRecorderExc_SQLRequestFailed)
    CatchAlso(RunRecorderExc_CurlSetOptFailed)
    CatchAlso(RunRecorderExc_CurlRequestFailed)
    CatchAlso(RunRecorderExc_ApiRequestFailed)
    CatchAlso(RunRecorderExc_InvalidProjectName)
    CatchAlso(RunRecorderExc_AddProjectFailed)
    CatchAlso(RunRecorderExc_MallocFailed) {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderAddProject.\n",
      RunRecorderExceptionStr[TryCatchGetLastExc()]);
    if (recorder->errMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->errMsg);

    }

    if (recorder->sqliteErrMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->sqliteErrMsg);

    }

    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTry;


  // Get the list of projects
  struct RunRecorderPairsRefVal* projects = NULL;
  Try {

    projects = RunRecorderGetProjects(recorder);
    for (
      long iProject = 0;
      iProject < projects->nb;
      ++iProject) {

      printf(
        "ref: %ld label: %s\n",
        projects->refs[iProject],
        projects->vals[iProject]);

    }

  } Catch(RunRecorderExc_SQLRequestFailed)
    CatchAlso(RunRecorderExc_ApiRequestFailed)
    CatchAlso(RunRecorderExc_CurlRequestFailed)
    CatchAlso(RunRecorderExc_CurlSetOptFailed)
    CatchAlso(RunRecorderExc_InvalidJSON)
    CatchAlso(RunRecorderExc_MallocFailed) {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderGetProjects.\n",
      RunRecorderExceptionStr[TryCatchGetLastExc()]);
    if (recorder->errMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->errMsg);

    }

    if (recorder->sqliteErrMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->sqliteErrMsg);

    }

    RunRecorderPairsRefValFree(&projects);
    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTry;

  // Create a new metric
  Try {

    RunRecorderAddMetric(
      recorder,
      "RoomTemperature",
      "Date",
      "-");
    RunRecorderAddMetric(
      recorder,
      "RoomTemperature",
      "Temperature",
      "0.0");

  } Catch(RunRecorderExc_SQLRequestFailed)
    CatchAlso(RunRecorderExc_CurlSetOptFailed)
    CatchAlso(RunRecorderExc_CurlRequestFailed)
    CatchAlso(RunRecorderExc_ApiRequestFailed)
    CatchAlso(RunRecorderExc_InvalidProjectName)
    CatchAlso(RunRecorderExc_AddProjectFailed)
    CatchAlso(RunRecorderExc_MallocFailed) {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderAddMetric.\n",
      RunRecorderExceptionStr[TryCatchGetLastExc()]);
    if (recorder->errMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->errMsg);

    }

    if (recorder->sqliteErrMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->sqliteErrMsg);

    }

    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTry;


/*  
bodyrecorder->sh add_metric "project=1&label=Date&default=-"
bodyrecorder->sh add_metric "project=1&label=Weight&default=0.0"
bodyrecorder->sh metrics "project=1"

*/
  // Free memory
  RunRecorderFree(&recorder);
  RunRecorderPairsRefValFree(&projects);

  return EXIT_SUCCESS;

}
