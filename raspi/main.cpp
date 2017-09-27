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

ArduinoCommunicator arduino("/dev/serial0"); // To support both raspi2 + raspi3 (ble must be disabled on raspi3)
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
    arduino.begin(115200); // Baudrate must be 9600 or 115200
    arduino.send(buildPktArduinoV2(1<<8, 0, 0, 0, 0)); // notify boot complete
    
    /* Server */
/*    memset(&context, 0, sizeof(ServerCommContext));
    context.bot_id = 1;
    context.bot_speed = 637;
    context.bot_battery = 95;
    context.acc_distance = 239;
    context.bot_version = 11;
    server.start(&context);
    */
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

void arduino_packet_handler(PktRaspi *pkt) {/*
    cout << "[Arduino] Got Packet" << endl;
    
    cout << "GPS Lat : " << pkt->gps_lat << endl;
    cout << "GPS Lon : " << pkt->gps_lon << endl;
    cout << "GPS Fix : " << (int)pkt->gps_fix << endl;*/
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

const double d=189.0;
/* For the previous algorithm.
const double k=0.806431;
const double r_factor=15.0;
const double r_threshold=40.2542;
const double v_factor = 50.0;
double m_right, m_left;
*/

const double v_factor = 20.0;
 
void video_feedback_handler(webcam::VideoFeedbackParam wfp) {
    //cout << "Video Handler Called, wfp = " << wfp.beta_hat << ", " << wfp.x_dev << ", " << wfp.vector_diff_x << ", " << wfp.vector_diff_y << endl;
	/* Previous algorithm.
    double vx_line = wfp.vector_diff_x - wfp.x_dev, vy_line = wfp.vector_diff_y;
    double alpha = abs(atan(vy_line/(vx_line-wfp.x_dev)));
    double r;
    cout << wfp.vector_diff_x << ", " << wfp.vector_diff_y << endl;
    cout << wfp.x_dev << ", " << vx_line << ", " << vy_line << endl;
    if(vx_line * wfp.x_dev < 0)	{
    	r = abs(wfp.x_dev)/(1/sin(alpha)-1);
	cout << "In the if statement, r is : " << r << endl;
    	if (r < r_threshold) {
    		if(wfp.x_dev > 0) {
    			m_right = v_factor;
    			m_left = v_factor*(r*r_factor - d/2)/(r*r_factor + d/2);
    		}
    		else {
    			m_right = v_factor*(r*r_factor - d/2)/(r*r_factor + d/2);
    			m_left = v_factor;
    		}
		cout << m_left << ", " << m_right << endl;
    		arduino.send(buildPktArduinoV2(0, (uint8_t)m_left, (uint8_t)m_left, (uint8_t)m_right, (uint8_t)m_right));
    		return;
    	}
    }
    r = abs((k*vx_line + wfp.x_dev)/2 + k*k*vy_line*vy_line/(2*(k*vx_line + wfp.x_dev)));
    cout << "Out of if statement, r is : " << r << endl;
    if(wfp.x_dev > 0) {
    	m_right = v_factor*(r*r_factor - d/2)/(r*r_factor + d/2);
    	m_left = v_factor;
    }
    else {
    	m_right = v_factor;
    	m_left = v_factor*(r*r_factor - d/2)/(r*r_factor + d/2);
    }
    cout << m_left << ", " << m_right << endl;
    arduino.send(buildPktArduinoV2(0, (uint8_t)m_left, (uint8_t)m_left, (uint8_t)m_right, (uint8_t)m_right));
    */
	const double theta = -atan(wfp.beta_hat);
	const double dev_coeff = 0.0001;
	const double base = 300.0;
	double m_left = v_factor * pow(base, dev_coeff * wfp.x_dev + theta);
	double m_right = v_factor * pow(base, -(dev_coeff * wfp.x_dev + theta));
	cout << theta << " " << m_left << " " << m_right << endl;
	arduino.send(buildPktArduinoV2(0, (uint8_t)m_left, (uint8_t)m_left, (uint8_t)m_right, (uint8_t)m_right));
}
void finish(int signal) {
    exit(0);
}
