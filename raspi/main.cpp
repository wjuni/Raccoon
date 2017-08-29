//
//  main.cpp
//
//  Created by Hwijoon Lim on 2017. 7. 2..
//  Copyright © 2017 Hwijoon Lim. All rights reserved.
//
#define _USE_MATH_DEFINES
#define BUFFER_SIZE sizeof(PktRaspi)
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <thread>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include "PktProtocol.h"
#include "UART.h"
#include "ServerCommunicator.hpp"

#ifdef RASPBERRY_PI
#define TEST_FFMPEG_PATH "/home/wjuni/opencvtest/ffmpeg/"
#else
#define TEST_FFMPEG_PATH "/Users/wjuni/ffmpeg/"
#endif

const int NUM_CORE = 4;
const int MAX_DEV_PIX = 150;
const int DEV_PIX_SENS = 4;
const std::string SERVER_ADDR = "https://raccoon.wjuni.com/ajax/report.php";

UART uart("/dev/ttyAMA0");
ServerCommunicator server(SERVER_ADDR);
ServerCommContext context;

using namespace std;
typedef struct {
    double est_line_left;
    double est_line_right;
    double est_line_left_btm;
    double est_line_right_btm;
    int offset;
} LineFittingParam ;

typedef struct {
    int min;
    int max;
    double best_fit_area;
    double best_percentage;
    cv::Mat *est_mask;
    LineFittingParam best_fit_param;
} TiltEstimationParam ;

inline bool file_exists (const std::string& name) {
    ifstream f(name.c_str());
    return f.good();
}

double moment_x, moment_y, estimated_linewidth;
int height, width, proc_count = 0;
cv::Mat frame_threshed;
double fps_sum = 0.0;
cv::Mat **mask_bank = NULL;

void tilt_estimation(void *paramAddr) {
    int best_fit_area = 0;
    double best_percentage = 0.0;
    LineFittingParam best_fit_param;
    TiltEstimationParam *tep = (TiltEstimationParam *)paramAddr;
    cv::Mat *est_mask =tep->est_mask;
    int min = tep->min;
    int max = tep->max;
    cv::Point sq[4];
    for (int offset=min; offset<max; offset+=DEV_PIX_SENS) {
        est_mask->setTo(cv::Scalar(0));
        double est_line_left = moment_x-estimated_linewidth/2 + offset;
        double est_line_right = moment_x+estimated_linewidth/2+ offset;
        double est_line_left_btm = moment_x - estimated_linewidth / 2 - offset;
        double est_line_right_btm = moment_x + estimated_linewidth / 2 -  offset;
        sq[0] = cv::Point2d(est_line_left, 0);
        sq[1] = cv::Point2d(est_line_right, 0);
        sq[2] = cv::Point2d(est_line_right_btm, height);
        sq[3] = cv::Point2d(est_line_left_btm, height);
        cv::fillConvexPoly(*est_mask, sq, 4, cv::Scalar(255));
        cv::Mat filest;
        cv::multiply(frame_threshed, *est_mask, filest);
        int mask_area = cv::countNonZero(*est_mask);
        int area = cv::countNonZero(filest);
        if (area > best_fit_area) {
            best_fit_area = area;
            best_percentage = (double)area / (double)mask_area;
            best_fit_param = {est_line_left, est_line_right, est_line_left_btm, est_line_right_btm, offset};
        }
    }
    tep->best_fit_param = best_fit_param;
    tep->best_fit_area = best_fit_area;
    tep->best_percentage = best_percentage;
}

cv::Mat im_hsv;

