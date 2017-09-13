//
//  PythonHttpsRequest.cpp
//  Raccoon_Raspi
//
//  Created by 임휘준 on 2017. 8. 9..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#include "PythonHttpsRequest.hpp"
#include <iostream>

PythonHttpsRequest::PythonHttpsRequest(std::string uri) {
    this->_uri = uri;
    Py_SetProgramName((char *)"Raccoon_Python");
    Py_Initialize();
    this->urllib = PyImport_Import(PyString_FromString("urllib"));
    this->urllib2 = PyImport_Import(PyString_FromString("urllib2"));
}

/*
 * Send a request to python interpreter to get json structure
 * Then receive the json structure from the web frontend
 */
void PythonHttpsRequest::sendData(json *data) {
    this->Response = GetResponse(data);
}

PythonHttpsRequest::~PythonHttpsRequest() {
    Py_Finalize();
}

PyObject* PythonHttpsRequest::GetResponse(json *data){
    PyObject* url = PyString_FromString( (char *)(_uri.c_str()) );
    PyObject* values = PyDict_New();
    PyDict_SetItemString(values, (char *)"data", PyString_FromString( (data->dump()).c_str() ));
    PyObject* urlencode = PyObject_CallObject(PyObject_GetAttrString(this->urllib, (char *)"urlencode"), PyTuple_Pack(1, values));
    PyObject* request = PyObject_CallObject(PyObject_GetAttrString(this->urllib2, (char *)"Request"), PyTuple_Pack(2, url, urlencode));
    PyObject* response = PyObject_CallObject(PyObject_GetAttrString(this->urllib2, (char *)"urlopen"), PyTuple_Pack(1, request));
    return response;
}

std::string PythonHttpsRequest::getData() {
    return std::string(PyString_AsString(PyObject_CallObject(PyObject_GetAttrString(this->Response, (char *)"read"), NULL)));
}
