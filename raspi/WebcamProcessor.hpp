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
#include <atomic>

namespace webcam {
    const int NUM_CORE = 4;
    const double SPEED_RATIO = 0.3;
    
    
    // Algorithm 1
    const int MAX_DEV_PIX = 150;
    const int DEV_PIX_SENS = 4;
    
    // Algorithm 2
    const int N = 16;
    const int CONV_THRESH = 25;  // change according to the screen width (30% of width is appropriate)
    const double LINE_MARGIN_RATIO = 1.2;
    
 	const int lines = 1; // can be improved to 2 afterwards
   
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
        double beta_hat;
        double x_dev;
        double x_f;
        uint8_t startP, emptyCnt;

    } VideoFeedbackParam;

	struct LineSegmentElement{
			double x;
			double y;
			double weight;
			double linewidth;
			bool valid = false;
	};



    class WebcamProcessor {
    private:
        std::thread thd;
        bool X11Support;
        bool isRunning;
        Device type;
        cv::VideoCapture cap;
        static void handleWebcamJob(WebcamProcessor *webcamprocessor);
        void (*handler)(VideoFeedbackParam);
        
    public:
		double moment_x, moment_y, estimated_linewidth;
		std::atomic<double> local_max_linewidth;
		int steps_n;
		LineSegmentElement line_center[lines][N];

		cv::Mat cropped[NUM_CORE];
		cv::Mat vec_avg[NUM_CORE];
		cv::Mat normalized_vec[NUM_CORE];
		cv::Mat conv_filter[NUM_CORE];
		cv::Mat conv[NUM_CORE];

		int height, width, proc_count = 0;
		cv::Mat frame_threshed;
		double fps_sum = 0.0;
		cv::Mat **mask_bank = NULL;
		double global_linewidth_estimation = 0;
		cv::Mat im_hsv;



		WebcamProcessor();
        ~WebcamProcessor();
        bool start(Device type, int deviceID, void (*handler)(VideoFeedbackParam));
        void setX11Support(bool X11Support);
        void close();
    };
}

#endif /* WebcamProcessor_hpp */
