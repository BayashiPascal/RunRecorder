#include <stdlib.h>
#include <stdio.h>
#include "runrecorder.h"

// ================== Macros =========================

// Polymorphic free
#define PolyFree(P) _Generic(P, \
  struct RunRecorder**: RunRecorderFree, \
  struct RunRecorderPairsRefVal**: RunRecorderPairsRefValFree, \
  struct RunRecorderPairsRefValDef**: RunRecorderPairsRefValDefFree, \
  struct RunRecorderMeasure**: RunRecorderMeasureFree, \
  struct RunRecorderMeasures**: RunRecorderMeasuresFree, \
  struct CLI**: CLIFree, \
  default: free)((void*)P)

// malloc freeing the assigned variable and raising exception if it fails
#define SafeMalloc(T, S)  \
  do { \
    PolyFree(T); T = malloc(S); \
    if (T == NULL) Raise(TryCatchExc_MallocFailed); \
  } while(false)

// Loop from 0 to n
#define ForZeroTo(I, N) for (long I = 0; I < N; ++I)

// strdup freeing the assigned variable and raising exception if it fails
#define SafeStrDup(T, S)  \
  do { \
    PolyFree(T); T = strdup(S); \
    if (T == NULL) Raise(TryCatchExc_MallocFailed); \
  } while(false)

// ================== Static variables =========================



// ================== CLI status =========================

enum CLIStatus {

  CLIStatus_main,
  CLIStatus_addProject,
  CLIStatus_selectProject,
  CLIStatus_addMetric,
  CLIStatus_quit,
  CLIStatus_lastID,

};

// ================== Structures definitions =========================

struct CLI;
struct CLI {

  // The RunRecorder instance
  struct RunRecorder* runRecorder;

  // Current status of the CLI
  enum CLIStatus status;

  // Functions to print the menu according to the status
  void (*printMenu[CLIStatus_lastID])(
    struct CLI* const);

  // Functions to process the user input according to the status
  void (*processInput[CLIStatus_lastID])(
    struct CLI* const,
    char const* const);

  // List of the projects
  struct RunRecorderPairsRefVal* projects;

  // Currently selected project
  char* curProject;

  // List of the metrics for the current project
  struct RunRecorderPairsRefValDef* metrics;

};

// ================== Functions declaration =========================

// Create a new struct CLI
// Inputs:
//   url: path to the local database or url to the web api
// Output:
//   Return the new struct CLI
struct CLI* CLICreate(
  char const* const url);

// Free memory used by a struct CLI
// Input:
//   that: the struct CLI to be freed
void CLIFree(
  struct CLI** const that);

// Print the error message when an exception is caught
// Inputs:
//   that: the struct CLI
void PrintCaughtException(
  struct CLI const* const that);

// Main loop of the CLI
// Input:
//   that: the struct CLI
void Run(
  struct CLI* const that);

// Print the main menu
// Input:
//   that: the struct CLI
void PrintMenuMain(
  struct CLI* const that);

// Print the menu to add a project
// Input:
//   that: the struct CLI
void PrintMenuAddProject(
  struct CLI* const that);

// Print the menu to add a metric
// Input:
//   that: the struct CLI
void PrintMenuAddMetric(
  struct CLI* const that);

// Print the menu to select a project
// Input:
//   that: the struct CLI
void PrintMenuSelectProject(
  struct CLI* const that);

// Process the user input in the main menu
// Input:
//    that: the struct CLI
//   input: the user input
void ProcessInputMain(
  struct CLI* const that,
  char const* const input);

// Process the user input in the menu to add project
// Input:
//    that: the struct CLI
//   input: the user input
void ProcessInputAddProject(
  struct CLI* const that,
  char const* const input);

// Process the user input in the menu to add metric
// Input:
//    that: the struct CLI
//   input: the user input
void ProcessInputAddMetric(
  struct CLI* const that,
  char const* const input);

// Process the user input in the menu to select a project
// Input:
//    that: the struct CLI
//   input: the user input
void ProcessInputSelectProject(
  struct CLI* const that,
  char const* const input);

