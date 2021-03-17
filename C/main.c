#include <stdio.h>
#include "runrecorder.h"

// Main function
int main(
     int argc,
  char** argv) {

  // Unused parameters
  (void)argc; (void)argv;

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

  } CatchDefault {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderInit.\n",
      TryCatchExceptionToStr(TryCatchGetLastExc()));
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

  } EndTryWithDefault;

  // Get the version of the database
  Try {

    char* version = RunRecorderGetVersion(recorder);
    printf(
      "version: %s\n",
      version);
    free(version);

  } CatchDefault {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderGetVersion.\n",
      TryCatchExceptionToStr(TryCatchGetLastExc()));
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

  } EndTryWithDefault;

  // Create a new project
  Try {

    Try {

      RunRecorderAddProject(
        recorder,
        "RoomTemperature");

    } Catch (TryCatchExc_ProjectNameAlreadyUsed) {

      printf("Project RoomTemperature already in the database\n");

    } EndTry;

  } CatchDefault {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderAddProject.\n",
      TryCatchExceptionToStr(TryCatchGetLastExc()));
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

  } EndTryWithDefault;

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
        projects->values[iProject]);

    }

    // Free memory
    RunRecorderPairsRefValFree(&projects);

  } CatchDefault {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderGetProjects.\n",
      TryCatchExceptionToStr(TryCatchGetLastExc()));
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

  } EndTryWithDefault;


  // Create new metrics
  Try {

    Try {

      RunRecorderAddMetric(
        recorder,
        "RoomTemperature",
        "Date",
        "-");

    } Catch (TryCatchExc_MetricNameAlreadyUsed) {

      printf("Metric Date already exists in RoomTemperature\n");

    } EndTry;

    Try {

      RunRecorderAddMetric(
        recorder,
        "RoomTemperature",
        "Temperature",
        "0.0");

    } Catch (TryCatchExc_MetricNameAlreadyUsed) {

      printf("Metric Temperature already exists in RoomTemperature\n");

    } EndTry;

  } CatchDefault {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderAddMetric.\n",
      TryCatchExceptionToStr(TryCatchGetLastExc()));
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

  } EndTryWithDefault;

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
        metrics->values[iMetric]);

    }

    // Free memory
    RunRecorderPairsRefValFree(&metrics);

  } CatchDefault {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderGetMetrics.\n",
      TryCatchExceptionToStr(TryCatchGetLastExc()));
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
    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTryWithDefault;

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
    printf(
      "Added measure ref. %lld\n",
      recorder->refLastAddedMeasure);
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
    printf(
      "Added measure ref. %lld\n",
      recorder->refLastAddedMeasure);
    RunRecorderMeasureFree(&measure);

  } CatchDefault {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderAddMeasure.\n"
      "last measure reference is %lld\n",
      TryCatchExceptionToStr(TryCatchGetLastExc()),
      recorder->refLastAddedMeasure);
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
    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTryWithDefault;

  // Delete measurement
  Try {

    printf(
      "Delete measure %lld\n",
      recorder->refLastAddedMeasure);
    RunRecorderDeleteMeasure(
      recorder,
      recorder->refLastAddedMeasure);

  } CatchDefault {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderDeleteMeasure.\n",
      TryCatchExceptionToStr(TryCatchGetLastExc()));
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

  } EndTryWithDefault;

  // Get the measures
  struct RunRecorderData* measures = NULL;
  Try {

    measures =
      RunRecorderGetMeasures(
        recorder,
        "RoomTemperature");
    if (measures != NULL) {

      RunRecorderDataPrintCSV(
        measures,
        stdout);
      RunRecorderDataFree(&measures);

    }

  } CatchDefault {

    fprintf(
      stderr,
      "Caught exception %s during RunRecorderGetMeasures.\n",
      TryCatchExceptionToStr(TryCatchGetLastExc()));
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

    RunRecorderDataFree(&measures);
    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTryWithDefault;

  // Free memory
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
