import cv2
import os.path
import numpy as np
import math

LINEWIDTH_DEFAULT = 0.220 # of screen width
TARGET_FRAMESIZE = (854, 480)
N = 16
CONV_THRESH = 25  # change according to the screen width (30% of width is appropriate)
LINE_MARGIN_RATIO = 1.2
SPEED_RATIO = 0.3
LINEWIDTH_INITIAL = LINEWIDTH_DEFAULT * TARGET_FRAMESIZE[0]
STEPS_N = TARGET_FRAMESIZE[1] / N

global_linewidth_estimation = None

def process(path, filename, rotate, lines):
    global global_linewidth_estimation

    # if the file not exist, return
    if not os.path.isfile(path + filename):
        return False

    im = cv2.imread(path + filename)
    e1 = cv2.getTickCount()
    # initialize tick counter to further count time

    if rotate:
        im = cv2.rotate(im, cv2.ROTATE_90_CLOCKWISE, im)
    # if the image is rotated, rotate the image

    # im = cv2.resize(im, TARGET_FRAMESIZE, 0, 0, cv2.INTER_CUBIC)

    height, width, channel = im.shape

    im_hsv = cv2.cvtColor(im, cv2.COLOR_BGR2HSV)

    ORANGE_MIN = np.array([15, 70, 100],np.uint8)
    ORANGE_MAX = np.array([30, 255, 255],np.uint8)

    frame_threshed = cv2.inRange(im_hsv, ORANGE_MIN, ORANGE_MAX)
    # convert the image into W/B image and threshold it

    line_center = []
    line_left = []
    line_right = []

    local_max_linewidth = 0


    try :
        for i in range(N):
            cropped = frame_threshed[i * STEPS_N:(i + 1) * STEPS_N, :]
            vec_avg = np.average(cropped, axis=0)
            max_val = np.max(vec_avg)
            normalized_vec = vec_avg / max_val * 2. - 1.
            estimated_linewidth = (np.sum(vec_avg) / 255)
            if estimated_linewidth > local_max_linewidth:
                local_max_linewidth = estimated_linewidth

            conv_filter = np.zeros(shape=(int(estimated_linewidth * 1.5)))
            conv_filter[:] = -1
            conv_filter[int(estimated_linewidth / 4):int(estimated_linewidth * 5 / 4)] = 1
            conv = np.convolve(normalized_vec, conv_filter)

            for l in range(lines):
                max_v = np.max(conv)
                max_arg = np.argmax(conv)
                if max_v < CONV_THRESH:
                    break
                max_arg -= estimated_linewidth * 0.75

                left_line = max_arg - estimated_linewidth / 2. * LINE_MARGIN_RATIO
                right_line = max_arg + estimated_linewidth / 2. * LINE_MARGIN_RATIO
                conv[int(left_line):int(right_line)] = 0
                line_center.append((max_arg, (i + 0.5) * STEPS_N, (max_v - CONV_THRESH) / 10, estimated_linewidth))
                line_right.append((max_arg + estimated_linewidth / 2., (i + 0.5) * STEPS_N, 2))
                line_left.append((max_arg - estimated_linewidth / 2., (i + 0.5) * STEPS_N, 2))

        center_xsample = []
        center_ysample = []
        center_weights = []
        for item in line_center:
            cv2.circle(im, (int(item[0]), int(item[1])), 2, (0, 0, 255), thickness=int(item[2]))
            center_xsample.append(item[0])
            center_ysample.append(item[1])
            center_weights.append(item[2] * ((estimated_linewidth / local_max_linewidth) ** 2))

        if global_linewidth_estimation is None:
            global_linewidth_estimation = local_max_linewidth
        else:
            global_linewidth_estimation = 0.9 * global_linewidth_estimation + 0.1 * local_max_linewidth

        try:
            xsample = np.array(center_xsample)
            ysample = np.array(center_ysample)
            weights = np.array(center_weights)
            xavg = np.average(xsample, weights=weights)
            yavg = np.average(ysample, weights=weights)
            xstd = np.sqrt(np.sum(weights * (xsample - xavg) ** 2) / np.sum(weights))
            ystd = np.sqrt(np.sum(weights * (ysample - yavg) ** 2) / np.sum(weights))
            cov = np.sum(weights * (xsample - xavg) * (ysample - yavg)) / np.sum(weights)
            corr = cov / (xstd * ystd)
            betahat = corr * xstd / ystd
            alphahat = xavg - yavg * betahat

            #calculate deviation / angle
            m = (betahat * (height/2) + alphahat, height/2)
            t = (m[0] + SPEED_RATIO*height*math.cos(math.atan(1/betahat)), m[1] - SPEED_RATIO*height*math.sin(math.atan(1/betahat)))
            v1 = (t[0] - width/2, t[1]-height/2)
            print 'vdiff', v1
            print 'alphabet', betahat
            cv2.line(im,
                     (int(alphahat), 0),
                     (int(alphahat + betahat * height), height), (255, 0, 255), 2)
            cv2.line(im,
                     (int(alphahat + global_linewidth_estimation / 2), 0),
                     (int(alphahat + betahat * height + global_linewidth_estimation / 2), height), (0, 255, 0), 2)
            cv2.line(im,
                     (int(alphahat - global_linewidth_estimation / 2), 0),
                     (int(alphahat + betahat * height - global_linewidth_estimation / 2), height), (0, 255, 0), 2)
        except:
            print ("To minimal")
    except:
        print "Cannot Find Line"

    e2 = cv2.getTickCount()
    t = (e2 - e1)/cv2.getTickFrequency()
    cv2.imwrite(path + 'detect_' + filename, im)
    print 'Task complete in ', t, 'secs (', 1./t, 'fps)'
    return True


fileId = 0
while True:
    fileId += 1
    path = "/Users/wjuni/ffmpeg/"
    filename = "frame%04d.jpg" % (fileId)
    if not os.path.isfile(path + filename):
        break
    print filename
    process(path, filename, False, 1)
