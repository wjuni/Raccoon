//
//  PythonHttpsRequest.cpp
//  Raccoon_Raspi
//
//  Created by 임휘준 on 2017. 8. 9..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#include "PythonHttpsRequest.hpp"
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif

//static std::string code_send ="\nresponse = urllib2.urlopen(urllib2.Request(url, urllib.urlencode(values)))\ndata = response.read()\n"; //"print(response.read())\n";


PythonHttpsRequest::PythonHttpsRequest(std::string uri) {
    this->_uri = uri;
    Py_SetProgramName((char *)"Raccoon_Python");
    Py_Initialize();
    this->urllib = PyImport_Import(PyString_FromString("urllib"));
    this->urllib2 = PyImport_Import(PyString_FromString("urllib2"));
    //PyRun_SimpleString("import urllib, urllib2");

}

/*
Send a request to python interpreter to get json structure
Then receive the json structure from the web frontend
*/
void PythonHttpsRequest::sendData(json *data) {
    //std::string code = buildPythonCode(data);
    //PyRun_SimpleString(code.c_str());
    this->Response = ReturnResponse(data);
}

void PythonHttpsRequest::ReceiveData() {
	this->ReceivedData = std::string(PyString_AsString(PyObject_CallObject(PyObject_GetAttrString(response, (char *)"read"), NULL)));
}

PythonHttpsRequest::~PythonHttpsRequest() {
    Py_Finalize();
}

//std::string PythonHttpsRequest::buildPythonCode(json *data){
PyObject* PythonHttpsRequest::GetResponse(json *data){
    /*
    std::ostringstream stringStream;
    stringStream << "url = '";
    stringStream << _uri;
    stringStream << "'\nvalues = {'data' : '";
    stringStream << data->dump();
    stringStream << "'}\n";
    */
    PyObject* url = PyString_FromString( (char *)(_uri.c_str()) );
    PyObject* values = PyDict_New();
    PyDict_SetItemString(values, (char *)"data", PyString_FromString( (data->dump()).c_str() ));
    PyObject* urlencode = PyObject_CallObject(PyObject_GetAttrString(this->urllib, (char *)"urlencode"), PyTuple_Pack(1, values));
    PyObject* request = PyObject_CallObject(PyObject_GetAttrString(this->urllib2, (char *)"Request"), PyTuple_Pack(2, url, urlencode));
    PyObject* response = PyObject_CallObject(PyObject_GetAttrString(this->urllib2, (char *)"urlopen"), PyTuple_Pack(1, request));
//    PyObject* read = PyObject_CallObject(PyObject_GetAttrString(response, (char *)"read"), NULL);
//    return read;
    return response;
    /*
    stringStream << code_send;
    return stringStream.str();
    */
}
