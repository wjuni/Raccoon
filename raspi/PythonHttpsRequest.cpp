//
//  PythonHttpsRequest.cpp
//  Raccoon_Raspi
//
//  Created by 임휘준 on 2017. 8. 9..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#include "PythonHttpsRequest.hpp"
#include <Python/Python.h>

static std::string code_send ="\nresponse = urllib2.urlopen(urllib2.Request(url, urllib.urlencode(values)))\n";

PythonHttpsRequest::PythonHttpsRequest(std::string uri) {
    this->_uri = uri;
    Py_SetProgramName((char *)"Raccoon_Python");
    Py_Initialize();
    PyRun_SimpleString("import urllib, urllib2");
}

void PythonHttpsRequest::sendData(json *data) {
    std::string code = buildPythonCode(data);
    PyRun_SimpleString(code.c_str());
}

PythonHttpsRequest::~PythonHttpsRequest() {
    Py_Finalize();
}

std::string PythonHttpsRequest::buildPythonCode(json *data){
    std::ostringstream stringStream;
    stringStream << "url = '";
    stringStream << _uri;
    stringStream << "'\nvalues = ";
    stringStream << data->dump();
    stringStream << code_send;
    return stringStream.str();
}
