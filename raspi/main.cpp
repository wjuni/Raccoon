// // main.cpp // // Created by Hwijoon Lim on 2017. 7. 2.. // Copyright © 2017 Hwijoon Lim. All rights reserved. //
 
#include <iostream>
#include <thread>
#include <cstring>
#include <signal.h>
#include <cmath>
#include <cstdio>
#include <unistd.h>
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
webcam::WebcamProcessor wp, wp_rear;

void arduino_packet_handler(PktRaspi *pkt);
void video_feedback_handler(webcam::VideoFeedbackParam wfp);
void rear_feedback_handler(webcam::VideoFeedbackParam wfp);
uint8_t lservoMap(int, int, int, int, double);
void finish(int signal);

/* Parameters */
double v_factor;
double max_v;
double min_v;
double dev_coeff;
double base;
double extra_factor;
double divide1, divide2;
double beta_creterion;
double servoVal, lservo1, lservo2;
/* ---------- */

/* Variables for linear servo ctrl */
double v21_ratio = 0.98/1.3;
bool sprayOper = false;
int servoWait = 10000000;
int spreadTime = 750000;
int lowest = 5, highest = 15;
/* ------------------------------- */

double bias, tangentVal;

double r_previous[3];
double l_previous[3];
const double m_coeff[3] = {1.0, 0.0, 0.0};
double x_prev = 0;
double extra_term = 0;

struct timeval start_point, end_point;
double operating_time;

int callCnt, setSleep, wasNan;

int main(int argc, const char * argv[]) {
    
    signal(SIGINT, finish);
    
    /* Arduino */
    arduino.begin(115200); // Baudrate must be 9600 or 115200
    arduino.send(buildPktArduinoV2(1<<8, 0, 0, 0, 0, 0, 0, 0)); // notify boot complete
    
    /* Server */
    memset(&context, 0, sizeof(ServerCommContext));
    context.bot_id = 1;
    context.bot_speed = 637;
    context.bot_battery = 95;
    context.acc_distance = 239;
    context.bot_version = 11;
    server.start(&context);
    
    /* Temporal part : parameter input */
    FILE *parStream = fopen("parameters.txt", "r");
    fscanf(parStream, "%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf",
	   &v_factor, &max_v, &min_v, &dev_coeff, &base, &extra_factor, &divide1, &divide2, &beta_creterion, &lservo1, &lservo2, &servoVal);
    cout << "reading complete" << endl;
    fclose(parStream);
    bias = (max_v + min_v)/2;
    tangentVal = (max_v - min_v)/2;
    /* ------------------------------- */

    /* Webcam */
    if(argc >=2 && strcmp(argv[1], "gui") == 0)
        wp.setX11Support(true);
    wp.start(webcam::WEBCAM, 0, video_feedback_handler);
    wp_rear.start(webcam::WEBCAM, 1, rear_feedback_handler);

    gettimeofday(&start_point, NULL);
	
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

const double d=189.0;

void video_feedback_handler(webcam::VideoFeedbackParam wfp) {
	cout << "Video Handler Called, wfp = " << wfp.beta_hat << ", " << wfp.x_dev << ", " << wfp.vector_diff_x << ", " << wfp.vector_diff_y << endl;

	const double theta = -atan(wfp.beta_hat);
	if(wfp.x_dev*(wfp.x_dev-x_prev)>0) {
		extra_term=extra_factor*(wfp.x_dev-x_prev);
	}
	else	extra_term = 0.0;

	double m_left, m_right;
	if (wfp.beta_hat > -beta_creterion && wfp.beta_hat < beta_creterion)	{
		m_left = tangentVal/divide1 * pow(extra_term + dev_coeff * wfp.x_dev, 5.0) + bias;
		m_right = - tangentVal/divide1 * pow(extra_term + dev_coeff * wfp.x_dev, 5.0) + bias;
	}
	else	{
		m_left =  tangentVal/divide2 * pow(- v_factor*wfp.beta_hat, 5.0) + bias;
		m_right = - tangentVal/divide2 * pow(- v_factor*wfp.beta_hat, 5.0) + bias;

	}
	x_prev = wfp.x_dev;


	if(m_left < min_v)	m_left = min_v;
	if(m_left > max_v)	m_left = max_v;
	if(m_right < min_v)	m_right = min_v;
	if(m_right > max_v)	m_right = max_v;

	if (std::isnan(m_right) || std::isnan(m_left))	{
		m_left = l_previous[0];
		m_right = r_previous[0];
	}
	else	{
		l_previous[0] = m_left;
		r_previous[0] = m_right;
	}
	if (sprayOper)	arduino.send(buildPktArduinoV2(0, 0, 0, 0, 0, lservoMap(0, 240, 0, 180, lservo1), lservoMap(0, 240, 0, 180, lservo2), servoVal));
	else arduino.send(buildPktArduinoV2(0, (int8_t)m_right, (int8_t)m_right, (int8_t)m_left, (int8_t)m_left, 0, 0, 0);

	if (wasNan)	usleep(setSleep);

}

void rear_feedback_handler(webcam::VideoFeedbackParam wfp) {
	/*
     * Determine whether the empty part of line is on the center of the rear camera
     * Get the y_f
     */
    if (wfp.startP > lowest && wfp.startP < highest && wfp.emptyCnt > 3 && !std::isnan(wfp.x_f)) {
    	cout << "Spray system starts to operate" << endl;
    	sprayOper = true;
    	lservo1 = wfp.x_f / (1 + v21_ratio);
    	lservo2 = wfp.x_f * v21_ratio / (1 + v21_ratio);
    	servoVal = 0;
    	usleep(servoWait);
    	cout << "Linear servo operation complete" << endl;
    	servoVal = 80;
    	usleep(spreadTime);
    	servoVal = 0;
    	usleep(spreadTime);
    	cout << "Spray operation complete" << endl;
    	lservo1 = lservo2 = 0.0;
    	cout << "Spray system operation ends" << endl;
    	sprayOper = false;
    }

}

uint8_t lservoMap(int prevMin, int prevMax, int mappedMin, int mappedMax, double value) {
    return (uint8_t) value * (prevMax - prevMin)/(mappedMax - mappedMin);
}

void finish(int signal) {
  arduino.send(buildPktArduinoV2(0, 0, 0, 0, 0, 0, 0, 0));
    exit(0);
}