// Print the list of projects
// Input:
//    that: the struct CLI
void ListProjects(
  struct CLI* const that);

// Print the list of metrics for the current project
// Input:
//    that: the struct CLI
void ListMetrics(
  struct CLI* const that);

// ================== Functions definition =========================

// Create a new struct CLI
// Inputs:
//   url: path to the local database or url to the web api
// Output:
//   Return the new struct CLI
struct CLI* CLICreate(
  char const* const url) {

  // Declare the new CLI
  struct CLI* cli = NULL;

  // Allocate memory
  SafeMalloc(
    cli,
    sizeof(struct CLI));

  // Init properties
  cli->status = CLIStatus_main;
  cli->runRecorder = NULL;
  cli->printMenu[CLIStatus_main] = PrintMenuMain;
  cli->printMenu[CLIStatus_addProject] = PrintMenuAddProject;
  cli->printMenu[CLIStatus_selectProject] = PrintMenuSelectProject;
  cli->printMenu[CLIStatus_addMetric] = PrintMenuAddMetric;
  cli->printMenu[CLIStatus_quit] = NULL;
  cli->processInput[CLIStatus_main] = ProcessInputMain;
  cli->processInput[CLIStatus_addProject] = ProcessInputAddProject;
  cli->processInput[CLIStatus_selectProject] = ProcessInputSelectProject;
  cli->processInput[CLIStatus_addMetric] = ProcessInputAddMetric;
  cli->processInput[CLIStatus_quit] = NULL;
  cli->projects = NULL;
  cli->curProject = NULL;
  cli->metrics = NULL;

  // Create the RunRecorder instance
  printf(
   "Connecting to database %s...\n",
   url);
  cli->runRecorder = RunRecorderCreate(url);
  RunRecorderInit(cli->runRecorder);
  printf("Connection established\n");

  // Display a welcome message
  char* version = RunRecorderGetVersion(cli->runRecorder);
  printf(
    "Welcome to RunRecorder version %s\n",
    version);
  free(version);

  // Initialise the list of projects
  cli->projects = RunRecorderGetProjects(cli->runRecorder);

  // Return the new struct CLI
  return cli;

}

// Free memory used by a struct CLI
// Input:
//   that: the struct CLI to be freed
void CLIFree(
  struct CLI** const that) {

  // If the struct CLI is already freed, nothing to do
  if (that == NULL || *that == NULL) return;

  // Free memory
  PolyFree(&((*that)->metrics));
  PolyFree((*that)->curProject);
  PolyFree(&((*that)->projects));
  PolyFree(&((*that)->runRecorder));
  printf("Disconnected from the database\n");
  PolyFree(*that);
  *that = NULL;

}

// Print the error message when an exception is caught
// Inputs:
//   that: the struct CLI
void PrintCaughtException(
  struct CLI const* const that) {

  fprintf(
    stderr,
    "Caught exception %s.\n",
    TryCatchExcToStr(TryCatchGetLastExc()));
  if (that != NULL && that->runRecorder != NULL) {

    if (that->runRecorder->errMsg != NULL)
      fprintf(
        stderr,
        "%s\n",
        that->runRecorder->errMsg);

    if (that->runRecorder->sqliteErrMsg != NULL)
      fprintf(
        stderr,
        "%s\n",
        that->runRecorder->sqliteErrMsg);

  }

}

// Get the user input for stdin
// Output:
//   Return the user input (without the line return) or NULL if the user
//   pressed ctrl-d
char* GetUserInput(
  void) {

  char* input = NULL;
  size_t maxLenInput = 0;
  ssize_t lenInput =
    getline(
      &input,
      &maxLenInput,
      stdin);

  // If there is a user input
  if (lenInput >= 0) {

    // Return the input without the line return
    input[lenInput - 1] = '\0';
    return input;

  // Else the user cancelled with ctrl-d
  } else {

    // Free memory and return NULL
    free(input);
    return NULL;

  }

}

