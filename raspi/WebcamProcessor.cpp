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


inline bool file_exists (const std::string& name) {
    ifstream f(name.c_str());
    return f.good();
}

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

void applyAlgorithm2_convolution_filter(int i, int core_id, WebcamProcessor *wp) {
    assert(core_id < NUM_CORE);
    cv::Rect myROI(0, i*wp->steps_n, wp->width-1, wp->steps_n);
    wp->cropped[core_id] = wp->frame_threshed(myROI);
    cv::reduce(wp->cropped[core_id], wp->vec_avg[core_id], 0, CV_REDUCE_AVG, CV_64F);
    double min = 0, max = 0;
    cv::minMaxLoc(wp->cropped[core_id], &min, &max);
    if (max == 0) {
        // cannot find frame here -> Continue & Wait
        return;
    }
    wp->normalized_vec[core_id] = wp->vec_avg[core_id] / max * 2. - 1.;
    double local_estimated_linewidth = (cv::sum(wp->vec_avg[core_id]) / 255.)[0];
    
    // thread-safe implementation updating max value
    double prev_value = wp->local_max_linewidth;
    while(prev_value < local_estimated_linewidth && !wp->local_max_linewidth.compare_exchange_weak(prev_value, local_estimated_linewidth));
    
    int filter_width = (int)(local_estimated_linewidth * 1.5);
    if(filter_width <= 1) {
        // filter width too narrow
        return;
    }
    wp->conv_filter[core_id] = cv::Mat1d(1, filter_width, -1.0);
    
    for(int j=(int)local_estimated_linewidth / 4; j<(int)(local_estimated_linewidth * 5 / 4); j++) {
        assert(j < wp->conv_filter[core_id].size().width);
        wp->conv_filter[core_id].at<double>(j) = 1.;
    }
    
    wp->conv[core_id] = cv::Mat();
    cv::filter2D(wp->normalized_vec[core_id], wp->conv[core_id], -1, wp->conv_filter[core_id]);
    
    // INFO: compensate to python code : newl = maxloc.x + conv_filter.size().width/2.
    for(int j=0; j<lines; j++) {
        cv::Point minloc, maxloc;
        cv::minMaxLoc(wp->conv[core_id], &min, &max, &minloc, &maxloc);
        if (max < CONV_THRESH)
            break;
        double left_line = maxloc.x - local_estimated_linewidth / 2. * LINE_MARGIN_RATIO;
        double right_line = maxloc.x + local_estimated_linewidth / 2. * LINE_MARGIN_RATIO;
        int start = static_cast<int>(left_line) < 0 ? 0 : static_cast<int>(left_line);
        for(int k=start; k < static_cast<int>(right_line); k++) {
            if(k >= wp->conv[core_id].size().width)
                break;
            assert(k < wp->conv[core_id].size().width && k >= 0);
            wp->conv[core_id].at<double>(k) = 0;
        }
        
        LineSegmentElement lse;
        lse.valid = true;
        lse.x = maxloc.x;
        lse.y = (i+0.5)*wp->steps_n;
        lse.weight = (max - CONV_THRESH)/10;
        lse.linewidth = local_estimated_linewidth;
        
        wp->line_center[j][i] = lse;
    }
}
struct algorithm2_multithread_obj {
    int start;
    int end;
    int core_id;
	WebcamProcessor *wp;
};
void applyAlgorithm2_convolution_filter_helper(struct algorithm2_multithread_obj *data) {
    for(int i=data->start; i<data->end; i++) {
        applyAlgorithm2_convolution_filter(i, data->core_id, data->wp);
    }
}

