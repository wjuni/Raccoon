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
webcam::WebcamProcessor wp;

void arduino_packet_handler(PktRaspi *pkt);
void video_feedback_handler(webcam::VideoFeedbackParam wfp);
void finish(int signal);

/* Parameters */
//double v_factor = 12.0;
double v_factor;
double max_v = 120.0;
double min_v = 3.0;
double dev_coeff = 0.0015;
double base = 250.0;
double extra_factor = 0.5;
double divide1, divide2;
double beta_creterion;
/* ---------- */

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

    /* Temporal part : parameter input */
    FILE *parStream = fopen("parameters.txt", "r");
/*
    char buffer[1024] = "";
    fgets(buffer, 1024, parStream);
    v_factor = atof(buffer);
    fgets(buffer, 1024, parStream);
    v_creterion = atof(buffer);
    fgets(buffer, 1024, parStream);
    dev_coeff = atof(buffer);
    fgets(buffer, 1024, parStream);
    base = atof(buffer);
*/
    fscanf(parStream, "%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf",
&v_factor, &max_v, &min_v, &dev_coeff, &base, &extra_factor, &divide1, &divide2, &beta_creterion);
    fclose(parStream);
    bias = (max_v + min_v)/2;
    tangentVal = (max_v - min_v)/2;
    /* ------------------------------- */

    /* Webcam */
    if(argc >=2 && strcmp(argv[1], "gui") == 0)
        wp.setX11Support(true);
    wp.start(webcam::WEBCAM, video_feedback_handler);

    gettimeofday(&start_point, NULL);
	
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

void video_feedback_handler(webcam::VideoFeedbackParam wfp) {
/*    cout << "Video Handler Called, wfp = " << wfp.beta_hat << ", " << wfp.x_dev << ", " << wfp.vector_diff_x << ", " << wfp.vector_diff_y << endl;
    gettimeofday(&end_point, NULL);
    operating_time = (double)(end_point.tv_sec) + (double)(end_point.tv_usec)/1000000.0-(double)(start_point.tv_sec)-(double)(start_point.tv_usec)/1000000.0;
    start_point = end_point;
    cout << "GapTime : " << operating_time << endl;*/
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
	if(wfp.x_dev*(wfp.x_dev-x_prev)>0) {
		extra_term=extra_factor*(wfp.x_dev-x_prev);
	}
	else	extra_term = 0.0;
//extra_term *= 0.99;
	
//	double m_left = v_factor * pow(base, extra_term+dev_coeff * wfp.x_dev + theta);
//	double m_right = v_factor * pow(base, -extra_term-(dev_coeff * wfp.x_dev + theta));
//	double m_left = v_factor * pow(base, extra_term + theta);
//	double m_right = v_factor * pow(base, -extra_term - theta);
//	cout << "dev_coeff : " << dev_coeff << endl << "x_dev : " << wfp.x_dev << endl;
	double m_left, m_right;
	if (wfp.beta_hat > -beta_creterion && wfp.beta_hat < beta_creterion)	{
		m_left = tangentVal/divide1 * pow(extra_term + dev_coeff * wfp.x_dev, 5.0) + bias;
		m_right = - tangentVal/divide1 * pow(extra_term + dev_coeff * wfp.x_dev, 5.0) + bias;
	}
	else	{
		m_left =  tangentVal/divide2 * pow(- v_factor*wfp.beta_hat, 5.0) + bias;
		m_right = - tangentVal/divide2 * pow(- v_factor*wfp.beta_hat, 5.0) + bias;

	}
	/*
	if (extra_term != 0.0) extra_term = pow(extra_term, 3.0);
	double m_left = (1 + extra_term) * v_factor * pow(base, dev_coeff * wfp.x_dev + theta);
	double m_right = (1 - extra_term) * v_factor * pow(base, -(dev_coeff * wfp.x_dev + theta));
	*/
	
//	cout << "theta : " <<  theta << endl << "beta_hat : " <<  wfp.beta_hat << endl;
//	cout << "extra : " << wfp.x_dev-x_prev << endl;
//	cout << m_left << endl;
//	cout << m_right << endl;
	x_prev = wfp.x_dev;
	/*
	const double alpha = -1.0;
	const double gamma = 0.001;
	const double limit = 0.46;
	const double v_ratio = pow(0.01, abs(wfp.beta_hat));
	const double v_max = v_creterion*(v_ratio < 0.1 ? 0.1 : v_ratio);
	const double calculated = alpha*wfp.beta_hat+gamma*wfp.x_dev;
	const double linearC = (calculated > limit) ? limit : (calculated < -limit) ? -limit : calculated;
	double m_left = v_max*(linearC+0.5);
	double m_right = v_max*(-linearC+0.5);

	cout << "beta : " << wfp.beta_hat << ", x_dev : " << wfp.x_dev << endl;
	cout << calculated << ", " << v_ratio << endl;
	cout << m_left << ", " << m_right << endl;
	*/
	/*
	if (std::isnan(m_right)) {
		m_right = 0.0;
		for(int i = 0; i < 3; i++)	m_right += m_coeff[i]*r_previous[i];
	}
	if (std::isnan(m_left)) {
		m_left = 0.0;
		for(int i = 0; i < 3; i++)	m_left += m_coeff[i]*l_previous[i];
	}*/

	if(m_left < min_v) m_left = min_v;
	if(m_left > max_v){
//			cout << "max" << endl;
		   	m_left = max_v;
	}
	if(m_right < min_v) m_right = min_v;
	if(m_right > max_v){
//			cout << "max" << endl;
		 	m_right = max_v;

	}
	/*
	for (int i = 1; i >= 0; i--) {
		r_previous[i+1] = r_previous[i];
		l_previous[i+1] = l_previous[i];
	}
	r_previous[0] = m_right;
	l_previous[0] = m_left;
	m_right = m_left = 0.0;
	for(int i = 0; i < 3; i++) {
		m_right += m_coeff[i]*r_previous[i];
		m_left += m_coeff[i]*l_previous[i];
	}
	*/
/*	
	if (std::isnan(m_right) || std::isnan(m_left))	{
		if(r_previous[0] > l_previous[0])	{
			m_left = -40.0;
			m_right = -10.0;
		}
		else	{
			m_left = -10.0;
			m_right = -40.0;
		}
		setSleep = 200000;
		callCnt++;
		wasNan = 1;
		if (callCnt >= 10)	{
			m_left = 50.0;
			m_right = 50.0;
			callCnt = 0;
			setSleep = 1000000;
		}
	}
	else	{
		callCnt = 0;
		wasNan = 0;
		r_previous[0] = m_right;
		l_previous[0] = m_left;
	}*/

	if (std::isnan(m_right) || std::isnan(m_left))	{
		m_left = l_previous[0];
		m_right = r_previous[0];
	}
	else	{
		l_previous[0] = m_left;
		r_previous[0] = m_right;
	}
//	cout << m_left << ", " << m_right << endl;
	
//	arduino.send(buildPktArduinoV2(0, (int8_t)m_right, (int8_t)m_right, (int8_t)m_left, (int8_t)m_left));

	if (wasNan)	usleep(setSleep);

	//arduino.send(buildPktArduinoV2(0, (uint8_t)120.0, (uint8_t)120.0, (uint8_t)1.0, (uint8_t)1.0));
}
void finish(int signal) {
    arduino.send(buildPktArduinoV2(0, 0, 0, 0, 0));
    //arduino.send(buildPktArduinoV2(0, (int8_t)-15.0, (int8_t)-15.0, (int8_t)-15.0, (int8_t)-15.0));
    exit(0);
}