// Main loop of the CLI
// Input:
//   that: the struct CLI
void Run(
  struct CLI* const that) {

  // Loop until the user ask to quit
  while (that->status != CLIStatus_quit) {

    // Print the menu according to the current status
    (*that->printMenu[that->status])(that);

    // Get the user input
    printf(" > ");
    char* input = GetUserInput();

    Try {

      // Process user input according to the current status
      (*that->processInput[that->status])(
        that,
        input);

    } CatchDefault {

      PrintCaughtException(that);

    } EndTryWithDefault;

    // Free memory
    PolyFree(input);

  }

  printf("Exiting from RunRecorder. Bye!\n");

}

// Print the main menu
// Input:
//   that: the struct CLI
void PrintMenuMain(
  struct CLI* const that) {

  // Print the menu
  printf(
    "--- Main menu ---\n"
    "1 - List the projects\n"
    "2 - Add a project\n"
    "3 - Select a project\n");
  if (that->curProject != NULL) {

    printf(
      "4 - List the metrics of %s\n"
      "5 - Add a metric to %s\n"
      "6 - Add one measurement to %s\n",
      that->curProject,
      that->curProject,
      that->curProject);

  }

  printf("q - Quit\n");

}

// Print the menu to add a project
// Input:
//   that: the struct CLI
void PrintMenuAddProject(
  struct CLI* const that) {

  // Print the menu
  printf(
    "--- Add a project ---\n"
    "Enter the name of the project, or leave blank to cancel\n");

}

// Print the menu to add a metric
// Input:
//   that: the struct CLI
void PrintMenuAddMetric(
  struct CLI* const that) {

  // Print the menu
  printf(
    "--- Add a metric ---\n"
    "Enter the label of the metric, or leave blank to cancel\n");

}

// Print the menu to select a project
// Input:
//   that: the struct CLI
void PrintMenuSelectProject(
  struct CLI* const that) {

  // Print the menu
  printf(
    "--- Select a project ---\n"
    "Enter the name of the project, or leave blank to cancel\n");

}

// Process the user input in the main menu
// Input:
//    that: the struct CLI
//   input: the user input
void ProcessInputMain(
  struct CLI* const that,
  char const* const input) {

  // Variable to memorise the acceptable commands
  #define NbCmdMain 6
  char* cmds[NbCmdMain] = {

    "1",
    "2",
    "3",
    "4",
    "5",
    "q"

  };

  // Loop on the acceptable commands
  ForZeroTo(iCmd, NbCmdMain) {

    // If the user input is this command
    int retCmp =
      strcmp(
        input,
        cmds[iCmd]);
    if (retCmp == 0) {

      // Switch on the command
      switch(iCmd) {

        case 0:
          ListProjects(that);
          break;

        case 1:
          that->status = CLIStatus_addProject;
          break;

        case 2:
          that->status = CLIStatus_selectProject;
          break;

        case 3:
          if (that->curProject != NULL) ListMetrics(that);
          break;

        case 4:
          if (that->curProject != NULL) that->status = CLIStatus_addMetric;
          break;

        case 5:
          that->status = CLIStatus_quit;
          break;

        default:
          break;

      }

    }

  }

}

// Process the user input in the menu to add project
// Input:
//    that: the struct CLI
//   input: the user input
void ProcessInputAddProject(
  struct CLI* const that,
  char const* const input) {

  // If the user has entered a name
  if (input != NULL && *input != '\0') {

    // If the name is valid
    bool isValid = RunRecorderIsValidLabel(input);
    if (isValid == true) {

      Try {

        RunRecorderAddProject(
          that->runRecorder,
          input);

        // Update the list of projects
        PolyFree(&(that->projects));
        that->projects = RunRecorderGetProjects(that->runRecorder);

      } Catch (RunRecorderExc_ProjectNameAlreadyUsed) {

        printf("This name is already used\n");

      } CatchDefault {

        PrintCaughtException(that);

      } EndTryWithDefault;


    // Else the input is invalid
    } else {

      printf("This name is invalid\n");

    }

  }

  // Set the CLI back to the main menu
  that->status = CLIStatus_main;

}

