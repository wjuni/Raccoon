//
//  ServerCommunicator.cpp
//  Raccoon_Raspi
//
//  Created by 임휘준 on 2017. 8. 10..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#include "ServerCommunicator.hpp"
#include "json.hpp"

ServerCommunicator::ServerCommunicator(std::string uri) {
    this->_uri = uri;
    phr = new PythonHttpsRequest(uri);
}

ServerCommunicator::~ServerCommunicator() {
    if (this->_running) {
        this->_running = false;
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
        json r = phr->getData();
        std::cout << "JSON = " << r.dump(4) << std::endl;
        ServerRecvContext *data = sc->scr;
        data->tid = r["tid"];
        data->multi = r["multi"];
        data->yellow = r["yellow"];
        data->recovery = r["recovery"];
        data->cond = r["cond"];
        data->param = r["param"];
        
        usleep(750*1000);
        
    }
}
