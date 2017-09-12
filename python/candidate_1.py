import cv2
import os.path
import numpy as np
import threading
import math

NUM_CORE = 8
MAX_DEV_PIX = 150
DEV_PIX_SENS = 4
SPEED_RATIO = 0.3
class ThreadTiltEstimation (threading.Thread):
    def __init__(self, _min, _max, _step):
        threading.Thread.__init__(self)
        self.min = _min
        self.max = _max
        self.step = _step

    def run(self):
        global frame_threshed, estimated_linewidth, moment_x, height, width
        best_fit_param = []
        best_fit_area = 0
        best_percentage = 0

        for offset in range(self.min, self.max, self.step):
            est_mask = np.zeros((height, width, 1), np.uint8)
            est_line_left = moment_x - estimated_linewidth / 2 + offset
            est_line_right = moment_x + estimated_linewidth / 2 + offset
            est_line_left_btm = moment_x - estimated_linewidth / 2 - offset
            est_line_right_btm = moment_x + estimated_linewidth / 2 - offset
            sq = np.array(
               [(est_line_left, 0), (est_line_right, 0), (est_line_right_btm, height), (est_line_left_btm, height)],
               np.int32)
            cv2.fillConvexPoly(est_mask, sq, (255))
            filest = cv2.multiply(frame_threshed, est_mask)
            mask_area = cv2.countNonZero(est_mask)
            area = cv2.countNonZero(filest)
            if area > best_fit_area:
                best_fit_area = area
                best_percentage = float(area) / float(mask_area)
                best_fit_param = (est_line_left, est_line_right, est_line_left_btm, est_line_right_btm, offset)

        self.data = best_fit_area, best_percentage, best_fit_param


