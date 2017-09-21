//
//  WebcamProcessor.cpp
//  Raccoon_Raspi
//
//  Created by 임휘준 on 2017. 9. 12..
//  Copyright © 2017년 임휘준. All rights reserved.
//
#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <thread>
#include <cmath>
#include <atomic>
#include "WebcamProcessor.hpp"

#ifdef RASPBERRY_PI
#define TEST_FFMPEG_PATH "/home/wjuni/opencvtest/ffmpeg/"
#else
#define TEST_FFMPEG_PATH "data/"
#endif

using namespace std;
using namespace webcam;

double moment_x, moment_y, estimated_linewidth;
int height, width, proc_count = 0;
cv::Mat frame_threshed;
double fps_sum = 0.0;
cv::Mat **mask_bank = NULL;
double global_linewidth_estimation = 0;


inline bool file_exists (const std::string& name) {
    ifstream f(name.c_str());
    return f.good();
}

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

bool applyAlgorithm1(cv::Mat *pim, string path, string filename, void (*handler)(VideoFeedbackParam), bool X11Support){
    
    int64_t e1 = cv::getTickCount();
    cv::Mat im = *pim;
    
    width = im.size().width;
    height = im.size().height;
    
    if (mask_bank == NULL){
        mask_bank = new cv::Mat* [NUM_CORE+1];
        for(int i=0;i<NUM_CORE+1;i++)
            mask_bank[i] = new cv::Mat(height, width, CV_8U);
    }
    
    cv::cvtColor(im, im_hsv, cv::COLOR_BGR2HSV);
    cv::Scalar orange_min = cv::Scalar(15, 70, 100);
    cv::Scalar orange_max = cv::Scalar(30, 255, 255);
    //cv::Scalar white_min = cv::Scalar(0, 0, 165);
    //cv::Scalar white_max = cv::Scalar(255, 15, 255);
    cv::inRange(im_hsv, orange_min, orange_max, frame_threshed);
    
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
    
//    double act_deg = 0, deviation = width/2. - (best_fit_param.est_line_left + best_fit_param.est_line_right)/2.;
    
    double vdiffx = 0., vdiffy = 0., x_dev = 0., beta_hat=0.;
    double mx = newm*height/2+newx;
    double my = height/2;
    if (best_fit_param.est_line_left != best_fit_param.est_line_left_btm) { // if not vertical
//        act_deg = atan(1. / newm) * 180. / M_PI + 90;
//        if (act_deg > 90) act_deg = act_deg - 180;
        //x - newm*y - newx = 0 , (width/2, height/2)
//        deviation = (width / 2 - newm * height / 2 - newx) / sqrt(1 + newm * newm);   
        double tx = mx+SPEED_RATIO*height*cos(atan(1/newm));
        double ty = my - SPEED_RATIO*height*sin(atan(1/newm));
        vdiffx = abs(tx - width/2);
        vdiffy = abs(ty - height/2);
        beta_hat = newm;
        x_dev=abs(mx-width/2);
    } else {
        vdiffx = abs(mx-width/2);
        x_dev=vdiffx;
        vdiffy = -SPEED_RATIO*height;
        beta_hat = 0;
    }
    
//    cout <<  "Direction Degree = " << act_deg << " Deviation from Center = " << deviation <<
//    ", (x=" << deviation * cos(act_deg * M_PI / 180) <<
//    ", y=" << - deviation * sin(act_deg * M_PI / 180) << ")" << endl;
    
    // call VideoFeedbackParam Handler here
    VideoFeedbackParam vfp;
    vfp.beta_hat = beta_hat;
    vfp.vector_diff_x = vdiffx;
    vfp.vector_diff_y = vdiffy;
    vfp.x_dev=x_dev;
    (*handler)(vfp);
    
    int64_t e2 = cv::getTickCount();
    double t = (e2 - e1)/cv::getTickFrequency();
    cv::imwrite(path + "detect_" + filename, im);
    if(X11Support) {
        cv::imshow("Algorithm 1", im);
        cv::waitKey(15);
    }
    cout << "Task complete in " << t << " secs (" << 1./t << " fps)" << endl;
    fps_sum += 1./t;
    proc_count ++;
    return true;
}