bool process(cv::Mat *pim, bool toRotate, string path, string filename){
    
    int64_t e1 = cv::getTickCount();
    cv::Mat im = *pim;
    
    if (toRotate)
        cv::rotate(im, im, cv::ROTATE_90_CLOCKWISE);
    
    width = im.size().width;
    height = im.size().height;
    
    if (mask_bank == NULL){
        mask_bank = new cv::Mat* [NUM_CORE+1];
        for(int i=0;i<NUM_CORE+1;i++)
            mask_bank[i] = new cv::Mat(height, width, CV_8U);
    }
    
    cv::cvtColor(im, im_hsv, cv::COLOR_BGR2HSV);
//    cv::Scalar orange_min = cv::Scalar(15, 70, 100);
//    cv::Scalar orange_max = cv::Scalar(30, 250, 255);
    cv::Scalar white_min = cv::Scalar(0, 0, 165);
    cv::Scalar white_max = cv::Scalar(255, 15, 255);
    cv::inRange(im_hsv, white_min, white_max, frame_threshed);
    
    cv::Moments mom = cv::moments(frame_threshed, true);
    moment_x = mom.m10 / mom.m00, moment_y = mom.m01/mom.m00;
    
    cv::circle(im, cv::Point2d(width/2, height/2), 3, cv::Scalar(255, 0, 0));
    
    LineFittingParam best_fit_param;
    int best_fit_area = 0;
    double best_percentage = 0.;
    double linewidth_compensation = 1.;
    cv::Mat *mask = mask_bank[NUM_CORE], masked_frame;
    int allocation_per_core = 2 * MAX_DEV_PIX / NUM_CORE;
    for (int i=0; i<3; i++) {
        estimated_linewidth = cv::countNonZero(frame_threshed) / height * linewidth_compensation;
        TiltEstimationParam tep[NUM_CORE];
        thread thds[NUM_CORE];
        for(int core=0; core<NUM_CORE; core++) {
            tep[core].min = allocation_per_core*core - MAX_DEV_PIX;
            tep[core].max = allocation_per_core*(core+1) - MAX_DEV_PIX;
            tep[core].est_mask = mask_bank[core];
            thds[core] = thread(tilt_estimation, &tep[core]);
        }
        for(int core=0; core<NUM_CORE; core++) {
            thds[core].join();
            if (best_fit_area < tep[core].best_fit_area) {
                best_fit_area = tep[core].best_fit_area;
                best_percentage = tep[core].best_percentage;
                best_fit_param = tep[core].best_fit_param;
            }
        }
        linewidth_compensation /= pow(best_percentage, 1./(i+1));
        
        // COM compensation in 3 sections
        double midPoint[] = {moment_x-estimated_linewidth/2, moment_x, moment_x+estimated_linewidth/2};
        int crack_area[3], crack_area_sum = 0;
        for (int sect=0; sect<3; sect++) {
            mask->setTo(cv::Scalar(0));
            cv::Point sq[4];
            sq[0] = cv::Point(best_fit_param.est_line_left, 0);
            sq[1] = cv::Point(best_fit_param.est_line_left+sect*estimated_linewidth/3, 0);
            sq[2] = cv::Point(best_fit_param.est_line_left_btm+sect*estimated_linewidth/3, height);
            sq[3] = cv::Point(best_fit_param.est_line_left_btm, height);
            cv::fillConvexPoly(*mask, sq, 4, cv::Scalar(255));
            cv::multiply(frame_threshed, *mask, masked_frame);
            int crack_area_current = cv::countNonZero(*mask) - cv::countNonZero(masked_frame);
            crack_area[sect] = crack_area_current;
            crack_area_sum += crack_area_current;
        }
        
        int whole_crack_area = cv::countNonZero(frame_threshed);
        double sum_denom_x = whole_crack_area + crack_area_sum;
        double sum_denom_y = sum_denom_x;
        double sum_nom_x = moment_x * whole_crack_area;
        double sum_nom_y = moment_y * whole_crack_area;
        for (int sect=0; sect<3; sect++) {
            sum_nom_x += midPoint[sect] * crack_area[sect];
            sum_nom_y += height / 2 * crack_area[sect];
            moment_x = sum_nom_x / sum_denom_x;
            moment_y = sum_nom_y / sum_denom_y;
        }
    }
    
    cv::line(im, cv::Point2d(best_fit_param.est_line_left, 0), cv::Point2d(best_fit_param.est_line_left_btm, height), cv::Scalar(0, 0, 255), 2);
    cv::line(im, cv::Point2d(best_fit_param.est_line_right, 0), cv::Point2d(best_fit_param.est_line_right_btm, height), cv::Scalar(0, 0, 255), 2);
    
    double newm = INFINITY; // in case of vertical line
    if (best_fit_param.est_line_left != best_fit_param.est_line_left_btm)
        newm = (best_fit_param.est_line_left_btm - best_fit_param.est_line_left) / height;
    
    // passing ((best_fit_param[0] + best_fit_param[1])/2, 0)
    double newx = (best_fit_param.est_line_left + best_fit_param.est_line_right)/2;
    cv::line(im,
             cv::Point2d((best_fit_param.est_line_left + best_fit_param.est_line_right)/2, 0),
             cv::Point2d((best_fit_param.est_line_left_btm + best_fit_param.est_line_right_btm)/2, height),
             cv::Scalar(255, 0, 255), 2);
    
    double act_deg = 0, deviation = width/2. - (best_fit_param.est_line_left + best_fit_param.est_line_right)/2.;
    

    if (best_fit_param.est_line_left != best_fit_param.est_line_left_btm) { // if not vertical
        act_deg = atan(1. / newm) * 180. / M_PI + 90;
        if (act_deg > 90) act_deg = act_deg - 180;
        //x - newm*y - newx = 0 , (width/2, height/2)
        deviation = (width / 2 - newm * height / 2 - newx) / sqrt(1 + newm * newm);
    }
    
    cout <<  "Direction Degree = " << act_deg << " Deviation from Center = " << deviation <<
    ", (x=" << deviation * cos(act_deg * M_PI / 180) <<
    ", y=" << - deviation * sin(act_deg * M_PI / 180) << ")" << endl;
    
    
    PktArduinoV2 pkt;
    pkt.mode = 0;
//    pkt.head_degree = act_deg * PKTARDUINO_MULTIPLIER;
//    pkt.x_deviation = deviation * cos(act_deg * M_PI / 180) * PKTARDUINO_MULTIPLIER;
//    pkt.y_deviation = -deviation * sin(act_deg * M_PI / 180) * PKTARDUINO_MULTIPLIER;
    pkt.motor_1_spd = 0;
    pkt.motor_2_spd = 0;
    pkt.motor_3_spd = 0;
    pkt.motor_4_spd = 0;
    PktArduinoV2_prepare_packet(&pkt);
//    printf("%d %d\n", pkt.x_deviation, pkt.y_deviation);
    uart.write(&pkt, sizeof(PktArduinoV2));

    
    
    int64_t e2 = cv::getTickCount();
    double t = (e2 - e1)/cv::getTickFrequency();
    // cv::imwrite(path + filename + '_out2.jpg', best_fit_est_mask)
    // cv::imwrite(path + filename + '_out1.jpg', best_fit_filest)
    cv::imwrite(path + "detect_" + filename, im);
//    cv::imshow("Demo", im);
//    cv::waitKey(15);
    cout << "Task complete in " << t << " secs (" << 1./t << " fps)" << endl;
    fps_sum += 1./t;
    proc_count ++;
    return true;
}