// Process the user input in the menu to add metric
// Input:
//    that: the struct CLI
//   input: the user input
void ProcessInputAddMetric(
  struct CLI* const that,
  char const* const input) {

  // If the user has entered a label
  if (input != NULL && *input != '\0') {

    // If the label is valid
    bool isValid = RunRecorderIsValidLabel(input);
    if (isValid == true) {

      // Ask the user for the default value
      printf(
        "Enter the default value of the metric, or ctrl-d to cancel\n"
        " > ");
      char* defaultVal = GetUserInput();

      // If the user has entered a default value
      if (defaultVal != NULL) {

        // If the default value is valid
        isValid = RunRecorderIsValidValue(defaultVal);
        if (isValid == true) {

          Try {

            RunRecorderAddMetric(
              that->runRecorder,
              that->curProject,
              input,
              defaultVal);

            // Update the list of metrics
            PolyFree(&(that->metrics));
            that->projects = RunRecorderGetProjects(that->runRecorder);

          } Catch (RunRecorderExc_MetricNameAlreadyUsed) {

            printf("This label is already used\n");

          } CatchDefault {

            PrintCaughtException(that);

          } EndTryWithDefault;

        // Else, the default value is invalid
        } else {

          printf("This default value is invalid\n");

        }

      }

    // Else the label is invalid
    } else {

      printf("This label is invalid\n");

    }

  }

  // Set the CLI back to the main menu
  that->status = CLIStatus_main;

}

// Process the user input in the menu to select a project
// Input:
//    that: the struct CLI
//   input: the user input
void ProcessInputSelectProject(
  struct CLI* const that,
  char const* const input) {

  // If the user has entered a name
  if (input != NULL && *input != '\0') {

    // Check if the name match one of the project in the database
    bool isValid = false;
    ForZeroTo(iProject, that->projects->nb) {

      int retCmp =
        strcmp(
          input,
          that->projects->values[iProject]);
      if (retCmp == 0) isValid = true;

    }

    // If the name is valid
    if (isValid == true) {

      // Memorise the name of the current project
      SafeStrDup(
        that->curProject,
        input);

      // Update the list of metrics
      PolyFree(&(that->metrics));
      that->metrics =
        RunRecorderGetMetrics(
          that->runRecorder,
          that->curProject);

    }

  }

  // Set the CLI back to the main menu
  that->status = CLIStatus_main;

}

// Print the list of projects
// Input:
//    that: the struct CLI
void ListProjects(
  struct CLI* const that) {

  // Get the projects
  PolyFree(&(that->projects));
  that->projects = RunRecorderGetProjects(that->runRecorder);

  // Print the projects
  printf("\nProjects:\n");
  ForZeroTo(iProject, that->projects->nb)
    printf(
      "ref: %ld label: %s\n",
      that->projects->refs[iProject],
      that->projects->values[iProject]);
  if (that->projects->nb == 0) printf("No projects\n");
  printf("\n");

}

// Print the list of metrics for the current project
// Input:
//    that: the struct CLI
void ListMetrics(
  struct CLI* const that) {

  // Get the projects
  PolyFree(&(that->metrics));
  that->metrics =
    RunRecorderGetMetrics(
      that->runRecorder,
      that->curProject);

  // Print the metrics
  printf("\nMetrics:\n");
  ForZeroTo(iMetric, that->metrics->nb)
    printf(
      "ref: %ld, label: %s, default value: %s\n",
      that->metrics->refs[iMetric],
      that->metrics->values[iMetric],
      that->metrics->defaultValues[iMetric]);
  if (that->metrics->nb == 0) printf("No metrics\n");
  printf("\n");

}

// Main function
int main(
     int argc,
  char** argv) {

  // Declare the variable to memorise the CLI instance
  struct CLI* cli = NULL;

  Try {

    // If the user gave one argument
    if (argc == 2) {

      // Create the CLI
      cli = CLICreate(argv[1]);

      // Start the main loop of the CLI
      Run(cli);

    // Else, the user gave the wrong number of arguments
    } else {

      // Print the help
      printf(
        "Usage: runrecorder <path to the local database, "
        "or url of the web api>\n");

    }

  } CatchDefault {

    PrintCaughtException(cli);

  } EndTryWithDefault;

  // Free memory
  CLIFree(&cli);

  // Return the success code
  return EXIT_SUCCESS;

}
