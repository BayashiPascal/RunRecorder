# RunRecorder

RunRecorder is a C library and a Web API to automate the recording of data in a sqlite database, locally or remotely.

## Install

Download this repository into a folder of your choice, in the examples below I'll call it `Repos`.
```
cd Repos
git clone https://github.com/BayashiPascal/RunRecorder.git
```

### Wep API

Copy the Web API to your local server or web server. For example, if you're using [LAMP](https://en.wikipedia.org/wiki/LAMP_(software_bundle)) it should look like this:
```
sudo ln -s Repos/RunRecorder/WebAPI /var/www/html/RunRecorder
```

That's all you need to do. The database will be automatically setup the first time the API will be accessed.

If you use a web server, be aware that the API doesn't implement any kind of security. Anyone knowing the URL of the API will be able to interact with it. If you have security concerns, use the API on a secured local network, or use it after modifying the code of the API according to your security policy.

### C library



## Usage

### Wep API

#### Check the version of the database

A first simple test to check if the API is working fine is to request the version of the database.

* Using curl from the command line: 
```
curl -d 'action=version' -H "Content-Type: application/x-www-form-urlencoded" -X POST https://localhost/RunRecorder/api.php
```

On success, the API returns, for example, `{"version":"1.0","ret":0}`. On failure, an error message is returned.

#### Create a new project

In the database, data are grouped by projects. To start recording data, the first thing you need to do is create the project they relate to.

* Using curl from the command line: 
```
curl -d 'action=add_project&label=My body weight' -H "Content-Type: application/x-www-form-urlencoded" -X POST https://localhost/RunRecorder/api.php
```

On success, the API returns, for example, `{"refProject":1,"ret":0}`. On failure, an error message is returned.

#### Add a metric to a project



### C library


## License

RunRecorder, a C library and a Web API to save results from experiments in a sqlite database.
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

