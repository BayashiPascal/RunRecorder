#include <stdio.h>
#include <stdbool.h>
#include "runrecorder.h"

// Main function
int main() {

  // Path to the SQLite database local file or Web API
  char const* pathDb = "./runrecorder.db";
  //char const* pathApi = "https://localhost/RunRecorder/api.php";
  char const* pathApi = "https://www.bayashiinjapan.net/RunRecorder/api.php";

  // Create the RunRecorder instance
  struct RunRecorder* recorder;
  Try {

    // Give pathApi in argument if you want to use the Web API instead
    // of a local file
    recorder = RunRecorderCreate(pathDb);
    //recorder = RunRecorderCreate(pathApi);

  } Catch(TryCatchException_CreateTableFailed) {

    fprintf(
      stderr,
      "Failed to create tables in the database.\n");
    fprintf(
      stderr,
      "%s\n",
      recorder->errMsg);
    exit(EXIT_FAILURE);

  } Catch(TryCatchException_OpenDbFailed) {

    fprintf(
      stderr,
      "Failed to open the database.\n");
    fprintf(
      stderr,
      "%s\n",
      recorder->errMsg);
    exit(EXIT_FAILURE);

  } Catch(TryCatchException_CreateCurlFailed) {

    fprintf(
      stderr,
      "Failed to create the Curl instance.\n");
    exit(EXIT_FAILURE);

  } Catch(TryCatchException_CurlSetOptFailed) {

    fprintf(
      stderr,
      "Curl setopt failed.\n");
    fprintf(
      stderr,
      "%s\n",
      recorder->errMsg);
    exit(EXIT_FAILURE);

  } Catch(TryCatchException_SQLRequestFailed) {

    fprintf(
      stderr,
      "SQL request failed.\n");
    fprintf(
      stderr,
      "%s\n",
      recorder->errMsg);
    exit(EXIT_FAILURE);

  } Catch(TryCatchException_ApiRequestFailed) {

    fprintf(
      stderr,
      "API request failed.\n");
    fprintf(
      stderr,
      "%s\n",
      recorder->errMsg);
    exit(EXIT_FAILURE);

  } Catch(TryCatchException_MallocFailed) {

    fprintf(
      stderr,
      "malloc failed.\n");
    exit(EXIT_FAILURE);

  } EndTry;

  // Get the version of the database
  Try {

    char* version = RunRecorderGetVersion(recorder);
    printf(
      "%s\n",
      version);
    free(version);

  } Catch(TryCatchException_SQLRequestFailed) {

    fprintf(
      stderr,
      "SQL request failed.\n");
    fprintf(
      stderr,
      "%s\n",
      recorder->errMsg);
    exit(EXIT_FAILURE);

  } EndTry;

/*
  // Create a new project
  bool success =
    RunRecorderAddProject(
      recorder,
      "Body weight");
  if (success == false) {

    fprintf(
      stderr,
      "%s\n",
      recorder->errMsg);

  }

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
