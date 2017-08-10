import cv2
import os.path
import numpy as np
import matplotlib.pyplot as plt


LINEWIDTH_DEFAULT = 0.220 # of screen width
TARGET_FRAMESIZE = (240, 136)
N = 8
CONV_THRESH = 50  # change according to the screen width (30% of width is appropriate)
LINE_MARGIN_RATIO = 1.2

LINEWIDTH_INITIAL = LINEWIDTH_DEFAULT * TARGET_FRAMESIZE[0]
STEPS_N = TARGET_FRAMESIZE[1] / N

def process(path, filename, rotate, lines):
    if not os.path.isfile(path + filename):
        return False

    im = cv2.imread(path + filename)
    e1 = cv2.getTickCount()

    if rotate:
        im = cv2.rotate(im, cv2.ROTATE_90_CLOCKWISE, im)

    im = cv2.resize(im, TARGET_FRAMESIZE, 0, 0, cv2.INTER_CUBIC)

    height, width = TARGET_FRAMESIZE

    im_hsv = cv2.cvtColor(im, cv2.COLOR_BGR2HSV)

    ORANGE_MIN = np.array([15, 70, 100],np.uint8)
    ORANGE_MAX = np.array([30, 250, 255],np.uint8)

    frame_threshed = cv2.inRange(im_hsv, ORANGE_MIN, ORANGE_MAX)

    line_center = []
    line_left = []
    line_right = []
    for i in range(N):
        cropped = frame_threshed[i*STEPS_N:(i+1)*STEPS_N, :]
        vec_avg = np.average(cropped, axis=0)
        max_val = np.max(vec_avg)
        normalized_vec = vec_avg / max_val * 2. - 1.
        # print max_val
        # print normalized_vec
        estimated_linewidth = (np.sum(vec_avg) / 255)
        print 'linewidth', estimated_linewidth

        conv_filter = np.zeros(shape=(int(estimated_linewidth*1.5)))
        conv_filter[:] = -1
        conv_filter[int(estimated_linewidth/4):int(estimated_linewidth*5/4)] = 1
        # print conv_filter

        conv = np.convolve(normalized_vec, conv_filter)
        # plt.figure()
        # plt.plot(conv)
        # plt.show()
        # print conv
        # print len(conv_filter), len(normalized_vec), len(conv)
        # print np.max(conv), np.argmax(conv)
        for l in range(lines):
            max_v = np.max(conv)
            max_arg = np.argmax(conv)
            if max_v < CONV_THRESH:
                break
            max_arg -= estimated_linewidth*0.75

            left_line = max_arg - estimated_linewidth/2.*LINE_MARGIN_RATIO
            right_line = max_arg + estimated_linewidth/2.*LINE_MARGIN_RATIO
            conv[int(left_line):int(right_line)] = 0
            line_center.append((max_arg, (i+0.5)*STEPS_N, (max_v-CONV_THRESH)/10))
            line_right.append((max_arg+estimated_linewidth/2., (i + 0.5) * STEPS_N, 2))
            line_left.append((max_arg-estimated_linewidth/2., (i + 0.5) * STEPS_N, 2))

    # print line_center
    # print LINEWIDTH_INITIAL
    for item in line_center:
        cv2.circle(im, (int(item[0]), int(item[1])), 2, (0, 0, 255), thickness=int(item[2]))
    for item in line_left:
        cv2.circle(im, (int(item[0]), int(item[1])), 2, (255, 0, 0), thickness=int(item[2]))
    for item in line_right:
        cv2.circle(im, (int(item[0]), int(item[1])), 2, (255, 0, 0), thickness=int(item[2]))

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


fileId = 0
while fileId< 149:
    fileId += 1
    path = "/Users/wjuni/ffmpeg/"
    filename = "frame%04d.jpg" % (fileId)
    print filename
    if not process(path, filename, True, 1):
        break
process('/Users/wjuni/', 'test7.jpg', False, 1)
# raw_input()
# process("/Users/wjuni/ffmpeg/", 'test_graphics.jpg', False, 1)