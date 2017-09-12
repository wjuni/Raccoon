//
//  WebcamProcessor.hpp
//  Raccoon_Raspi
//
//  Created by 임휘준 on 2017. 9. 12..
//  Copyright © 2017년 임휘준. All rights reserved.
//

#ifndef WebcamProcessor_hpp
#define WebcamProcessor_hpp
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <thread>

namespace webcam {
    const int NUM_CORE = 4;
    const int MAX_DEV_PIX = 150;
    const int DEV_PIX_SENS = 4;
    enum Device {WEBCAM, IMAGE};
    
    typedef struct {
        double est_line_left;
        double est_line_right;
        double est_line_left_btm;
        double est_line_right_btm;
        int offset;
    } LineFittingParam;
    
    typedef struct {
        int min;
        int max;
        double best_fit_area;
        double best_percentage;
        cv::Mat *est_mask;
        LineFittingParam best_fit_param;
    } TiltEstimationParam;
    
    typedef struct {
        double vector_diff_x;
        double vector_diff_y;
        double alpha_hat;
    } VideoFeedbackParam;
    
    class WebcamProcessor {
    private:
        std::thread thd;
        bool isRunning;
        Device type;
        cv::VideoCapture cap;
        static void handleWebcamJob(WebcamProcessor *webcamprocessor);
        
    public:
        WebcamProcessor();
        ~WebcamProcessor();
        bool open(Device type);
        void close();
    };
}

#endif /* WebcamProcessor_hpp */
