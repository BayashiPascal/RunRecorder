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
  if (recorder->errMsg != NULL)
    fprintf(
      stderr,
      "%s\n",
      recorder->errMsg);

  if (recorder->sqliteErrMsg != NULL)
    fprintf(
      stderr,
      "%s\n",
      recorder->sqliteErrMsg);

}

// Main function
int main(
     int argc,
  char** argv) {

  return EXIT_SUCCESS;

}
