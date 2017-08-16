# WMI

This is a Windows Management Instrumentation (WMI) based System Monitor for Windows that checks all running processes and send their information to a cloud-based server. The server does actions based on what it receives from the clients.

The project was made in C++ using the CPPRESTSDK library.

## Requeriments

* Windows
* Visual Studio
* vcpkg
* cpprestsdk
* libcurl

## Running

* Install all the dependencies (vcpkg, cpprestsdk, libcurl)
* Create project on Visual Studio
* Set the *server/server_config.xml* with server's configuration (must follow the given pattern)
* Run the *server/server.cpp* (must execute on same folder the *server_config.xml* is)
* Set the *client/config.txt* with client's data (must follow given pattern)
* Run the *client/client.cpp* (must execute on same folder the *config.txt* is)

The client will send to the server all information about its running processes.

## Example Build

The *build* folder has compiled versions of this project, read to be executed.