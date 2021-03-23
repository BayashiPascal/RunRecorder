# RunRecorder

RunRecorder is a C library and a Web API helping to interface the acquisition of textual data and their recording in a remote or local SQLite database.

The C library allows the user to record data with a small and simple set of functions, hiding the commands and management of the database.

The Web API allows the user to use a remote database, making RunRecorder suitable for distributed environments where several applications in a network of devices record and access data in a shared and centralized scheme.

The C library can interact directly with a database, or remotely with it through the Web API. It acts as an abstraction layer for the difference between the two modes, allowing the user to develop data recording application without worrying whether the database is local or remote.

The Web API being agnostic to the source of the requests it responds, it can also be used with programming languages or tools able to perform HTTP requests. The [Curl](https://curl.se/) project provides such tools and libraries in more than 50 languages. Then, the Web API can also easily be reused in other projects and environments.

RunRecorder uses SQLite databases to store recorded data. [SQLite](https://www.sqlite.org/) is the most widely deployed database engine and benefits of tools and libraries in all OS and many programming languages. Then, the recorded data can easily be used after recording.

RunRecorder provides a simple functionality to retrieve the recorded data in CSV format, and a basic Web viewer using the Web API to display the recorded data in a Web browser.

## Table Of Content

* 1. Install
* 1.1 C library
* 1.2 Web API
* 2. Usage
* 2.1 Through the C library
* 2.1.1 Get the version
* 2.1.2 Create a new project
* 2.1.3 Get the list of projects
* 2.1.4 Create a new metric
* 2.1.5 Get the list of metrics
* 2.1.6 Add a measure
* 2.1.7 Delete a measure
* 2.1.8 Get the measures
* 2.1.9 Delete a project
* 2.2 Through the Web API
* 2.2.1 Get the version
* 2.2.2 Create a new project
* 2.2.3 Get the list of projects
* 2.2.4 Create a new metric
* 2.2.5 Get the list of metrics
* 2.2.6 Add a measure
* 2.2.7 Delete a measure
* 2.2.8 Get the measures
* 2.2.9 Delete a project
* 2.3 From the command line, with Curl
* 2.3.1 Get the version
* 2.3.2 Create a new project
* 2.3.3 Get the list of projects
* 2.3.4 Create a new metric
* 2.3.5 Get the list of metrics
* 2.3.6 Add a measure
* 2.3.7 Delete a measure
* 2.3.8 Get the measures
* 2.3.9 Delete a project
* 2.4 From the command line, with the RunRecorder CLI
* 2.4.1 Get the version
* 2.4.2 Create a new project
* 2.4.3 Get the list of projects
* 2.4.4 Create a new metric
* 2.4.5 Get the list of metrics
* 2.4.6 Add a measure
* 2.4.7 Delete a measure
* 2.4.8 Get the measures
* 2.4.9 Delete a project
* 2.5 From other languages, with HTTP request (e.g. JavaScript)
* 2.5.1 Get the version
* 2.5.2 Create a new project
* 2.5.3 Get the list of projects
* 2.5.4 Create a new metric
* 2.5.5 Get the list of metrics
* 2.5.6 Add a measure
* 2.5.7 Delete a measure
* 2.5.8 Get the measures
* 2.5.9 Delete a project

# 1. Install

Download this repository into a folder of your choice, in the examples below I'll call it `Repos`, as follow:
```
cd Repos
wget https://github.com/BayashiPascal/RunRecorder/archive/main.zip
unzip main.zip
mv RunRecorder-main RunRecorder
rm main.zip
```

## 1.1 C library

Compile the RunRecorder C library as follow:

```
cd Repos/RunRecorder/C
make all
sudo make install
```

*The [SQLite](https://www.sqlite.org/), [Curl](https://curl.se/) and [TryCatchC](https://github.com/BayashiPascal/TryCatchC) libraries will automatically be installed during installation. You may be asked for your password during installation.*

## 1.2 Web API

Install the Web API to your local server or web server by uploading, copying or linking the folder `Repos/RunRecorder/WebAPI` to an URL of your choice on the server. For example, if you're using [LAMP](https://en.wikipedia.org/wiki/LAMP_(software_bundle)) and want to access the API via `https://localhost/RunRecorder/api.php` it should look something like this:
```
sudo ln -s Repos/RunRecorder/WebAPI /var/www/html/RunRecorder
```

*If you use a web server, be aware that the Web API doesn't implement any kind of security mechanism. Anyone knowing the URL of the API will be able to interact with it. If you have security concerns, use the API on a secured local network, or use it after modifying the code of the API according to your security policy.*

# 2. Usage

## 2.1 Through the C library

You can use the C library as introduced in the minimal example below

```
#include <RunRecorder/runrecorder.h>

int main() {

  char const* pathDb = "./runrecorder.db";
  char const* pathApi = "https://localhost/RunRecorder/api.php";

  // Create the RunRecorder instance
  // Give pathApi in argument if you want to use the Web API instead
  // of a local database
  struct RunRecorder* recorder = RunRecorderCreate(pathDb);

  // Initialise the RunRecorder instance
  Try {

    RunRecorderInit(recorder);

  } CatchDefault {

    exit(EXIT_FAILURE);

  } EndTryWithDefault;

  // Your code using RunRecorder here (see below for details)
  // ...

  // Free memory
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
```

*In examples below the Try/Catch blocks are omitted to shorten this documentation. Refer to `Repos/RunRecorder/C/main.c` for more information.*

Compile with the following Makefile:

```
main: main.o
	gcc main.o -lsqlite3 -lm -lpthread -ldl -lcurl -ltrycatchc -o main 

main.o: main.c
	gcc -c main.c 
```

### 2.1.1 Get the version

A first simple test to check if everything is working fine is to request the version of the database.

*The database version is automatically checked and the database is automatically upgraded if needed.*

```
#include <stdio.h>
#include <RunRecorder/runrecorder.h>

int main() {

  char const* pathDb = "./runrecorder.db";

  // Create the RunRecorder instance
  struct RunRecorder* recorder = RunRecorderCreate(pathDb);
  RunRecorderInit(recorder);

  // Get the version of the database
  char* version = RunRecorderGetVersion(recorder);
  printf(
    "%s\n",
    version);
  free(version);

  // Free memory
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
```
Output:
```
01.00.00
```

### 2.1.2 Create a new project

In RunRecorder, data are grouped by projects. To start recording data, the first thing you need to do is to create the project they belong to.

*The project's name must respect the following pattern: `/^[a-zA-Z][a-zA-Z0-9_]*$/`.*

```
#include <RunRecorder/runrecorder.h>

int main() {

  char const* pathDb = "./runrecorder.db";

  // Create the RunRecorder instance
  struct RunRecorder* recorder = RunRecorderCreate(pathDb);
  RunRecorderInit(recorder);

  // Create a new project
  RunRecorderAddProject(
    recorder,
    "RoomTemperature");

  // Free memory
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
```

### 2.1.3 Get the list of projects

You can check the list of projects in the database as follow.

```
#include <stdio.h>
#include <RunRecorder/runrecorder.h>

int main() {

  char const* pathDb = "./runrecorder.db";
  char const* pathApi = "https://localhost/RunRecorder/api.php";

  // Create the RunRecorder instance
  // Give pathApi in argument if you want to use the Web API instead
  // of a local file
  struct RunRecorder* recorder = RunRecorderCreate(pathDb);
  RunRecorderInit(recorder);

  // Get the projects
  struct RunRecorderPairsRefVal* projects = RunRecorderGetProjects(recorder);
  for (
    long iProject = 0;
    iProject < projects->nb;
    ++iProject) {

    printf(
      "ref: %ld label: %s\n",
      projects->refs[iProject],
      projects->vals[iProject]);

  }

  // Free memory
  RunRecorderPairsRefValFree(&projects);
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
```
Output:
```
ref: 1 label: RoomTemperature
```

### 2.1.4 Create a new metric

After adding a new project you'll want to add the metrics that defines this project. In our example project there are two metrics: the date and time of the recording, and the recorded room temperature.

*The metric's label must respect the following pattern: `/^[a-zA-Z][a-zA-Z0-9_]*$/`. The default value of the metric must respect the following pattern: `/^[^"=&]+$*/`. There cannot be two metrics with the same label for the same project. A metric's label can't be 'action' or 'project' (case sensitive, so 'Action' is fine).*

```
#include <stdio.h>
#include <RunRecorder/runrecorder.h>

int main() {

  char const* pathDb = "./runrecorder.db";

  // Create the RunRecorder instance
  struct RunRecorder* recorder = RunRecorderCreate(pathDb);
  RunRecorderInit(recorder);

  // Add metrics to the project
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

  // Free memory
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
```

*You can add more metrics even after having recorded some data. A measurement with no value for a given metric automatically get attributed the default value of this metric.*

### 2.1.5 Get the list of metrics

You can get the list of metrics for a project as follow.

```
#include <stdio.h>
#include <RunRecorder/runrecorder.h>

int main() {

  char const* pathDb = "./runrecorder.db";

  // Create the RunRecorder instance
  struct RunRecorder* recorder = RunRecorderCreate(pathDb);
  RunRecorderInit(recorder);

  // Get the metrics
  struct RunRecorderPairsRefValDef* metrics =
    RunRecorderGetMetrics(
      recorder,
      "RoomTemperature");
  for (
    long iMetric = 0;
    iMetric < metrics->nb;
    ++iMetric) {

    printf(
      "ref: %ld label: %s default: %s\n",
      metrics->refs[iMetric],
      metrics->values[iMetric],
      metrics->defaultValues[iMetric]);

  }


  // Free memory
  RunRecorderPairsRefValDefFree(&metrics);
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
```
Output:
```
ref: 1 label: Date default: -
ref: 2 label: Temperature default: 0.0
```

### 2.1.6 Add a measure

Once you've created a project and set its metrics, you're ready to add measures!

*The values of the measure must respect the following pattern: `/^[^"=&]+$*/`. It is not mandatory to provide a value for all the metrics of the project (if missing, the default value is used instead).*

```
#include <stdio.h>
#include <RunRecorder/runrecorder.h>

int main() {

  char const* pathDb = "./runrecorder.db";

  // Create the RunRecorder instance
  struct RunRecorder* recorder = RunRecorderCreate(pathDb);
  RunRecorderInit(recorder);

  // Create the measure
  struct RunRecorderMeasure* measure = RunRecorderMeasureCreate();
  RunRecorderMeasureAddValue(
    measure,
    "Date",
    "2021-03-08 15:45:00");
  RunRecorderMeasureAddValue(
    measure,
    "Temperature",
    18.5);

  // Add a measure
  RunRecorderAddMeasure(
    recorder,
    "RoomTemperature",
    measure);
  printf(
    "Added measure ref. %ld\n",
    recorder->refLastAddedMeasure);

  // Free memory
  RunRecorderMeasureFree(&measure);
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
```
Output:
```
Added measure ref. 1
```

### 2.1.7 Delete a measure

If you've mistakenly added a measure and want to delete it, you can do so as long as you have memorised its reference (returned when you add it). Also, if an error occured when addind a measure, it may be partially saved in the database, depending on the error. RunRecorder ensures as much as possible the database stays coherent even if there is an error, so you can use the partially saved data. In the other hand if you don't want to keep potentially incomplete measurement, you should always try to delete it with the returned reference if an error occured.

```
#include <stdio.h>
#include <RunRecorder/runrecorder.h>

int main() {

  char const* pathDb = "./runrecorder.db";

  // Create the RunRecorder instance
  struct RunRecorder* recorder = RunRecorderCreate(pathDb);
  RunRecorderInit(recorder);

  // Create the measure
  struct RunRecorderMeasure* measure = RunRecorderMeasureCreate();
  RunRecorderMeasureAddValue(
    measure,
    "Date",
    "2021-03-08 15:45:00");
  RunRecorderMeasureAddValue(
    measure,
    "Temperature",
    18.5);

  Try {

    // Add a measure
    RunRecorderAddMeasure(
      recorder,
      "RoomTemperature",
      measure);
    printf(
      "Added measure ref. %ld\n",
      recorder->refLastAddedMeasure);

    // Raise an exception to simulate an error during addition of the measure
    Raise(TryCatchExc_MallocFailed);

  } CatchDefault {

    printf(
      "Error occured, delete the last added measure ref. %ld\n",
      recorder->refLastAddedMeasure);
    RunRecorderDeleteMeasure(
      recorder,
      recorder->refLastAddedMeasure);

  } EndTryCatchWithDefault;

  // Free memory
  RunRecorderMeasureFree(&measure);
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
```
Output:
```
Added measure ref. 1
Error occured, delete the last added measure ref. 1
```

### 2.1.8 Get the measures

After adding your measurements you'll want to use retrieve them. You can do so as follow.

*RunRecorder retrieve the data as a structure which you could use according to your needs. For convenience, it also provides a function to convert this structure to CSV format and print it to a stream.*

```
#include <stdio.h>
#include <RunRecorder/runrecorder.h>

int main() {

  char const* pathDb = "./runrecorder.db";

  // Create the RunRecorder instance
  struct RunRecorder* recorder = RunRecorderCreate(pathDb);
  RunRecorderInit(recorder);

  // Get the measures
  struct RunRecorderMeasures* measures =
    RunRecorderGetMeasures(
      recorder,
      "RoomTemperature");

  // Print the measures in CSV format on the standard output
  RunRecorderMeasuresPrintCSV(
    measures,
    stdout);

  // Free memory
  RunRecorderMeasuresFree(&measures);
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
```
Output:
```
Ref&Date&Temperature
1&2021-03-08 15:45:00&18.500000
```

*Metrics (columns) are ordered alphabetically, measures (rows) are ordered by time of creation in the database. The delimiter of columns for the CSV conversion is ampersand `&`, and the first line contains the label of metrics. All metrics of the project are present, and their default value is used in rows containing missing values.*

If you have a lot of data and want to retrieve only the last few ones, it is possible to do so as follow. In that case, rows are ordered from the most recent to the oldest.

```
#include <stdio.h>
#include <RunRecorder/runrecorder.h>

int main() {

  char const* pathDb = "./runrecorder.db";

  // Create the RunRecorder instance
  struct RunRecorder* recorder = RunRecorderCreate(pathDb);
  RunRecorderInit(recorder);

  // Get the last 2 measures
  long nbMeasure = 2;
  struct RunRecorderMeasures* measures =
    RunRecorderGetLastMeasures(
      recorder,
      "RoomTemperature",
      nbMeasure);

  // Print the measures in CSV format on the standard output
  RunRecorderMeasuresPrintCSV(
    measures,
    stdout);

  // Free memory
  RunRecorderMeasuresFree(&measures);
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
```
Output:
```
Ref&Date&Temperature
1&2021-03-08 15:45:00&18.500000
```

### 2.1.9 Delete a project

Once you've finished collecting data for a project and want to free space in the database, you can delete the project and all the associated metrics and measurements as follow. 

```
#include <stdio.h>
#include <RunRecorder/runrecorder.h>

int main() {

  char const* pathDb = "./runrecorder.db";

  // Create the RunRecorder instance
  struct RunRecorder* recorder = RunRecorderCreate(pathDb);
  RunRecorderInit(recorder);

  // Delete the project
  RunRecorderFlushProject(
    recorder,
    "RoomTemperature");

  // Free memory
  RunRecorderFree(&recorder);

  return EXIT_SUCCESS;

}
```

## 2.2 Through the Web API

### 2.2.1 Get the version

### 2.2.2 Create a new project

### 2.2.3 Get the list of projects

### 2.2.4 Create a new metric

### 2.2.5 Get the list of metrics

### 2.2.6 Add a measure

### 2.2.7 Delete a measure

### 2.2.8 Get the measures

### 2.2.9 Delete a project

## 2.3 From the command line, with Curl

### 2.3.1 Get the version

### 2.3.2 Create a new project

### 2.3.3 Get the list of projects

### 2.3.4 Create a new metric

### 2.3.5 Get the list of metrics

### 2.3.6 Add a measure

### 2.3.7 Delete a measure

### 2.3.8 Get the measures

### 2.3.9 Delete a project

## 2.4 From the command line, with the RunRecorder CLI

### 2.4.1 Get the version

### 2.4.2 Create a new project

### 2.4.3 Get the list of projects

### 2.4.4 Create a new metric

### 2.4.5 Get the list of metrics

### 2.4.6 Add a measure

### 2.4.7 Delete a measure

### 2.4.8 Get the measures

### 2.4.9 Delete a project

## 2.5 From other languages, with HTTP request (e.g. JavaScript)

### 2.5.1 Get the version

### 2.5.2 Create a new project

### 2.5.3 Get the list of projects

### 2.5.4 Create a new metric

### 2.5.5 Get the list of metrics

### 2.5.6 Add a measure

### 2.5.7 Delete a measure

### 2.5.8 Get the measures

### 2.5.9 Delete a project





## Usage

I'll consider the example of recording one's room temperature to illustrate the usage of RunRecorder.

### Through the Web API

One can send commands to the Web API as follow (`<action>` and `<data>` are explained in the following sections):

*  using [Curl](https://curl.se/) from the command line:
```
curl -d 'action=<action>&<data>' -H "Content-Type: application/x-www-form-urlencoded" -X POST https://localhost/RunRecorder/api.php
```
* using the provided shell script from the command line:
For ease of use, a shell script is available: `Repos/RunRecorder/WebAPI/runrecorder.sh`. Replace in this script `https://localhost/RunRecorder/` with the URL to your copy of the Web API, ensure it's executable (for example, `chmod +x Repos/RunRecorder/WebAPI/runrecorder.sh`) and accessible (for example, add `export PATH=$PATH:Repos/RunRecorder/WebAPI/` in your `~/.profile`). Then you'll be able to send command to the Web API as follow: `runrecorder.sh <action> '<data>'`.
* if you use C:
You should consider using the RunRecorder C library (see next section), but you can also do it as follow:
```
#include <stdio.h>
#include <curl/curl.h>
int main() {
  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://localhost/RunRecorder/api.php");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "action=<action>&<data>");
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n", 
              curl_easy_strerror(res));
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
  return 0;
}
```
* if you use [JavaScript](https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest/send):
```
var xhr = new XMLHttpRequest();
xhr.open("POST", 'https://localhost/RunRecorder/api.php', true);
xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
xhr.onreadystatechange = function() {
  if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {
    console.log(this.responseText);
  }
}
xhr.send("action=<action>&<data>");
```
* if you use Python (I feel really sorry for you):
```
import requests
url = 'https://localhost/RunRecorder/api.php'
payload = {'action':'<action>',<data>}
headers = {'content-type': 'application/x-www-form-urlencoded'}
r = requests.post(url, data=payload, headers=headers)
print(str(r.text))
```

The API returns, when the request was successfull:
```
{"ret":"0", ...}
```
when the success failed:
```
{"errMsg":"something went wrong","ret":"1"}
```

### Check the version of the database

A first simple test to check if everything is working fine is to request the version of the database.

*The database version is automatically checked and the database is upgraded if needed by the API and C library.*

```
<action>: version
```

Example using the shell script:
```
> runrecorder.sh version
{"version":"01.00.00","ret":"0"}
```

### Help command

When using th Web API, if you have forgotten the list of available command, you can get them with the following command:

```
<action>: help
```

Example using the shell script:
```
> runrecorder.sh help
{"ret":"0","actions":"version, add_project&label=..., projects, add_metric&project=...&label=...&default=..., metrics&project=..., add_measure&project=...&...=...&..., delete_measure&measure=..., csv&project=..., flush&project=..."}
```

### Create a new project

In RunRecorder, data are grouped by projects. To start recording data, the first thing you need to do is to create the project they relate to.

```
<action>: add_project
<data>: label=<the project's name>
```

*The project's name must respect the following pattern: `/^[a-zA-Z][a-zA-Z0-9_]*$/`.*

Example using the shell script:
```
> runrecorder.sh add_project "label=RoomTemperature"
{"ret":"0"}
```


#### Get the projects

You can get the list of projects.

```
<action>: projects
```

Example using the shell script:
```
> runrecorder.sh projects
{"ret":"0","projects":{"1":"RoomTemperature"}}
```


#### Add a metric to a project

After adding a new project you'll probably want to add metrics that defines this project. In our example project there are two metrics: the date and time of the recording, and the recorded room temperature.

```
<action>: add_metric
<data>: project=<name of this metric's project>&label=<metric's name>&default=<default value of the metric>
```

*The metric's name must respect the following pattern: `/^[a-zA-Z][a-zA-Z0-9_]*$/`. The default of the metric must be one character long at least. The double quote `"`, equal sign `=` and ampersand `&` can't be used in the default value. There cannot be two metrics with the same label for the same project. A metric label can't be 'action' or 'project' (case sensitive, so 'Action' is fine).*

Example using the shell script:
```
> runrecorder.sh add_metric "project=RoomTemperature&label=Date&default=-"
{"ret":"0"}
> runrecorder.sh add_metric "project=RoomTemperature&label=Temperature&default=0.0"
{"ret":"0"}
```

You can add metrics even after having recorded data. A measurement with no value for a given metric automatically get attributed the default value of this metric.

#### Get the metrics of a project

You can get the list of metrics for a project.

```
<action>: metrics
<data>: project=<name of the project>
```

Example using the shell script:
```
> runrecorder.sh metrics "project=RoomTemperature"
{"metrics":{"1":"Date","2":"Temperature"},"ret":"0"}
```

#### Add a measurement to a project

Now you're ready to add measurements to your project!

```
<action>: add_measure
<data>: project=<name of the project>&<metric 1>=<value metric 1>&<metric 2>=<value metric 2>...
```

*The double quote `"`, equal sign `=` and ampersand `&` can't be used in the  value. A value must be at least one character long. It is not mandatory to provide a value for all the metrics of the project (if missing, the default value is used instead).*

Example using the shell script:
```
> runrecorder.sh add_measure "project=RoomTemperature&Date=2021-03-08 15:45:00&Temperature=18.5"
{"ret":"0","refMeasure":"1"}
> runrecorder.sh add_measure "project=RoomTemperature&Date=2021-03-08 16:19:00&Temperature=19.1"
{"ret":"0","refMeasure":"2"}
```

#### Delete a measure

If you've mistakenly added a measure and want to delete it, you can do so as long as you have memorised its reference (returned when you add it). Also, if an error occured when addind a measure, it may be partially saved in the database, depending on the error. RunRecorder ensures the database stays coherent even if there is an error, you can then use the partially saved data. In the other hand if you don't want to keep potentially incomplete measurement, in case of error during addition of measurement you should always try to delete it with the returned reference.

Example using the shell script:
```
> runrecorder.sh delete_measure "project=RoomTemperature&Measure=1"
{"ret":"0"}
```


#### Retrieve measurements from a project

Obviously after adding your measurements you'll want to use them. You can access directly the SQLite database with any compatible software, or you can retrieve them in CSV format using RunRecorder. 

```
<action>: csv
<data>: project=<name of the project>
```

Example using the shell script:
```
runrecorder.sh csv "project=RoomTemperature"
```

Example using the C library:
```
TODO
```

On success, the API returns, for example:
```
Date&Temperature
2021-03-08 15:45:00&18.5
2021-03-08 16:19:00&19.1
```

On failure, an error message is returned.
```
{"errMsg":"something went wrong","ret":"0"}
```

Metrics (columns) are ordered alphabetically, values (rows) are ordered by time of creation in the database. The delimiter is ampersand `&`, and the first line contains the label of metrics. All metrics of the project are present, and their default value is used in rows containing missing values.

#### Delete a project

Once you've finished collecting data for a project and want to free space in the database, you can delete the project and all the associated metrics and measurements. 

```
<action>: flush
<data>: project=<name of the project>
```

Example using the shell script:
```
runrecorder.sh flush "project=RoomTemperature"
```

Example using the C library:
```
TODO
```

On success, the API returns:
```
{"ret":"0"}
```

On failure, an error message is returned.
```
{"errMsg":"something went wrong","ret":"0"}
```

## License

RunRecorder, a C library and a Web API helping to interface the acquisition of textual data and their recording in a remote or local SQLite database.
Copyright (C) 2021  Pascal Baillehache

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