bool applyAlgorithm2(cv::Mat *pim, string path, string filename, void (*handler)(VideoFeedbackParam), bool X11Support, WebcamProcessor *wp) {
    
    int64_t e1 = cv::getTickCount();
    cv::Mat im = *pim;
    
   
    wp->width = im.size().width;
    wp->height = im.size().height;
    
    wp->local_max_linewidth.store(0);
    
    cv::cvtColor(im, wp->im_hsv, cv::COLOR_BGR2HSV);
    cv::Scalar orange_min = cv::Scalar(15, 70, 100);
    cv::Scalar orange_max = cv::Scalar(40, 255, 255);
    //cv::Scalar white_min = cv::Scalar(0, 0, 165);
    //cv::Scalar white_max = cv::Scalar(255, 15, 255);
    cv::inRange(wp->im_hsv, orange_min, orange_max, wp->frame_threshed);
    
	wp->steps_n = wp->height / N;

	int task_per_cpu = (N+NUM_CORE-1) / NUM_CORE;
	thread thds[NUM_CORE];
    algorithm2_multithread_obj amo[NUM_CORE];
    memset(wp->line_center, 0, sizeof(wp->line_center)); 
    for(int i=0, j=0; i<N; j++, i+=task_per_cpu) {
        int end = (i + task_per_cpu > N)? N : i + task_per_cpu;
        amo[j].start = i;
        amo[j].end = end;
        amo[j].core_id = j;
		amo[j].wp = wp;
        thds[j] = thread(applyAlgorithm2_convolution_filter_helper, &amo[j]);
    }
    for(int i=0; i<NUM_CORE; i++) {
        thds[i].join();
    }
    
    uint8_t emptyCnt = 0, startP = -1, maxEmptyCnt = 0, maxStartP = 0;

    double center_xsample[lines][N] = {0, }, center_ysample[lines][N] = {0, }, center_weights[lines][N] = {0, };
    int center_samples[lines] = {0, };
    for(int i=3; i<N-3; i++) {
        for(int j=0; j<lines; j++) {
            if(wp->line_center[j][i].valid){
                LineSegmentElement lse = wp->line_center[j][i];
                cv::circle(im, cv::Point2d(lse.x, lse.y), 2, cv::Scalar(0, 0, 255), (int)(lse.weight));
                center_xsample[j][center_samples[j]] = lse.x;
                center_ysample[j][center_samples[j]] = lse.y;
                // algorithm mistake fixed (estimated_linewidth should be lse.linewidth)
                center_weights[j][center_samples[j]] = lse.weight *
                (lse.linewidth / wp->local_max_linewidth) * (lse.linewidth / wp->local_max_linewidth);
                
                if (wp->global_linewidth_estimation == 0)
                    wp->global_linewidth_estimation = wp->local_max_linewidth;
                else
                    wp->global_linewidth_estimation = 0.9 * wp->global_linewidth_estimation + 0.1 * wp->local_max_linewidth;
                
                center_samples[j]++;

                emptyCnt = 0;
            }
            else {
                if (emptyCnt == 0)  startP = i;
                emptyCnt++;
                if (emptyCnt > maxEmptyCnt) {
                    maxStartP = startP;
                    maxEmptyCnt = emptyCnt;
                }
            }
        }
    }
    
    VideoFeedbackParam vfp;
    vfp.startP = maxStartP;
    vfp.emptyCnt = maxEmptyCnt;

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
        double mx = betahat * (wp->height/2) + alphahat;
        double my = wp->height/2;
	double theta = atan(-1/betahat);
	if(theta < 0.0)	theta += M_PI;
        double tx = mx + SPEED_RATIO*wp->height*cos(theta);
        //double ty = my - SPEED_RATIO*height*abs(sin(atan(1/betahat)));
        double vdiffy = SPEED_RATIO*wp->height*abs(sin(atan(1/betahat)));
	double vdiffx = tx - wp->width/2;
        //double vdiffy = ty - height/2;
        double x_dev = mx- wp->width/2;
        
        cv::line(im,
                 cv::Point2d(alphahat, 0),
                 cv::Point2d(alphahat + betahat * wp->height, wp->height), cv::Scalar(255, 0, 255), 2);
        cv::line(im,
                 cv::Point2d(alphahat + wp->global_linewidth_estimation / 2, 0),
                 cv::Point2d(alphahat + betahat * wp->height + wp->global_linewidth_estimation / 2, wp->height), cv::Scalar(0, 255, 0), 2);
        cv::line(im,
                 cv::Point2d(alphahat - wp->global_linewidth_estimation / 2, 0),
                 cv::Point2d(alphahat + betahat * wp->height - wp->global_linewidth_estimation / 2, wp->height), cv::Scalar(0, 255, 0), 2);
        
        if (center_samples[j] >=2 ) {
            vfp.beta_hat = betahat;
            vfp.vector_diff_x = vdiffx;
            vfp.vector_diff_y = vdiffy;
            vfp.x_dev = x_dev;
            vfp.x_f = mx;
            (*handler)(vfp);
		} else {
			vfp.beta_hat = std::nan("");
			vfp.vector_diff_x = std::nan("");
			vfp.vector_diff_y = std::nan("");
			vfp.x_dev = std::nan("");
       		vfp.x_f = std::nan("");
			(*handler)(vfp);

		}
    }
    
    
    int64_t e2 = cv::getTickCount();
    double t = (e2 - e1)/cv::getTickFrequency();
