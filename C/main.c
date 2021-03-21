#include <stdio.h>
#include "runrecorder.h"

// Helper function to commonalize code during exception management
// Inputs:
//     caller: string to identify the calling portion of code
//   recorder: the struct RunRecorder used to print info related to the
//             exception
void PrintCaughtException(
                char const* const caller,
  struct RunRecorder const* const recorder) {

  fprintf(
    stderr,
    "Caught exception %s during %s.\n",
    TryCatchExcToStr(TryCatchGetLastExc()),
    caller);
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

}

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

    PrintCaughtException(
      "RunRecorderInit",
      recorder);
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

    PrintCaughtException(
      "RunRecorderGetVersion",
      recorder);
    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTryWithDefault;

  // Create a new project
  Try {

    Try {

      RunRecorderAddProject(
        recorder,
        "RoomTemperature");

    } Catch (RunRecorderExc_ProjectNameAlreadyUsed) {

      printf("Project RoomTemperature already in the database\n");

    } EndTry;

  } CatchDefault {

    PrintCaughtException(
      "RunRecorderAddProject",
      recorder);
    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTryWithDefault;

  // Get the list of projects
  struct RunRecorderPairsRefVal* projects = NULL;
  Try {

    projects = RunRecorderGetProjects(recorder);
    printf("Projects:\n");
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

    PrintCaughtException(
      "RunRecorderGetProjects",
      recorder);
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

  } CatchDefault {

    PrintCaughtException(
      "RunRecorderAddMetric",
      recorder);
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
    printf("Metrics of RoomTemperature:\n");
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

    PrintCaughtException(
      "RunRecorderGetMetrics",
      recorder);
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
      "Added measure ref. %ld\n",
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
      "Added measure ref. %ld\n",
      recorder->refLastAddedMeasure);
    RunRecorderMeasureFree(&measure);

  } CatchDefault {

    PrintCaughtException(
      "RunRecorderAddMeasure",
      recorder);
    RunRecorderMeasureFree(&measure);
    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTryWithDefault;

  // Delete measurement
  Try {

    printf(
      "Delete measure %ld\n",
      recorder->refLastAddedMeasure);
    RunRecorderDeleteMeasure(
      recorder,
      recorder->refLastAddedMeasure);

  } CatchDefault {

    PrintCaughtException(
      "RunRecorderDeleteMeasure",
      recorder);
    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTryWithDefault;

  // Get the measures
  struct RunRecorderMeasures* measures = NULL;
  Try {

    measures =
      RunRecorderGetMeasures(
        recorder,
        "RoomTemperature");
    if (measures != NULL) {

      RunRecorderMeasuresPrintCSV(
        measures,
        stdout);
      RunRecorderMeasuresFree(&measures);

    }

  } CatchDefault {

    PrintCaughtException(
      "RunRecorderGetMeasures",
      recorder);
    RunRecorderMeasuresFree(&measures);
    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTryWithDefault;

  // Delete the project
  Try {

    RunRecorderFlushProject(
      recorder,
      "RoomTemperature");
    printf("Deleted project RoomTemperature\n");

  } CatchDefault {

    PrintCaughtException(
      "RunRecorderFlushProject",
      recorder);
    RunRecorderFree(&recorder);
    exit(EXIT_FAILURE);

  } EndTryWithDefault;

  // Free memory
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