struct LineSegmentElement{
    double x;
    double y;
    double weight;
    double linewidth;
    bool valid = false;
};

void printMat(cv::Mat conv) {
    for (int k=0;k<conv.size().width; k++) {
        cout << conv.at<double>(k) << " ";
    }
    cout << "D" << endl;
}

inline double weighted_average(const double *arr, const double *weights, int len) {
    if (len == 0) return 0;
    double sum = 0.0;
    double weightsum = 0.0;
    for(int i=0; i<len; i++){
        sum += arr[i]*weights[i];
        weightsum += weights[i];
    }
    if(weightsum == 0.0) return 0.0;
    return sum / weightsum;
}

const int lines = 1; // can be improved to 2 afterwards
atomic<double> local_max_linewidth;
int steps_n;
LineSegmentElement line_center[lines][N];

cv::Mat cropped[NUM_CORE];
cv::Mat vec_avg[NUM_CORE];
cv::Mat normalized_vec[NUM_CORE];
cv::Mat1i conv_filter[NUM_CORE];
cv::Mat conv[NUM_CORE];
void applyAlgorithm2_convolution_filter(int i, int core_id) {
    cv::Rect myROI(0, i*steps_n, width-1, steps_n);
    cropped[core_id] = frame_threshed(myROI);
    cv::reduce(cropped[core_id], vec_avg[core_id], 0, CV_REDUCE_AVG, CV_64F);
    double min = 0, max = 0;
    cv::minMaxLoc(cropped[core_id], &min, &max);
    if (max == 0) {
        // cannot find frame here -> Continue & Wait
        return;
    }
    normalized_vec[core_id] = vec_avg[core_id] / max * 2. - 1.;
    double local_estimated_linewidth = (cv::sum(vec_avg[core_id]) / 255.)[0];
    
    // thread-safe implementation updating max value
    double prev_value = local_max_linewidth;
    while(prev_value < local_estimated_linewidth && !local_max_linewidth.compare_exchange_weak(prev_value, local_estimated_linewidth));
    
    conv_filter[core_id] = cv::Mat1i(1, (int)(local_estimated_linewidth * 1.5), -1);
    
    for(int j=(int)local_estimated_linewidth / 4; j<(int)(local_estimated_linewidth * 5 / 4); j++) {
        conv_filter[core_id].at<int>(j) = 1;
    }
    
    conv[core_id] = cv::Mat::zeros(steps_n, width, CV_64F);
    cv::filter2D(normalized_vec[core_id], conv[core_id], -1, conv_filter[core_id]);
    
    // INFO: compensate to python code : newl = maxloc.x + conv_filter.size().width/2.
    for(int j=0; j<lines; j++) {
        cv::Point minloc, maxloc;
        cv::minMaxLoc(conv[core_id], &min, &max, &minloc, &maxloc);
        if (max < CONV_THRESH)
            break;
        double left_line = maxloc.x - local_estimated_linewidth / 2. * LINE_MARGIN_RATIO;
        double right_line = maxloc.x + local_estimated_linewidth / 2. * LINE_MARGIN_RATIO;
        for(int k=static_cast<int>(left_line); k < static_cast<int>(right_line); k++) {
            conv[core_id].at<double>(k) = 0;
        }
        
        LineSegmentElement lse;
        lse.valid = true;
        lse.x = maxloc.x;
        lse.y = (i+0.5)*steps_n;
        lse.weight = (max - CONV_THRESH)/10;
        lse.linewidth = local_estimated_linewidth;
        
        line_center[j][i] = lse;
    }
}
struct algorithm2_multithread_obj {
    int start;
    int end;
    int core_id;
};
void applyAlgorithm2_convolution_filter_helper(struct algorithm2_multithread_obj *data) {
    for(int i=data->start; i<data->end; i++) {
        applyAlgorithm2_convolution_filter(i, data->core_id);
    }
}

