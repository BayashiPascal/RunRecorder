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
    recorder = RunRecorderCreate(pathDb);
    //recorder = RunRecorderCreate(pathApi);

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
  Try {

    Try {

      RunRecorderAddProject(
        recorder,
        "RoomTemperature");

    } Catch (RunRecorderExc_ProjectNameAlreadyUsed) {

      printf("Project RoomTemperature already in the database\n");

    } EndTry;

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

  // Create new metrics
  Try {

    Try {

      RunRecorderAddMetric(
        recorder,
        "RoomTemperature",
        "Date",
        "-");

    } Catch (RunRecorderExc_MetricNameAlreadyUsed) {

      printf("Metric Date already exists in RoomTemperature\n");

    } EndTry;

    Try {

      RunRecorderAddMetric(
        recorder,
        "RoomTemperature",
        "Temperature",
        "0.0");

    } Catch (RunRecorderExc_MetricNameAlreadyUsed) {

      printf("Metric Temperature already exists in RoomTemperature\n");

    } EndTry;

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

  // Get the list of metrics
  struct RunRecorderPairsRefVal* metrics = NULL;
  Try {

    metrics =
      RunRecorderGetMetrics(
        recorder,
        "RoomTemperature");
    for (
      long iMetric = 0;
      iMetric < metrics->nb;
      ++iMetric) {

      printf(
        "ref: %ld label: %s\n",
        metrics->refs[iMetric],
        metrics->vals[iMetric]);

    }

  } Catch(RunRecorderExc_SQLRequestFailed)
    CatchAlso(RunRecorderExc_ApiRequestFailed)
    CatchAlso(RunRecorderExc_CurlRequestFailed)
    CatchAlso(RunRecorderExc_CurlSetOptFailed)
    CatchAlso(RunRecorderExc_InvalidJSON)
    CatchAlso(RunRecorderExc_MallocFailed) {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderGetMetrics.\n",
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

    RunRecorderPairsRefValFree(&metrics);
    RunRecorderPairsRefValFree(&projects);
    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTry;

  // Add measurements
  struct RunRecorderMeasure* measure = NULL;
  Try {

    measure = RunRecorderMeasureCreate();
    RunRecorderMeasureAddValue(
      measure,
      "Date",
      "2021-03-08 15:45:00");
    RunRecorderMeasureAddValue(
      measure,
      "Temperature",
      18.5);
    RunRecorderAddMeasure(
      recorder,
      "RoomTemperature",
      measure);

    RunRecorderMeasureFree(&measure);
    measure = RunRecorderMeasureCreate();
    RunRecorderMeasureAddValue(
      measure,
      "Date",
      "2021-03-08 16:19:00");
    RunRecorderMeasureAddValue(
      measure,
      "Temperature",
      19.1);
    RunRecorderAddMeasure(
      recorder,
      "RoomTemperature",
      measure);

  } Catch (RunRecorderExc_MallocFailed) {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderAddMeasure.\n",
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

    RunRecorderMeasureFree(&measure);
    RunRecorderPairsRefValFree(&metrics);
    RunRecorderPairsRefValFree(&projects);
    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTry;

  // Free memory
  RunRecorderMeasureFree(&measure);
  RunRecorderFree(&recorder);
  RunRecorderPairsRefValFree(&projects);
  RunRecorderPairsRefValFree(&metrics);

  return EXIT_SUCCESS;

}
