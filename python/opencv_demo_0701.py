import cv2
import numpy as np

# def solve_linear_eq(x1, x2, y1, y2, m, x12, x22, y12, y22, m2):
#     denom = m * x1 - m2 * x12  - y1 + y12
#     nom = m-m2
#     if nom == 0:
#         return None
#     cross_x = denom / nom
#     cross_y = m * (cross_x - x1) + y1
#     print cross_x, cross_y
#     return cross_x, cross_y

def filter_and_convert_lines(lines, target_amt):
    result = []
    if lines is None : return result, False
    for i in lines:
        for rho,theta in i:
            a = np.cos(theta)
            b = np.sin(theta)
            x0 = a*rho
            y0 = b*rho
            x1 = (x0 + 1000*(-b))
            y1 = (y0 + 1000*(a))
            x2 = (x0 - 1000*(-b))
            y2 = (y0 - 1000*(a))
            if x1 == x2:
                continue
            m = float(y2-y1)/float(x2-x1)
            if np.abs(m) >= 1:
                flag_save = True
                for prev in result:
                    if abs(prev[4]-rho) < 15 and abs(prev[5]-theta) < 0.1:
                        #print 'Remove Redundancy'
                        flag_save = False
                        break
                if flag_save:
                    result.append((x1, x2, y1, y2, rho, theta, m))

    min_degdiff = 100.
    min_line1 = None
    min_line2 = None

    if len(result) > target_amt and len(result) <= 20:
        for i in range(len(result)):
            for j in range(len(result)):
                if i < j and len(result) > j and len(result) > i:
                    if min_degdiff > np.abs(result[i][5] - result[j][5]):
                        min_degdiff = np.abs(result[i][5] - result[j][5])
                        min_line1 = result[i]
                        min_line2 = result[j]

                    # if np.abs(result[i][5] - result[j][5]) > 0.08: # 10 deg
                    #     print 'delete ', result[i][5] , result[j][5]
                    #     del result[j]


#    print len(result), min_degdiff



    # for i in range(len(result)):
    #     for j in range(len(result)):
    #         if i < j and len(result) > j and len(result) > i:
    #             # (y2-y1)/(x2-x1) * x1 - (y2'-y1')/(x2'-x1') * x1'  - y1 + y1' = ( (y2-y1)/(x2-x1) -  (y2'-y1')/(x2'-x1') )  * x
    #
    #             x1 = float(result[i][0])
    #             x2 = float(result[i][1])
    #             y1 = float(result[i][2])
    #             y2 = float(result[i][3])
    #             x12 = float(result[j][0])
    #             x22 = float(result[j][1])
    #             y12 = float(result[j][2])
    #             y22 = float(result[j][3])
    #             denom = ((y2-y1)/(x2-x1) * x1 - (y22-y12)/(x22-x12) * x12  - y1 + y12)
    #             nom = ( (y2-y1)/(x2-x1) -  (y22-y12)/(x22-x12))
    #             if nom == 0:
    #                 continue
    #             cross_x = denom / nom
    #             cross_y = (y2-y1)/(x2-x1) * (cross_x - x1) + y1
    #             if cross_x >= 0 and cross_x < 1000 and cross_y >=0 and cross_y < 1000:
    #                 del result[j]
    #             print cross_x, cross_y
    if min_degdiff < 0.05:
        return [min_line1, min_line2], True
    if len(result) == 0:
        return [], False
    return result, False

