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
    int bot_id;             // Raccoon bot ID number
    int record_time;        // Passed time after the robot boots
    int bot_status;         // 0 : default, 1 : On-line, 2 : Task on progress, 3 : error occured
    int damage_ratio;       // Damage ratio from the Video (Picture from the cammera)
    int acc_distance;       // Accumulated distance;
    int task_id;            // Task id which is on progress (Should be gotten from the board)
    int gps_lat, gps_lon;   // Gps values
    int bot_battery;        // Voltage of the battery
    int bot_speed;          // Speed of the robot gotten from the GPS
    int bot_version;        // Firmware version
    char repair_module[8];  // have a segfault issue on Raspberry Pi with std::string
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
    void GetData(ServerCommContext *data); // Get data from the web page
    inline bool isRunning();
    inline PythonHttpsRequest *getPhr();
    inline ServerCommContext *getScc();
};

#endif /* ServerCommunicator_hpp */
