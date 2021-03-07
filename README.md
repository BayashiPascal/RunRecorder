# RunRecorder

RunRecorder is a C library and a Web API to automate the recording of textual data in a Sqlite database, locally or remotely.

## Install

Download this repository into a folder of your choice, in the examples below I'll call it `Repos`.
```
cd Repos
wget https://github.com/BayashiPascal/RunRecorder/archive/main.zip
unzip main.zip
mv RunRecorder-main RunRecorder
rm main.zip
```

### Web API

Install the Web API to your local server or web server. For example, if you're using [LAMP](https://en.wikipedia.org/wiki/LAMP_(software_bundle)) it should look something like this:
```
sudo ln -s Repos/RunRecorder/WebAPI /var/www/html/RunRecorder
```

That's all you need to do. The database will be automatically setup the first time the API will be accessed.

*If you use a web server, be aware that the API doesn't implement any kind of security mechanism. Anyone knowing the URL of the API will be able to interact with it. If you have security concerns, use the API on a secured local network, or use it after modifying the code of the API according to your security policy.*

### C library

Compile the C library as follow (you'll need the `gcc` compiler to be installed, and the Sqlite library will automatically be downloaded into `Repos/RunRecorder/C/`):

```
cd Repos/RunRecorder/C
make all
```

Then you'll be able to compile your code using RunRecorder's C library (for example `main.c`) with the following Makefile:

```
main: main.o
	gcc main.o Repos/RunRecorder/C/runrecorder.o Repos/RunRecorder/C/sqlite3.o -lm -lpthread -ldl -o main 

main.o: main.c
	gcc -IRepos/RunRecorder/C -c main.c 
```

## Usage

I'll consider the example of recording one's room temperature to illustrate the usage of RunRecorder.

### Sending commands to the Web API

One can send commands to the Web API using [Curl](https://curl.se/) as follow:

```
curl -d 'action=version' -H "Content-Type: application/x-www-form-urlencoded" -X POST https://localhost/RunRecorder/api.php
```

A shortcut to this command is available as a shell script: `Repos/RunRecorder/WebAPI/runrecorder.sh`. Replace `https://localhost/RunRecorder/` with the URL to your copy of the Web API, ensure it's executable (`chmod +x Repos/RunRecorder/WebAPI/runrecorder.sh`) and accessible (for example, add `export PATH=$PATH:Repos/RunRecorder/WebAPI/` in your `~/.profile`). Then you'll be able to send command to the Web API as follow: `runrecorder.sh command data`, where `command` and `data` are explained below.

### Creating a RunRecorder instance in C

TODO

### Check the version of the database

A first simple test to check if everything is working fine is to request the version of the database.

*The database version is automatically checked and the database is upgraded if needed by the API and C library.*

#### Web API

From the command line: 
```
runrecorder.sh version
```

On success, the API returns, for example, `{"version":"01.00.00","ret":0}`. On failure, an error message is returned.

#### C library

TODO

### Create a new project

In RunRecorder, data are grouped by projects. To start recording data, the first thing you need to do is to create the project they relate to.

#### Web API

From the command line: 
```
runrecorder.sh add_project "label"="Room temperature"
```

On success, the API returns, for example, `{"refProject":1,"ret":0}`. Note the value of `refProject`, you will need it to record measure later. On failure, an error message is returned.

*The double quote `"` can't be used in the label.*

#### C library

TODO

#### Add a metric to a project



### C library


## License

RunRecorder, a C library and a Web API to automate the recording of textual data in a Sqlite database, locally or remotely.
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