def process(filename, line_cnt):
    im = cv2.imread(filename)
    e1 = cv2.getTickCount()
    height, width, channels = im.shape
    im_hsv = cv2.cvtColor(im, cv2.COLOR_BGR2HSV)


    ORANGE_MIN = np.array([15, 70, 100],np.uint8)
    ORANGE_MAX = np.array([30, 250, 255],np.uint8)

    frame_threshed = cv2.inRange(im_hsv, ORANGE_MIN, ORANGE_MAX)
    # print 'Nonzeropix = ' , cv2.countNonZero(frame_threshed)
    frame_threshed = cv2.GaussianBlur(frame_threshed, (35,35), 0)

    filtered = cv2.bitwise_and(im, im, mask=frame_threshed)

    gray = cv2.cvtColor(filtered, cv2.COLOR_BGR2GRAY)
    edges = cv2.Canny(gray,300 , 500, apertureSize=3)

    cv2.imwrite(filename+'_out0.jpg', edges)
    hough_threshold = 70
    iter = 0

    cartesian_line = []
    found_exact = False
    last_met_cartesian_line = []
    while True:
        lines = cv2.HoughLines(edges, 1, np.pi/180, hough_threshold)
        iter += 1
        if iter > 20 or hough_threshold < 0: break
        # if len(lines) > line_cnt * 4:
        #     hough_threshold += 10
        #     if len(last_met_cartesian_line) == 0 : cartesian_line = filter_and_convert_lines(lines)
        #     continue
        cartesian_line, houristic = filter_and_convert_lines(lines, line_cnt)

        if houristic:
            found_exact = True
            print 'Houristic Decision'
            break

        if len(cartesian_line) == 0:
            hough_threshold -= 10
        elif len(cartesian_line) < line_cnt*2:
            hough_threshold -= 1
        elif len(cartesian_line) > line_cnt*4:
            hough_threshold += 10
            last_met_cartesian_line = cartesian_line
        elif len(cartesian_line) > line_cnt*2:
            hough_threshold += 1
            last_met_cartesian_line = cartesian_line
        else:
            found_exact = True
            break
    if not found_exact:
        cartesian_line = last_met_cartesian_line

    # print "Selected Threshold="  , hough_threshold , "Lines = ", len(cartesian_line)
    # rhosum = 0
    # thetasum = 0
    # let y = 0, then  x1 - y1/m = x
    msum = 0
    xsum = 0
    for i in cartesian_line:
        cv2.line(im,(int(i[0]),int(i[2])),(int(i[1]),int(i[3])),(0,0,255),2)
        # rhosum += i[4]
        # thetasum += i[5]
        xsum += i[0] - i[2]/i[6]
        msum += 1./i[6]

    # if found_exact:
    # newrho = rhosum / len(cartesian_line)
    # newtheta = thetasum/ len(cartesian_line)

    if len(cartesian_line) == 0:
        print "Cannot Find Proper Line"
        return

    newm = msum / len(cartesian_line)
    newx = xsum / len(cartesian_line)
    # x = newm * y + newx
    # (newx, 0) (newm*100 + newx,100)
    try:
        cv2.line(im, (int(newx), int(0)), (int(newm * width * 2 + newx), int(width * 2)), (255, 0, 255), 2)
    except:
        pass
    # print newrho, newtheta
    # a = np.cos(newtheta)
    # b = np.sin(newtheta)
    # x0 = a * newrho
    # y0 = b * newrho
    # x1 = int(x0 + 1000 * (-b))
    # y1 = int(y0 + 1000 * (a))
    # x2 = int(x0 - 1000 * (-b))
    # y2 = int(y0 - 1000 * (a))

    # cv2.line(im, (int(x1), int(y1)), (int(x2), int(y2)), (255, 0, 255), 2)
    # act_deg2 = newtheta * 180 / np.pi
    # if act_deg2 < -90 : act_deg2 += 180
    # if act_deg2 > 90 : act_deg2 -= 180
    act_deg = np.arctan(1./newm) * 180 / np.pi + 90
    if act_deg > 90 : act_deg = act_deg - 180
    print 'Direction Degree = ' , act_deg, 'Exact = ', found_exact

    # x - newm*y - newx = 0 , (width/2, height/2)
    deviation = (width/2 - newm*height/2 - newx) / np.sqrt(1 + newm*newm)
    print 'Deviation from Center = ', deviation, ', (x=', deviation * np.cos(act_deg * np.pi / 180) , ', y=', - deviation * np.sin(act_deg * np.pi / 180) , ')'

    e2 = cv2.getTickCount()
    t = (e2 - e1)/cv2.getTickFrequency()

    print 'Task complete in ', t, 'secs (', 1./t, 'fps)'

    cv2.circle(im, (width/2, height/2), 2, (255, 0, 0), thickness=3)
    cv2.imwrite(filename+'_out.jpg', frame_threshed)
    cv2.imwrite(filename+'_out1.jpg', filtered)
    cv2.imwrite(filename+'_out2.jpg', im)

for i in range(500):
    path = "/Users/wjuni/ffmpeg/frame%04d.jpg" % (i+1)
    print path
    process(path, 1)