bool applyAlgorithm2(cv::Mat *pim, string path, string filename, void (*handler)(VideoFeedbackParam), bool X11Support) {
    
    int64_t e1 = cv::getTickCount();
    cv::Mat im = *pim;
    
   
    width = im.size().width;
    height = im.size().height;
    
    local_max_linewidth.store(0);
    
    cv::cvtColor(im, im_hsv, cv::COLOR_BGR2HSV);
    cv::Scalar orange_min = cv::Scalar(15, 70, 100);
    cv::Scalar orange_max = cv::Scalar(30, 255, 255);
    //cv::Scalar white_min = cv::Scalar(0, 0, 165);
    //cv::Scalar white_max = cv::Scalar(255, 15, 255);
    cv::inRange(im_hsv, orange_min, orange_max, frame_threshed);
    
    steps_n = height / N;
    
    int task_per_cpu = (N+NUM_CORE-1) / NUM_CORE;
    thread thds[NUM_CORE];
    algorithm2_multithread_obj amo[NUM_CORE];
    
    for(int i=0, j=0; i<N; j++, i+=task_per_cpu) {
        int end = (i + task_per_cpu > N)? N : i + task_per_cpu;
        amo[j].start = i;
        amo[j].end = end;
        amo[j].core_id = j;
        thds[j] = thread(applyAlgorithm2_convolution_filter_helper, &amo[j]);
    }
    for(int i=0; i<NUM_CORE; i++) {
        thds[i].join();
    }
    
    double center_xsample[lines][N] = {0, }, center_ysample[lines][N] = {0, }, center_weights[lines][N] = {0, };
    int center_samples[lines] = {0, };
    for(int i=0; i<N; i++) {
        for(int j=0; j<lines; j++) {
            if(line_center[j][i].valid){
                LineSegmentElement lse = line_center[j][i];
                cv::circle(im, cv::Point2d(lse.x, lse.y), 2, cv::Scalar(0, 0, 255), (int)(lse.weight));
                center_xsample[j][center_samples[j]] = lse.x;
                center_ysample[j][center_samples[j]] = lse.y;
                // algorithm mistake fixed (estimated_linewidth should be lse.linewidth)
                center_weights[j][center_samples[j]] = lse.weight *
                (lse.linewidth / local_max_linewidth) * (lse.linewidth / local_max_linewidth);
                
                if (global_linewidth_estimation == 0)
                    global_linewidth_estimation = local_max_linewidth;
                else
                    global_linewidth_estimation = 0.9 * global_linewidth_estimation + 0.1 * local_max_linewidth;
                
                center_samples[j]++;
            }
        }
    }
    
    for(int j=0;j<lines;j++) {
        double xavg = weighted_average(center_xsample[j], center_weights[j], center_samples[j]);
        double yavg = weighted_average(center_ysample[j], center_weights[j], center_samples[j]);
        
        double _psum1 = 0, _psum2 = 0, _psum3 = 0, _psum4 = 0;
        for(int k=0; k<center_samples[j]; k++) {
            _psum1 += center_weights[j][k] * (center_xsample[j][k] - xavg) * (center_xsample[j][k] - xavg);
            _psum2 += center_weights[j][k];
            _psum3 += center_weights[j][k] * (center_ysample[j][k] - yavg) * (center_ysample[j][k] - yavg);
            _psum4 += center_weights[j][k] * (center_xsample[j][k] - xavg) * (center_ysample[j][k] - yavg);
        }
        double xstd = sqrt(_psum1 / _psum2);
        double ystd = sqrt(_psum3 / _psum2);
        double cov = _psum4 / _psum2;
        double corr = cov / (xstd * ystd);
        double betahat = corr * xstd / ystd;
        double alphahat = xavg - yavg * betahat;
        double mx = betahat * (height/2) + alphahat;
        double my = height/2;
        double tx = mx + SPEED_RATIO*height*cos(atan(1/betahat));
        double ty = my - SPEED_RATIO*height*sin(atan(1/betahat));
        double vdiffx = abs(tx - width/2);
        double vdiffy = abs(ty - height/2);
        double x_dev = abs(width/2-mx);
        
        cv::line(im,
                 cv::Point2d(alphahat, 0),
                 cv::Point2d(alphahat + betahat * height, height), cv::Scalar(255, 0, 255), 2);
        cv::line(im,
                 cv::Point2d(alphahat + global_linewidth_estimation / 2, 0),
                 cv::Point2d(alphahat + betahat * height + global_linewidth_estimation / 2, height), cv::Scalar(0, 255, 0), 2);
        cv::line(im,
                 cv::Point2d(alphahat - global_linewidth_estimation / 2, 0),
                 cv::Point2d(alphahat + betahat * height - global_linewidth_estimation / 2, height), cv::Scalar(0, 255, 0), 2);
        
        if (center_samples[j] >=2 ) {
            VideoFeedbackParam vfp;
            vfp.beta_hat = betahat;
            vfp.vector_diff_x = vdiffx;
            vfp.vector_diff_y = vdiffy;
            vfp.x_dev=x_dev;
            (*handler)(vfp);
        }
    }
    
    
    int64_t e2 = cv::getTickCount();
    double t = (e2 - e1)/cv::getTickFrequency();
    cv::imwrite(path + "detect_" + filename, im);
    
    if(X11Support) {
        cv::imshow("Algorithm 2", im);
        cv::waitKey(15);
    }
    cout << "Task complete in " << t << " secs (" << 1./t << " fps)" << endl;
    fps_sum += 1./t;
    proc_count ++;
    return true;
}


