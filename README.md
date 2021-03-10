# RunRecorder

RunRecorder is a C library and a Web API to automate the recording of textual data in a SQLite database, locally or remotely.

RunRecorder is based on [SQLite](https://www.sqlite.org/) and [Curl](https://curl.se/).

It also uses the Try/Catch implementation in C described [here](https://github.com/BayashiPascal/TryCatchC).

## Installation

Download this repository into a folder of your choice, in the examples below I'll call it `Repos`.
```
cd Repos
wget https://github.com/BayashiPascal/RunRecorder/archive/main.zip
unzip main.zip
mv RunRecorder-main RunRecorder
rm main.zip
```

Depending on how you plan to use RunRecorder you'll have to install the Web API, or the C library, or both.

### Web API

Install the Web API to your local server or web server by copying or linking `Repos/RunRecorder/WebAPI` to an URL of your choice on the server. For example, if you're using [LAMP](https://en.wikipedia.org/wiki/LAMP_(software_bundle)) and want to access the API via `https://localhost/RunRecorder/api.php` it should look something like this:
```
sudo ln -s Repos/RunRecorder/WebAPI /var/www/html/RunRecorder
```

*If you use a web server, be aware that the API doesn't implement any kind of security mechanism. Anyone knowing the URL of the API will be able to interact with it. If you have security concerns, use the API on a secured local network, or use it after modifying the code of the API according to your security policy.*

### C library

Compile the RunRecorder C library as follow:

```
cd Repos/RunRecorder/C
make all
```

*The [SQLite](https://www.sqlite.org/) and [Curl](https://curl.se/) libraries will automatically be downloaded into `Repos/RunRecorder/C/` during installation. You'll be asked for your password once during installation.*

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

### Through the C library

```
#include "runrecorder.h"

int main() {

  char const* pathDb = "./runrecorder.db";
  char const* pathApi = "https://localhost/RunRecorder/api.php";

  // Create the RunRecorder instance
  struct RunRecorder* recorder;

  Try {
    // Give pathApi in argument if you want to use the Web API instead
    // of a local file
    recorder = RunRecorderCreate(pathDb);
  } Catch(TryCatchException_CreateTableFailed) {
    exit(EXIT_FAILURE);
  // Other Catch omitted...
  } EndTry;

  // Your code using RunRecorder here (see below for details)
  // ...

  // Free memory
  RunRecorderFree(&recorder);
  return EXIT_SUCCESS;

}
```

In examples below the Try/Catch blocks are omitted. Refer to `Repos/RunRecorder/C/main.c` or the comments in the code for detailed information.

You can compile code using the RunRecorder C library with the following Makefile:

```
main: main.o
	gcc main.o Repos/RunRecorder/C/runrecorder.o Repos/RunRecorder/C/sqlite3.o -lm -lpthread -ldl -o main 

main.o: main.c
	gcc -IRepos/RunRecorder/C -c main.c 
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

Example using the C library:
```
#include <stdio.h>
#include "runrecorder.h"

int main() {

  char const* pathDb = "./runrecorder.db";
  char const* pathApi = "https://localhost/RunRecorder/api.php";

  // Create the RunRecorder instance
  struct RunRecorder* recorder;
  // Give pathApi in argument if you want to use the Web API instead
  // of a local file
  recorder = RunRecorderCreate(pathDb);

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

// Result:
// 01.00.00
```

### Create a new project

In RunRecorder, data are grouped by projects. To start recording data, the first thing you need to do is to create the project they relate to.

```
<action>: add_project
<data>: label=<the project's name>
```

*The double quote `"`, equal sign `=` and ampersand `&` can't be used in the project's name.*

Example using the shell script:
```
runrecorder.sh add_project "label=Room temperature"
{"refProject":1,"ret":"0"}
```

Example using the C library:
```
TODO
```

Memorise the value of `refProject`, you will need it to record measure later.

#### Get the projects

You can get the list of projects.

```
<action>: projects
```

Example using the shell script:
```
runrecorder.sh projects
```

Example using the C library:
```
TODO
```

On success, the API returns, for example:
```
{"ret":"0","1":"Room temperature"}
```

On failure, an error message is returned.
```
{"errMsg":"something went wrong","ret":"0"}
```

#### Add a metric to a project

After adding a new project you'll probably want to add metrics that defines this project. In our example project there are two metrics: the date and time of the recording, and the recorded room temperature.

```
<action>: add_metric
<data>: project=<reference of this metric's project>&label=<metric's name>&default=<default value of the metric>
```

*The label of the metric must contain only characters in a-zA-Z0-9. The label and default of the metric must be one character long at least. The double quote `"`, equal sign `=` and ampersand `&` can't be used in the default value. There cannot be two metrics with the same label for the same project. A metric label can't be 'action' or 'project' (case sensitive, so 'Action' is fine).*

Example using the shell script:
```
runrecorder.sh add_metric "project=1&label=Date&default=-"
runrecorder.sh add_metric "project=1&label=Temperature&default=0.0"
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

You can add metrics even after having recorded data. A measurement with no value for a given metric automatically get attributed the default value of this metric.

#### Get the metrics of a project

You can get the list of metrics for a project.

```
<action>: metrics
<data>: project=<reference of the project>
```

Example using the shell script:
```
runrecorder.sh metrics "project=1"
```

Example using the C library:
```
TODO
```

On success, the API returns, for example:
```
{"1":"Date","2":"Temperature","ret":"0"}
```

On failure, an error message is returned.
```
{"errMsg":"something went wrong","ret":"0"}
```

#### Add a measurement to a project

Now you're ready to add measurement to your project!

```
<action>: add_measure
<data>: project=<reference of the project>&<metric 1>=<value metric 1>&<metric 2>=<value metric 2>...
```

*The double quote `"`, equal sign `=` and ampersand `&` can't be used in the  value. A value must be at least one character long. It is not mandatory to provide a value for all the metrics of the project (if missing, the default value is used instead).*

Example using the shell script:
```
runrecorder.sh add_measure "project=1&Date=2021-03-08 15:45:00&Temperature=18.5"
runrecorder.sh add_measure "project=1&Date=2021-03-08 16:19:00&Temperature=19.1"
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

Unknown metrics are ignored.

#### Retrieve measurements from a project

Obviously after adding your measurements you'll want to use them. You can access directly the SQLite database with any compatible software, or you can retrieve them in CSV format using RunRecorder. 

```
<action>: measures
<data>: project=<reference of the project>
```

Example using the shell script:
```
runrecorder.sh measures "project=1"
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
<data>: project=<reference of the project>
```

Example using the shell script:
```
runrecorder.sh flush "project=1"
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

RunRecorder, a C library and a Web API to automate the recording of textual data in a SQLite database, locally or remotely.
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

