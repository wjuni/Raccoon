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

void server_packet_handler(ServerRecvContext *cx);
void arduino_packet_handler(PktRaspi *pkt);
void video_feedback_handler(webcam::VideoFeedbackParam wfp);
void rear_feedback_handler(webcam::VideoFeedbackParam wfp);
uint8_t lservoMap(uint16_t, uint16_t, uint16_t, uint16_t, double);
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
double lservo1 = 0.0, lservo2 = 0.0;
/* ---------- */

/* Variables for linear servo ctrl */
double v21_ratio = 0.98/(1.3*1.857);
bool sprayOper = false, isReversing = false;
int servoWait, spreadTime, reverseTime;
int lowest = 5, highest = 15;
int servoMin, servoMax, ldefault;
int sprayOn, sprayOff;
uint16_t servoVal;
uint8_t lservo1Val, lservo2Val;
/* ------------------------------- */

double bias, tangentVal;
double r_previous, l_previous;
double x_prev = 0;
double extra_term = 0;
int skipFrame;
int8_t skipCnt;

bool hasDetected = false, forwardGoing = false;

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
    context.bot_status = 0;
    context.damage_ratio = 0;
    strcpy(context.repair_module, "YELLOW");
    
    server.start(&context, server_packet_handler);
    
    /* Temporal part : parameter input */
    FILE *parStream = fopen("parameters.txt", "r");
    fscanf(parStream, "v_factor %lf\nmax_v %lf\nmin_v %lf\ndev_coeff %lf\nbase %lf\nextra_factor %lf\ndivide1 %lf\ndivide2 %lf\nbeta_creterion %lf\nsprayOff %d\nsprayOn %d\nservoMin %d\nservoMax %d\nlowest %d\nhighest %d\nldefault %d\nskipFrame %d\nservoWait %d\nspreadTime %d\nreverseTime %d",
	   &v_factor, &max_v, &min_v, &dev_coeff, &base, &extra_factor, &divide1, &divide2, &beta_creterion, &sprayOff, &sprayOn, &servoMin, &servoMax, &lowest, &highest, &ldefault, &skipFrame, &servoWait, &spreadTime, &reverseTime);
    printf("v_factor %lf\nmax_v %lf\nmin_v %lf\ndev_coeff %lf\nbase %lf\nextra_factor %lf\ndivide1 %lf\ndivide2 %lf\nbeta_creterion %lf\nsprayOff %d\nsprayOn %d\nservoMin %d\nservoMax %d\nlowest %d\nhighest %d\nldefault %d\nskipFrame %d\nservoWait %d\nspreadTime %d\nreverseTime %d\n",
	   v_factor, max_v, min_v, dev_coeff, base, extra_factor, divide1, divide2, beta_creterion, sprayOff, sprayOn, servoMin, servoMax, lowest, highest, ldefault, skipFrame, servoWait, spreadTime, reverseTime);
    cout << "reading complete" << endl;
    fclose(parStream);
    bias = (max_v + min_v)/2;
    tangentVal = (max_v - min_v)/2;
    skipCnt = (int8_t)skipFrame;
    /* ------------------------------- */

    /* Webcam */
    if(argc >=2 && strcmp(argv[1], "gui") == 0) {
//        wp.setX11Support(true);
	wp_rear.setX11Support(true);
    }
//    wp_rear.hMin = 15;
	wp_rear.sMin = 40;
//	wp_rear.bMin = 100;
//	wp_rear.hMax = 40;
//	wp_rear.sMax = 255;
//	wp_rear.bMax = 255;
	wp.start(webcam::WEBCAM, 0, video_feedback_handler);
    wp_rear.start(webcam::WEBCAM, 1, rear_feedback_handler);
    
    // Test Code
/*
    cout << "start to test" << endl;
    arduino.send(buildPktArduinoV2(0, 0, 0, 0, 0, 0, 0, (uint16_t)servoMin));
    usleep(servoWait/4);
    arduino.send(buildPktArduinoV2(0, 0, 0, 0, 0, 0, 0, (uint16_t)servoMax));
    usleep(spreadTime);
    arduino.send(buildPktArduinoV2(0, 0, 0, 0, 0, 0, 0, (uint16_t)servoMin));
    usleep(spreadTime);
    cout << "end test" << endl;
*/
    // end Test Code

    while(true) {
        arduino.recv(arduino_packet_handler);
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    
    return 0;
}

