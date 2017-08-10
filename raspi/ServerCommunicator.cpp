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
}

void ServerCommunicator::start(ServerCommContext *scc) {
    this->scc = scc;
    this->_running = true;
    nw_thd = std::thread(handleTransmission, this);
}

void ServerCommunicator::stop() {
    this->_running = false;
}

inline bool ServerCommunicator::isRunning() { return this->_running; }

inline PythonHttpsRequest * ServerCommunicator::getPhr() { return this->phr; }

inline ServerCommContext * ServerCommunicator::getScc() { return this->scc; }

void ServerCommunicator::handleTransmission(void *communicator) {
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
