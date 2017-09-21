//
//  main.cpp
//
//  Created by Hwijoon Lim on 2017. 7. 2..
//  Copyright © 2017 Hwijoon Lim. All rights reserved.
//

#include <iostream>
#include <thread>
#include <cstring>
#include <signal.h>
#include <cmath>
#include "PktProtocol.h"
#include "ArduinoCommunicator.hpp"
#include "ServerCommunicator.hpp"
#include "WebcamProcessor.hpp"

using namespace std;

/*
 * Initialize the context struct with the values from the board.
 * The context struct will be sent to the server later.
 */

ArduinoCommunicator arduino("/dev/ttyAMA0");
const string SERVER_ADDR = "https://raccoon.wjuni.com/ajax/report.php";
ServerCommunicator server(SERVER_ADDR);
ServerCommContext context;
webcam::WebcamProcessor wp;

void arduino_packet_handler(PktRaspi *pkt);
void video_feedback_handler(webcam::VideoFeedbackParam wfp);
void finish(int signal);

int main(int argc, const char * argv[]) {
    
    signal(SIGINT, finish);
    
    /* Arduino */
    arduino.begin(115200);
    arduino.send(buildPktArduinoV2(1<<8, 0, 0, 0, 0)); // notify boot complete
    
    /* Server */
    memset(&context, 0, sizeof(ServerCommContext));
    context.bot_id = 1;
    context.bot_speed = 637;
    context.bot_battery = 95;
    context.acc_distance = 239;
    context.bot_version = 11;
    server.start(&context);
    
    /* Webcam */
    if(argc >=2 && strcmp(argv[1], "gui") == 0)
        wp.setX11Support(true);
    wp.start(webcam::WEBCAM, video_feedback_handler);

    while(true) {
        arduino.recv(arduino_packet_handler);
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    
    return 0;
}

void arduino_packet_handler(PktRaspi *pkt) {
    cout << "[Arduino] Got Packet" << endl;
    
    cout << "GPS Lat : " << pkt->gps_lat << endl;
    cout << "GPS Lon : " << pkt->gps_lon << endl;
    cout << "GPS Fix : " << (int)pkt->gps_fix << endl;
    context.bot_id = 1;
    context.bot_status = 1;
    //   context.damage_ratio =
    //   context.acc_distance =
    //   context.task_id =
    context.gps_lat = pkt->gps_lat;
    context.gps_lon = pkt->gps_lon;
    
    /*
     * If voltage of Raccoon is greater than 12V, consider as 100%
     * If voltage of Raccoon is greater than 11V and less than 12V, consider as 50%
     * Otherwise, consider as 25%
     */
    context.bot_battery = pkt->voltage>=12 ? 100 : pkt->voltage >= 11 ? 50 : 25;
    
    context.bot_speed = pkt->gps_spd;
    context.bot_version = 10;
}
double p=3.1415926535;
double theta;
double d=189;
double r=100;
double right, left;
void video_feedback_handler(webcam::VideoFeedbackParam wfp) {
    cout << "Video Handler Called, wfp = " << wfp.beta_hat << ", " << wfp.x_dev << ", " << wfp.vector_diff_x << ", " << wfp.vector_diff_y << endl;
    if(wfp.x_dev==0){
        theta=atan(wfp.beta_hat);
        if(theta>0){
            left=d*sin(theta)+r;
            right=r-d*sin(theta);
            right=100*right/left;
            left=100;
        }
        else{
            right=d*sin(theta)+r;
            left=r-d*sin(theta);      
            left=100*left/right;
            right=100;    
        }
    }
    else{
        theta=p/2-atan(wfp.vector_diff_y/wfp.vector_diff_x);
        double cof=wfp.vector_diff_x/cos(atan(wfp.vector_diff_y/wfp.vector_diff_x));
        r=cof*10
        if(theta>0){
            left=d*sin(theta)+r;
            right=r-d*sin(theta);
            right=100*right/left;
            left=100;
        }
        else{
            right=d*sin(theta)+r;
            left=r-d*sin(theta); 
            left=100*left/right;
            right=100;            
        }

        r=100;
        theta=atan(wfp.vector_diff_y/wfp.vector_diff_x)-(p/2-atan(wfp.beta_hat));
        if(theta>0){
            left=d*sin(theta)+r;
            right=r-d*sin(theta);
            right=100*right/left;
            left=100;
        }
        else{
            right=d*sin(theta)+r;
            left=r-d*sin(theta);   
            left=100*left/right;
            right=100;          
        }
    }
    arduino.send(buildPktArduinoV2(0, left, left, right, right));
 
}
void finish(int signal) {
    exit(0);
}