def process(path, filename, rotate):
    global frame_threshed, estimated_linewidth, moment_x, moment_y, height, width
    if not os.path.isfile(path + filename):
        return False

    im = cv2.imread(path + filename)
    e1 = cv2.getTickCount()

    if rotate:
        im = cv2.rotate(im, cv2.ROTATE_90_CLOCKWISE, im)

    height, width, channels = im.shape
    im_hsv = cv2.cvtColor(im, cv2.COLOR_BGR2HSV)

    ORANGE_MIN = np.array([15, 70, 100],np.uint8)
    ORANGE_MAX = np.array([30, 255, 255],np.uint8)

    frame_threshed = cv2.inRange(im_hsv, ORANGE_MIN, ORANGE_MAX)
    #filtered = cv2.bitwise_and(im, im, mask=frame_threshed)

    mom =  cv2.moments(frame_threshed, True)
    moment_x = mom['m10'] / mom['m00']
    moment_y = mom['m01'] / mom['m00']

    # cv2.imwrite(path + filename + '_out.jpg', frame_threshed)
    # cv2.imwrite(path + filename + '_out0.jpg', filtered)

    # cv2.circle(im, (width / 2, height / 2), 2, (255, 0, 0), thickness=3)
    cv2.circle(im, (int(moment_x), int(moment_y)), 2, (255, 0, 255), thickness=15)

    cv2.imwrite(path + 'threshed_com.jpg', im)

    best_fit_param = []
    best_fit_area = 0
    best_percentage = 0
    linewidth_compensation = 1.
    allocation = 2 * MAX_DEV_PIX / NUM_CORE
    for i in range(3):
        estimated_linewidth = cv2.countNonZero(frame_threshed) / height * linewidth_compensation
        thds = []
        for core in range(NUM_CORE):
            t = ThreadTiltEstimation(allocation*core - MAX_DEV_PIX, allocation*(core+1) - MAX_DEV_PIX, DEV_PIX_SENS)
            t.start()
            thds.append(t)

        for t in thds:
            t.join()
            if best_fit_area < t.data[0]:
                best_fit_area, best_percentage, best_fit_param = t.data

        linewidth_compensation /= np.power(best_percentage, 1./(i+1))

        # COM compensation in 3 sections
        est_line_left, est_line_right, est_line_left_btm, est_line_right_btm, offset = best_fit_param
        midPoint = (moment_x-estimated_linewidth/2, moment_x, moment_x+estimated_linewidth/2)
        mask_sect = []
        crack_area = []
        for sect in range(3):
            mask = np.zeros((height, width, 1), np.uint8)
            sq = np.array(
                [(est_line_left, 0), (est_line_left+sect*estimated_linewidth/3, 0),
                 (est_line_left_btm+sect*estimated_linewidth/3, height), (est_line_left_btm, height)], np.int32)
            cv2.fillConvexPoly(mask, sq, (255))
            masked_frame = cv2.multiply(frame_threshed, mask)
            crack_area_current = cv2.countNonZero(mask) - cv2.countNonZero(masked_frame)
            mask_sect.append(mask)
            crack_area.append(crack_area_current)

        whole_crack_area = cv2.countNonZero(frame_threshed)

        sum_denom_x = sum_denom_y = whole_crack_area + sum(crack_area)
        sum_nom_x = moment_x * whole_crack_area
        sum_nom_y = moment_y * whole_crack_area
        for sect in range(3):
            sum_nom_x += midPoint[sect] * crack_area[sect]
            sum_nom_y += height / 2 * crack_area[sect]
        moment_x = sum_nom_x / sum_denom_x
        moment_y = sum_nom_y / sum_denom_y

    cv2.line(im, (int(best_fit_param[0]), 0), (int(best_fit_param[2]), height), (0, 0, 255), 2)
    cv2.line(im, (int(best_fit_param[1]), 0), (int(best_fit_param[3]), height), (0, 0, 255), 2)

    newm = np.inf # in case of vertical line
    if best_fit_param[0] != best_fit_param[2]:
        newm = (best_fit_param[2] - best_fit_param[0]) / height

    # passing ((best_fit_param[0] + best_fit_param[1])/2, 0)
    newx = (best_fit_param[0] + best_fit_param[1])/2
    cv2.line(im,
             (int((best_fit_param[0] + best_fit_param[1]))/2, 0),
             (int((best_fit_param[2] + best_fit_param[3]))/2, height), (255, 0, 255), 2)

    # in case of vertical line
    act_deg = 0
    deviation = width/2 - (best_fit_param[0] + best_fit_param[1])/2

    if best_fit_param[0] != best_fit_param[2]:  # if not vertical
        act_deg = np.arctan(1. / newm) * 180 / np.pi + 90
        if act_deg > 90: act_deg = act_deg - 180
        # x - newm*y - newx = 0 , (width/2, height/2)
        deviation = (width / 2 - newm * height / 2 - newx) / np.sqrt(1 + newm * newm)
        m=(newm*height/2+newx, height/2)
        t=(m[0]+SPEED_RATIO*height*math.cos(math.atan(1/newm)), m[1] - SPEED_RATIO*height*math.sin(math.atan(1/newm)))
        v1 = (t[0] - width / 2, t[1] - height / 2)
        w=newm
    else:
        print 'm not defined'
        v1=(0, -SPEED_RATIO*height)
        w=0
    print m
    print t
    print 'vdiff', v1
    print 'alphabet', w
    print 'Direction Degree = ', act_deg, 'Deviation from Center = ', deviation, ', (x=', deviation * np.cos(
        act_deg * np.pi / 180), ', y=', - deviation * np.sin(act_deg * np.pi / 180), ')'

    e2 = cv2.getTickCount()
    t = (e2 - e1)/cv2.getTickFrequency()
    cv2.imwrite(path + 'detect_' + filename, im)

    print 'Task complete in ', t, 'secs (', 1./t, 'fps)'
    return True


fileId = 0
while True:
    fileId += 1
    path = ""
    filename = "frame%04d.jpg" % (fileId)
    if not os.path.isfile(path + filename):
        break
    print filename
    process(path, filename, False)