bool process(cv::VideoCapture* vc, string path, string filename) {
    cv::Mat frame;
    vc->read(frame);
    if (frame.empty()) {
        cerr << "ERROR! blank frame grabbed\n";
        return false;
    }
    cv::resize(frame, frame, cv::Size(640, 360), 0, 0, cv::INTER_CUBIC);
    return process(&frame, false, path, filename);
}
bool process(string path, string filename, bool toRotate) {
    cout << path+filename << endl;
    if (!file_exists(path + filename))
        return false;
    cv::Mat im = cv::imread(path + filename);
    cv::resize(im, im, cv::Size(136, 240), 0, 0, cv::INTER_CUBIC);
    return process(&im, true, path, filename);
}

void read(void (*handler)(PktRaspi *)) {
    int buffer_len = 0;
    PktRaspi received_packet;
    char serial_buffer[BUFFER_SIZE];
    memset(serial_buffer, 0, BUFFER_SIZE);
    if(BUFFER_SIZE > buffer_len) {
        if(buffer_len == 0) {
            while(uart.read() != PKTRASPI_PREAMBLE);
            serial_buffer[buffer_len++] = PKTRASPI_PREAMBLE;
        }
//        buffer_len += this->serial->readBytes(this->serial_buffer + this->buffer_len, BUFFER_SIZE - this->buffer_len);
        buffer_len += uart.read(serial_buffer + buffer_len, BUFFER_SIZE - buffer_len);
        cout << "Packet Read, Acclen=";
        cout << buffer_len << endl;
    }

    if(buffer_len >= BUFFER_SIZE) {
        cout << "New Packet Detected Starting With ";
        cout << ((unsigned int) serial_buffer[0]);
        cout << " ";
        cout << ((unsigned int) serial_buffer[1]);
        cout << " ";
        cout << ((unsigned int) serial_buffer[2]) << endl;

        if (PktRaspi_parse_packet(serial_buffer, buffer_len, &received_packet) > 0) {
            cout << "Packet Read Successful" << endl;
            (*handler)(&received_packet);
        }

        buffer_len = 0; //clear buffer
    }

}

void read_img(){
    char dst[100] = {};
    for(int i=1;i<=302;i++){
        sprintf(dst, "frame%04d.jpg", i);
        process(TEST_FFMPEG_PATH, dst, true);
    }
    cout << "Average FPS = " << fps_sum / proc_count << endl;
}

void read_vid(){
    cv::VideoCapture cap;
    cap.open(0);
    int deviceID = 0;             // 0 = open default camera
    int apiID = cv::CAP_ANY;      // 0 = autodetect default API
    cap.open(deviceID + apiID);
    if (!cap.isOpened()) {
        cerr << "ERROR! Unable to open camera\n";
        return;
    }
    while(true){
        if (!process(&cap, "", "out.jpg"))
            break;
    }
}

void packet_handler(PktRaspi *pkt) {
   cout << "GPS Lat : " << pkt->gps_lat << endl;
   cout << "GPS Lon : " << pkt->gps_lon << endl;
   cout << "GPS Fix : " << pkt->gps_fix << endl;
}

int main(int argc, const char * argv[]) {
    
    /* UART */
    uart.begin(115200);
    PktArduinoV2 pkt;
    pkt.mode = (1<<8);
    pkt.motor_1_spd = 0;
    pkt.motor_2_spd = 0;
    pkt.motor_3_spd = 0;
    pkt.motor_4_spd = 0;
    PktArduinoV2_prepare_packet(&pkt);
    uart.write(&pkt, sizeof(PktArduinoV2)); // notify boot complete
    
    /* Server */
    memset(&context, 0, sizeof(ServerCommContext));
    context.bot_id = 1;
    context.bot_speed = 637;
    context.bot_battery = 95;
    context.acc_distance = 239;
    context.bot_version = 11;
    server.start(&context);
    
/*    read_vid(); */
    // read_img();/

    while(true) {
        read(packet_handler);
        usleep(10000);
    }

    return 0;
}
