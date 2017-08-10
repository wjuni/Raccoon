//
//  ServerCommunicator.hpp
//  Raccoon_Raspi
//
//  Created by 임휘준 on 2017. 8. 10..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#ifndef ServerCommunicator_hpp
#define ServerCommunicator_hpp

#include <iostream>
#include <thread>
#include "PythonHttpsRequest.hpp"

typedef struct {
    int bot_id, record_time, bot_status, damage_ratio, acc_distance, task_id, gps_lat, gps_lon, bot_battery, bot_speed;
    std::string repair_module;
} ServerCommContext;

class ServerCommunicator {
private:
    bool _running = false;
    std::string _uri;
    std::thread nw_thd;
    static void handleTransmission(void *data);
    PythonHttpsRequest *phr;
    ServerCommContext *scc;
    
public:
    ServerCommunicator(std::string uri);
    ~ServerCommunicator();
    void start(ServerCommContext *sco);
    void stop();
    inline bool isRunning();
    inline PythonHttpsRequest *getPhr();
    inline ServerCommContext *getScc();
};

#endif /* ServerCommunicator_hpp */
