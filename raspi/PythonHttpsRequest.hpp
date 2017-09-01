//
//  PythonHttpsRequest.hpp
//  Raccoon_Raspi
//
//  Created by 임휘준 on 2017. 8. 9..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#ifndef PythonHttpsRequest_hpp
#define PythonHttpsRequest_hpp

#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif
#include <iostream>
#include "json.hpp"
using json = nlohmann::json;

class PythonHttpsRequest {
private:
    std::string _uri;
    //std::string buildPythonCode(json *data);
    PyObject* GetResponse(json *data);
    PyObject* urllib;
    PyObject* urllib2;
    PyObject* Response;
    std::string ReceivedData;
public:
    PythonHttpsRequest(std::string uri);
    ~PythonHttpsRequest();
    void sendData(json *data);
    void ReceiveData();
};
#endif /* PythonHttpsRequest_hpp */
