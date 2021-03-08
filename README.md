# RunRecorder

RunRecorder is a C library and a Web API to automate the recording of textual data in a SQLite database, locally or remotely.

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

Install the Web API to your local server or web server by copying or linking `Repos/RunRecorder` to an URL of your choice on the server. For example, if you're using [Apache](https://en.wikipedia.org/wiki/Apache_HTTP_Server) and want to access the API via `https://localhost/RunRecorder/api.php` it should look something like this:
```
sudo ln -s Repos/RunRecorder/WebAPI /var/www/html/RunRecorder
```

*If you use a web server, be aware that the API doesn't implement any kind of security mechanism. Anyone knowing the URL of the API will be able to interact with it. If you have security concerns, use the API on a secured local network, or use it after modifying the code of the API according to your security policy.*

### C library

You'll need the `gcc` compiler to be installed on your system ([gcc.gnu.org](https://gcc.gnu.org/)), and the [SQLite](https://www.sqlite.org/index.html) library will automatically be downloaded into `Repos/RunRecorder/C/`. Compile the RunRecorder C library as follow:

```
cd Repos/RunRecorder/C
make all
```

Then you'll be able to compile your code (for example `main.c`) using the RunRecorder C library with the following Makefile:

```
main: main.o
	gcc main.o Repos/RunRecorder/C/runrecorder.o Repos/RunRecorder/C/sqlite3.o -lm -lpthread -ldl -o main 

main.o: main.c
	gcc -IRepos/RunRecorder/C -c main.c 
```

## Usage

I'll consider the example of recording one's room temperature to illustrate the usage of RunRecorder.

### Through the Web API

One can send commands to the Web API as follow (`<action>` and `<data>` are explained in the following sections):

*  using [Curl](https://curl.se/) from the command line:
```
curl -d 'action=<action>&<data>' -H "Content-Type: application/x-www-form-urlencoded" -X POST https://localhost/RunRecorder/api.php
```
* using the provided shell script from the command line:
For ease of use, a shell script is available: `Repos/RunRecorder/WebAPI/runrecorder.sh`. Replace in this script `https://localhost/RunRecorder/` with the URL to your copy of the Web API, ensure it's executable (for example, `chmod +x Repos/RunRecorder/WebAPI/runrecorder.sh`) and accessible (for example, add `export PATH=$PATH:Repos/RunRecorder/WebAPI/` in your `~/.profile`). Then you'll be able to send command to the Web API as follow: `runrecorder.sh <action> <data>`.
* if you use C:
You should consider using the RunRecorder C libray (see next section), but you can also do it as follow:
```
TODO
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

### Through the C library

TODO

### Check the version of the database

A first simple test to check if everything is working fine is to request the version of the database.

*The database version is automatically checked and the database is upgraded if needed by the API and C library.*

```
<action>: version
```

Example using the shell script:
```
runrecorder.sh version
```

Example using the C library:
```
TODO
```

On success, the API returns, for example:
```
{"version":"01.00.00","ret":0}
```

On failure, an error message is returned.
```
{"errMsg":"something went wrong","ret":0}
```

### Create a new project

In RunRecorder, data are grouped by projects. To start recording data, the first thing you need to do is to create the project they relate to.

```
<action>: add_project
<data>: the project's name
```

*The double quote `"` can't be used in the project's name.*

Example using the shell script:
```
runrecorder.sh add_project "label"="Room temperature"
```

Example using the C library:
```
TODO
```

On success, the API returns, for example:
```
{"refProject":1,"ret":0}
```
Memorise the value of `refProject`, you will need it to record measure later.

On failure, an error message is returned.
```
{"errMsg":"something went wrong","ret":0}
```

#### Add a metric to a project



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