void arduino_packet_handler(PktRaspi *pkt) {
//    cout << "[Arduino] Got Packet" << endl;
    
//    cout << "GPS Lat : " << pkt->gps_lat << endl;
//    cout << "GPS Lon : " << pkt->gps_lon << endl;
//    cout << "GPS Fix : " << (int)pkt->gps_fix << endl;

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

void video_feedback_handler(webcam::VideoFeedbackParam wfp) {
	//cout << "Video Handler Called, wfp = " << wfp.beta_hat << ", " << wfp.x_dev << ", " << wfp.vector_diff_x << ", " << wfp.vector_diff_y << endl;

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
		m_left = l_previous;
		m_right = r_previous;
	}
	else	{
		l_previous = m_left;
		r_previous = m_right;
	}
//	cout << "Before : "<< lservo1 << " " << lservo2 << endl;
	if (sprayOper)	{
//		lservo1Val = lservoMap(0, 240, (uint8_t)servoMin, (uint8_t)servoMax, lservo1);
//		lservo2Val = lservoMap(0, 240, (uint8_t)servoMin, (uint8_t)servoMax, lservo2);
//		cout << "Converted : " << (int)lservo1Val << " " << (int)lservo2Val << " " << (int)servoVal << endl;
		if (isReversing) arduino.send(buildPktArduinoV2(0, (int8_t)(-10), (int8_t)(-10), (int8_t)(-10), (int8_t)(-10), lservo1Val, lservo2Val, servoVal));
		else if (forwardGoing) arduino.send(buildPktArduinoV2(0, (int8_t)m_right, (int8_t)m_right, (int8_t)m_left, (int8_t)m_left, lservo1Val, lservo2Val, servoVal));
		else  arduino.send(buildPktArduinoV2(0, 0, 0, 0, 0, lservo1Val, lservo2Val, servoVal));
//		cout << "Spray is Operating" << endl;
	}
	else {
//		cout << "Default : " << ldefault << endl;
//		cout << "Default : " << (int) (ldefault / (1 + v21_ratio) + servoMin) <<" " <<  (int)(ldefault * v21_ratio / (1 + v21_ratio) + servoMin) << endl;
		arduino.send(buildPktArduinoV2(0, (int8_t)m_right, (int8_t)m_right, (int8_t)m_left, (int8_t)m_left, (uint8_t) (ldefault / (1 + v21_ratio) + servoMin), (uint8_t)(ldefault * v21_ratio / (1 + v21_ratio) + servoMin), servoVal));
	}
}

void rear_feedback_handler(webcam::VideoFeedbackParam wfp) {
	/*
     * Determine whether the empty part of line is on the center of the rear camera
     * Get the y_f
     */
//	cout << "Rear feedback handler called" << endl; 
//	cout << "Rear Handler Called, wfp = " << wfp.beta_hat << ", " << wfp.x_dev << ", " << wfp.vector_diff_x << ", " << wfp.vector_diff_y << endl;

	if (skipCnt > 0) {
		skipCnt--;
		return;
	}
	
	if (!hasDetected && wfp.startP > lowest && wfp.startP < highest && wfp.emptyCnt > 3 && !std::isnan(wfp.x_f) && abs(wfp.x_dev) < 105) {
	cout << "Spray system starts to operate" << endl;
	cout << "x_f : " << wfp.x_f << endl;
    	sprayOper = true;
	lservo1 = wfp.x_f / (1 + v21_ratio);
	lservo2 = wfp.x_f * v21_ratio / (1 + v21_ratio);
	lservo1Val = lservoMap(0, ldefault, (uint16_t)servoMin, (uint16_t)servoMax, lservo1);
    	lservo2Val = lservoMap(0, ldefault, (uint16_t)servoMin, (uint16_t)servoMax, lservo2);
	cout << "Converted : " << (int)lservo1Val << " " << (int)lservo2Val << endl;
	servoVal = (uint16_t)sprayOff;
	isReversing = true;
    	usleep(reverseTime);
	isReversing = false;
	usleep(servoWait);
	forwardGoing = true;
    	cout << "Linear servo operation complete" << endl;
    	servoVal = (uint16_t)sprayOn;
    	usleep(spreadTime);
    	servoVal = (uint16_t)sprayOff;
    	usleep(spreadTime/2);
	forwardGoing = false;
    	cout << "Spray operation complete" << endl;
    	lservo1 = lservo2 = 0.0;
    	cout << "Spray system operation ends" << endl;
    	skipCnt = (int8_t)skipFrame;
	sprayOper = false;
	hasDetected = true;
    }

}

uint8_t lservoMap(uint16_t prevMin, uint16_t prevMax, uint16_t mappedMin, uint16_t mappedMax, double value) {
    return (uint8_t) ((uint16_t) value * (mappedMax - mappedMin)/(prevMax - prevMin) + mappedMin);
}

void server_packet_handler(ServerRecvContext *rc) {
    if (context.task_id == 0 && rc->tid != 0) {
        cout << "New Task id=" << rc->tid << endl;
    }
    if (context.task_id != 0 && rc->tid == 0) {
        cout << "Task Stop id=" << rc->tid << endl;
    }
    context.task_id = rc->tid;
    
    context.bot_status = (rc->tid > 0) ? 1 : 0;
    
}
void finish(int signal) {
    arduino.send(buildPktArduinoV2(0, 0, 0, 0, 0, 0, 0, 0));
    usleep(500000);
    exit(0);
}

