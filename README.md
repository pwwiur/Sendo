# Sendo
This compiler is made for [Sendo](https://www.amirforsati.ir/doc/7) language. It has a runnable server which can be used by third party libraries to connect to the compiler by various programming languages.

## Requirements
 - C++ compiler
 - Boost library
 - MySQL database
 
## Installation guide
By editing configurations file, You will able to compile Sendo server for your machine. And then you can execute it to run the Sendo server. After running the Sendo server you can connect it by thirt party libraries to execute queries of Sendo server.

## Usage
Sendo server is a tcp socket server running on a specific port that can be connected by third party libraries to execute commands. Some of provided libraries consist of:

 - Javascript (In progress)

## Example
Here is a javascript example of using Sendo which gets the commands from other devices by wifi io stream and passing them to Sendo server to execute them and answer the devices.

    var Sendo = require('sendo');
    var wifi = require('wifi');
    var io = new wifi.inputstream();
    var sendo = new Sendo.Connector("localhost", 8569);

    io.on("request", function(request) {
        sendo.query(request, function(response) {
            console.log(response.toString());
        });
    });
