import cv2
import os.path
import numpy as np
from pylab import figure, axis, pie, title, plot, show, savefig, xlabel, ylabel, annotate, close


LINEWIDTH_DEFAULT = 0.220 # of screen width
# TARGET_FRAMESIZE = (240, 136)
TARGET_FRAMESIZE = (854, 480)
N = 16
CONV_THRESH = 25  # change according to the screen width (30% of width is appropriate)
LINE_MARGIN_RATIO = 1.2

LINEWIDTH_INITIAL = LINEWIDTH_DEFAULT * TARGET_FRAMESIZE[0]
STEPS_N = TARGET_FRAMESIZE[1] / N

global_linewidth_estimation = None

def process(path, filename, rotate, lines):
    global global_linewidth_estimation

    if not os.path.isfile(path + filename):
        return False
    # if the file is not exist, return

    im = cv2.imread(path + filename)
    e1 = cv2.getTickCount()
    # initialize tick counter to further count time

    if rotate:
        im = cv2.rotate(im, cv2.ROTATE_90_CLOCKWISE, im)
    # if the image is rotated, rotate the image

    # im = cv2.resize(im, TARGET_FRAMESIZE, 0, 0, cv2.INTER_CUBIC)

    height, width = TARGET_FRAMESIZE

    im_hsv = cv2.cvtColor(im, cv2.COLOR_BGR2HSV)

    ORANGE_MIN = np.array([15, 70, 100],np.uint8)
    ORANGE_MAX = np.array([30, 250, 255],np.uint8)

    frame_threshed = cv2.inRange(im_hsv, ORANGE_MIN, ORANGE_MAX)
    # convert the image into W/B image and threshold it

    line_center = []
    line_left = []
    line_right = []

    local_max_linewidth = 0

    for i in range(N):
        cropped = frame_threshed[i*STEPS_N:(i+1)*STEPS_N, :]
        vec_avg = np.average(cropped, axis=0)
        max_val = np.max(vec_avg)
        normalized_vec = vec_avg / max_val * 2. - 1.
        # print max_val
        # print normalized_vec
        estimated_linewidth = (np.sum(vec_avg) / 255)
        print 'linewidth', estimated_linewidth
        if estimated_linewidth > local_max_linewidth :
            local_max_linewidth = estimated_linewidth

        # figure()
        # title('Distribution of Thresholded Pixels')
        # plot(normalized_vec)
        # ylabel('number of thres. pixels in column (normalized)')
        # xlabel('horizontal axis (px)')
        # savefig('normalized_vec.png', transparent=True)

        conv_filter = np.zeros(shape=(int(estimated_linewidth*1.5)))
        conv_filter[:] = -1
        conv_filter[int(estimated_linewidth/4):int(estimated_linewidth*5/4)] = 1
        # print conv_filter

        conv = np.convolve(normalized_vec, conv_filter)
        figure()
        plot(conv)
        title('Convolution Result')
        xlabel('(px)')
        t = np.arange(0., len(conv))
        t2 = np.arange(0., len(conv))
        t2.fill(75.)
        plot(t, t2, 'r--')
        axis([200, 900, -150, 250])
        savefig('convolution_result_' + str(i) + '.png', transparent=True)
        close()

        print conv
        print len(conv_filter), len(normalized_vec), len(conv)
        print np.max(conv), np.argmax(conv)
        for l in range(lines):
            max_v = np.max(conv)
            max_arg = np.argmax(conv)
            if max_v < CONV_THRESH:
                break
            max_arg -= estimated_linewidth*0.75

            left_line = max_arg - estimated_linewidth/2.*LINE_MARGIN_RATIO
            right_line = max_arg + estimated_linewidth/2.*LINE_MARGIN_RATIO
            conv[int(left_line):int(right_line)] = 0
            line_center.append((max_arg, (i+0.5)*STEPS_N, (max_v-CONV_THRESH)/10, estimated_linewidth))
            line_right.append((max_arg+estimated_linewidth/2., (i + 0.5) * STEPS_N, 2))
            line_left.append((max_arg-estimated_linewidth/2., (i + 0.5) * STEPS_N, 2))

    # print line_center
    # print LINEWIDTH_INITIAL
    center_xsample = []
    center_ysample = []
    center_weights = []
    for item in line_center:
        cv2.circle(im, (int(item[0]), int(item[1])), 2, (0, 0, 255), thickness=int(item[2]))
        center_xsample.append(item[0])
        center_ysample.append(item[1])
        center_weights.append(item[2]*((estimated_linewidth/local_max_linewidth)**2))

    if global_linewidth_estimation is None:
        global_linewidth_estimation = local_max_linewidth
    else:
        global_linewidth_estimation = 0.9 * global_linewidth_estimation + 0.1 * local_max_linewidth
    print global_linewidth_estimation

    try:
        xsample = np.array(center_xsample)
        ysample = np.array(center_ysample)
        weights = np.array(center_weights)
        xavg = np.average(xsample, weights=weights)
        yavg = np.average(ysample, weights=weights)
        xstd = np.sqrt(np.sum(weights * (xsample-xavg) **2) / np.sum(weights))
        ystd = np.sqrt(np.sum(weights * (ysample-yavg) **2) / np.sum(weights))
        cov = np.sum(weights * (xsample - xavg) * (ysample - yavg)) / np.sum(weights)
        corr = cov / (xstd * ystd)
        # print corr, np.corrcoef(xsample, ysample)[0, 1]
        betahat = corr * xstd / ystd
        alphahat = xavg - yavg * betahat
        print betahat, alphahat
        # cv2.line(im,
        #          (int(alphahat), 0),
        #          (int(alphahat + betahat * height), height), (255, 0, 255), 2)
        # cv2.line(im,
        #          (int(alphahat + global_linewidth_estimation / 2), 0),
        #          (int(alphahat + betahat * height + global_linewidth_estimation / 2), height), (0, 255, 0), 2)
        # cv2.line(im,
        #          (int(alphahat - global_linewidth_estimation / 2), 0),
        #          (int(alphahat + betahat * height - global_linewidth_estimation / 2), height), (0, 255, 0), 2)
    except:
        print ("To minimal")

    # alpha = np.average(xsample) - average

    #filtered = cv2.bitwise_and(im, im, mask=frame_threshed)


    # cv2.imwrite(path + filename + '_out.jpg', frame_threshed)
    # cv2.imwrite(path + filename + '_out0.jpg', filtered)

    # cv2.circle(im, (width / 2, height / 2), 2, (255, 0, 0), thickness=3)
    # cv2.circle(im, (int(moment_x), int(moment_y)), 2, (255, 0, 255), thickness=3)


    e2 = cv2.getTickCount()
    t = (e2 - e1)/cv2.getTickFrequency()
    cv2.imwrite(path + 'detect_' + filename, im)
    cv2.imwrite(path + 'thresh_' + filename, frame_threshed)

    print 'Task complete in ', t, 'secs (', 1./t, 'fps)'
    return True


#
# figure()
# # title('Distribution of Thresholded Pixels')
# conv_filter = np.zeros(shape=(int(160 * 1.5)))
# conv_filter[:] = -1
# conv_filter[int(160 / 4):int(160 * 5 / 4)] = 1
#
# plot(conv_filter)
# xlabel('(px)')
# savefig('filter.png', transparent=True)

fileId = 0
while fileId< 1:
    fileId += 1
    path = "/Users/wjuni/ffmpeg/"
    filename = "frame%04d.jpg" % (fileId)
    print filename
    if not process(path, filename, True, 1):
        break
process('/Users/wjuni/', 'test7.jpg', False, 1)
# raw_input()
# process("/Users/wjuni/ffmpeg/", 'test_graphics.jpg', False, 1)