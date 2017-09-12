//
//  ServerCommunicator.cpp
//  Raccoon_Raspi
//
//  Created by 임휘준 on 2017. 8. 10..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#include "ServerCommunicator.hpp"
#include "json.hpp"
#include <unistd.h>
#include <cstring>


#include <stdio.h>

ServerCommunicator::ServerCommunicator(std::string uri) {
    this->_uri = uri;
    phr = new PythonHttpsRequest(uri);
}

ServerCommunicator::~ServerCommunicator() {
    if (this->_running) {
        //this->_running = false;
        nw_thd.join();
    }
    delete phr;
    delete scr;
}

void ServerCommunicator::start(ServerCommContext *scc) {
    this->scc = scc;
    this->scr = new ServerRecvContext;
    this->_running = true;
    nw_thd = std::thread(handleTransmission, this);
}

void ServerCommunicator::stop() {
    this->_running = false;
}

inline bool ServerCommunicator::isRunning() { return this->_running; }

inline PythonHttpsRequest * ServerCommunicator::getPhr() { return this->phr; }

inline ServerCommContext * ServerCommunicator::getScc() { return this->scc; }

/*
void ServerCommunicator::SetScc(const ServerCommContext* context){
    this->bot_id = context->bot_id;
    this->record_time = context->record_time;
    this->bot_status = context->bot_status;
    this->damage_ratio = context->damage_ratio;
    this->acc_distance = context->acc_distance;
    this->task_id = context->task_id;
    this->gps_lat = context->gps_lat;
    this->gps_lon = context->gps_lon;
    this->bot_battery = context->bot_battery;
    this->bot_speed = context->bot_speed;
    this->bot_version = context->bot_version;
    strcpy(this->repair_module, context->repair_module);
}
*/

void ServerCommunicator::handleTransmission(void* communicator) {
    ServerCommunicator *sc = (ServerCommunicator *)communicator;
    ServerCommContext *scc = sc->getScc();
    PythonHttpsRequest *phr = sc->getPhr();
    while(sc->isRunning()) {
        json j;
        j["bid"] = scc->bot_id;
        j["sta"] = scc->bot_status;
        j["dam"] = scc->damage_ratio;
        j["dis"] = scc->acc_distance;
        j["tid"] = scc->task_id;
        j["lat"] = scc->gps_lat;
        j["lon"] = scc->gps_lon;
        j["bat"] = scc->bot_battery;
        j["rep"] = scc->repair_module;
        j["spd"] = scc->bot_speed;
        j["ver"] = scc->bot_version;
        phr->sendData(&j);
        usleep(750*1000);
    }
}

void ServerCommunicator::GetandParseData() {
    PythonHttpsRequest *phr = this->getPhr();
    ServerRecvContext *data = this->scr;
    std::cout << "Inside the GetandParseData\n";
//    phr->ReceiveData();
//    std::cout << "After Receive data\n";
    std::cout << phr->getData() << "\nGetting Data\n";

    std::vector<std::string> tokens = split(phr->getData(), '"');
    std::cout << "Tokenized" << std::endl;
    int temp = atoi(tokens[2].substr(1, tokens[2].length()-2).c_str());
    printf("%d\n", temp);
    std::cout << "ATOI Succeeded\n";
    data->tid = atoi(tokens[2].substr(1, tokens[2].length()-2).c_str());
    std::cout << "Step1\n";
    data->multi = atoi(tokens[4].substr(1, tokens[4].length()-2).c_str());
    data->yellow = atoi(tokens[6].substr(1, tokens[6].length()-2).c_str());
    data->recovery = atoi(tokens[8].substr(1, tokens[8].length()-2).c_str());
    data->cond = atoi(tokens[10].substr(1, tokens[10].length()-2).c_str());
    data->param = tokens[13];
}

void ServerCommunicator::PrintData()    {
    ServerRecvContext *data = this->scr;
    std::cout << "Starting to receive data\n";
    std::cout << "tid : " << data->tid << std::endl;
    std::cout << "multi : " << data->multi << std::endl;
    std::cout << "yellow : " << data->yellow << std::endl;
    std::cout << "recovery : " << data->recovery << std::endl;
    std::cout << "cond : " << data->cond << std::endl;
    std::cout << "param : " << data->param << std::endl;
    std::cout << "End of the data\n";
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}
