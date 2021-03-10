#include <stdio.h>
#include "runrecorder.h"

// Main function
int main() {

  // Path to the SQLite database local file or Web API
  char const* pathDb = "./runrecorder.db";
  //char const* pathApi = "https://localhost/RunRecorder/api.php";
  char const* pathApi = "http://www.bayashiinjapan.net/RunRecorder/api.php";

  // Create the RunRecorder instance
  // Give pathApi in argument if you want to use the Web API instead
  // of a local file
  //struct RunRecorder* recorder = RunRecorderCreate(pathDb);
  struct RunRecorder* recorder = RunRecorderCreate(pathApi);
  Try {

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
      "Caught exception %d.\n",
      TryCatchGetLastExc());
    if (recorder->errMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->errMsg);

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
      "Caught exception %d.\n",
      TryCatchGetLastExc());
    if (recorder->errMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->errMsg);

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
        "Body weight");
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
      "Caught exception %d.\n",
      TryCatchGetLastExc());
    if (recorder->errMsg != NULL) {

      fprintf(
        stderr,
        "%s\n",
        recorder->errMsg);

    }

    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTry;

/*
  // Get the list of projects
  success =
    RunRecorderGetProjects(
      recorder,
      );
  if (success == false) {

    fprintf(
      stderr,
      "%s\n",
      recorder->errMsg);

  }
  
bodyrecorder->sh add_metric "project=1&label=Date&default=-"
bodyrecorder->sh add_metric "project=1&label=Weight&default=0.0"
bodyrecorder->sh metrics "project=1"

*/
  // Free memory
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