//    cv::imwrite(path + "detect_" + filename, im);
    
    if(X11Support) {
        cv::imshow("Algorithm 2", im);
 //       cv::imshow("Algorithm 2_threshed", wp->frame_threshed);
        cv::waitKey(15);
    }
//    cout << "Task complete in " << t << " secs (" << 1./t << " fps)" << endl;
    wp->fps_sum += 1./t;
    wp->proc_count ++;
    return true;
}

bool process(cv::VideoCapture* vc, string path, string filename, void (*handler)(VideoFeedbackParam), bool X11Support, WebcamProcessor *wp) {
    cv::Mat frame;
    vc->read(frame);
    if (frame.empty()) {
        cerr << "ERROR! blank frame grabbed\n";
        return false;
    }
    cv::resize(frame, frame, cv::Size(240, 160), 0, 0, cv::INTER_CUBIC);
    return applyAlgorithm2(&frame, path, filename, handler, X11Support, wp);
}
bool process(string path, string filename, void (*handler)(VideoFeedbackParam), bool X11Support, WebcamProcessor *wp) {
    cout << path+filename << endl;
    if (!file_exists(path + filename))
        return false;
    cv::Mat im = cv::imread(path + filename);
    if(im.size().width < im.size().height) {
        cv::rotate(im, im, cv::ROTATE_90_CLOCKWISE);
    }
    cv::resize(im, im, cv::Size(240, 160), 0, 0, cv::INTER_CUBIC);
    return applyAlgorithm2(&im, path, filename, handler, X11Support, wp);
}

WebcamProcessor::WebcamProcessor() {
    isRunning = false;
    X11Support = false;
}


void WebcamProcessor::setX11Support(bool X11Support){
    this->X11Support = X11Support;
}

bool WebcamProcessor::start(webcam::Device type, int deviceID, void (*handler)(VideoFeedbackParam)) {
    this->type = type;
    this->handler = handler;
    
    if(type == WEBCAM) {
        //cap.open(0);
        int apiID = cv::CAP_ANY;      // 0 = autodetect default API
        cap.open(deviceID + apiID);
        if (!cap.isOpened()) {
            cerr << "ERROR! Unable to open camera\n";
            return false;
        }
        isRunning = true;
        thd = thread(handleWebcamJob, this);
        return true;
    } else if(type == IMAGE) {
        isRunning = true;
        thd = thread(handleWebcamJob, this);
        return true;
    }
    
    return false;
}

void WebcamProcessor::handleWebcamJob(WebcamProcessor *webcamprocessor) {
    if (webcamprocessor->type == WEBCAM) {
        while(true){
            if (!process(&(webcamprocessor->cap), "", "out.jpg", webcamprocessor->handler, webcamprocessor->X11Support, webcamprocessor)) break;
            if (!webcamprocessor->isRunning) break;
        }
    } else if(webcamprocessor->type==IMAGE) {
        char dst[100] = {};
        for(int i=1;;i++){
            sprintf(dst, "frame%04d.jpg", i);
            if (!process(TEST_FFMPEG_PATH, dst, webcamprocessor->handler, webcamprocessor->X11Support, webcamprocessor)) break;
            if (!webcamprocessor->isRunning) break;
        }
        cout << "Average FPS = " << webcamprocessor->fps_sum / webcamprocessor->proc_count << endl;
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