bool process(cv::VideoCapture* vc, string path, string filename, void (*handler)(VideoFeedbackParam), bool X11Support) {
    cv::Mat frame;
    vc->read(frame);
    if (frame.empty()) {
        cerr << "ERROR! blank frame grabbed\n";
        return false;
    }
    cv::resize(frame, frame, cv::Size(240, 160), 0, 0, cv::INTER_CUBIC);
    return applyAlgorithm1(&frame, path, filename, handler, X11Support);
}
bool process(string path, string filename, void (*handler)(VideoFeedbackParam), bool X11Support) {
    cout << path+filename << endl;
    if (!file_exists(path + filename))
        return false;
    cv::Mat im = cv::imread(path + filename);
    if(im.size().width < im.size().height) {
        cv::rotate(im, im, cv::ROTATE_90_CLOCKWISE);
    }
    cv::resize(im, im, cv::Size(240, 160), 0, 0, cv::INTER_CUBIC);
    return applyAlgorithm1(&im, path, filename, handler, X11Support);
}


WebcamProcessor::WebcamProcessor() {
    isRunning = false;
    X11Support = false;
}


void WebcamProcessor::setX11Support(bool X11Support){
    this->X11Support = X11Support;
}

bool WebcamProcessor::start(webcam::Device type, void (*handler)(VideoFeedbackParam)) {
    this->type = type;
    this->handler = handler;
    
    if(type == WEBCAM) {
        cap.open(0);
        int deviceID = 0;             // 0 = open default camera
        int apiID = cv::CAP_ANY;      // 0 = autodetect default API
        cap.open(deviceID + apiID);
        if (!cap.isOpened()) {
            cerr << "ERROR! Unable to open camera\n";
            return false;
        }
        isRunning = true;
        thd = thread(handleWebcamJob, this);
        return true;
    } else if(type==IMAGE) {
        isRunning = true;
        thd = thread(handleWebcamJob, this);
        return true;
    }
    
    return false;
}

void WebcamProcessor::handleWebcamJob(WebcamProcessor *webcamprocessor) {
    if (webcamprocessor->type == WEBCAM) {
        while(true){
            if (!process(&(webcamprocessor->cap), "", "out.jpg", webcamprocessor->handler, webcamprocessor->X11Support)) break;
            if (!webcamprocessor->isRunning) break;
        }
    } else if(webcamprocessor->type==IMAGE) {
        char dst[100] = {};
        for(int i=1;;i++){
            sprintf(dst, "frame%04d.jpg", i);
            if (!process(TEST_FFMPEG_PATH, dst, webcamprocessor->handler, webcamprocessor->X11Support)) break;
            if (!webcamprocessor->isRunning) break;
        }
        cout << "Average FPS = " << fps_sum / proc_count << endl;
    }
    webcamprocessor->isRunning = false;
}

void WebcamProcessor::close() {
    isRunning = false;
    thd.join();
}

WebcamProcessor::~WebcamProcessor() {
    close();
}

