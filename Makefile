CC=g++
CFLAGS = -std=c++11 -g -DRASPBERRY_PI
TARGET = raccoon

SRCDIR = raspi
OBJDIR = raspi/obj

SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

INCLUDES = -I/usr/local/include/opencv -I/usr/local/include -I/usr/include/python2.7
LFLAGS = -L/usr/local/lib
LIBS = -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dpm -lopencv_freetype -lopencv_fuzzy -lopencv_line_descriptor -lopencv_optflow -lopencv_reg -lopencv_saliency -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_rgbd -lopencv_surface_matching -lopencv_tracking -lopencv_datasets -lopencv_text -lopencv_face -lopencv_plot -lopencv_dnn -lopencv_xfeatures2d -lopencv_shape -lopencv_video -lopencv_ximgproc -lopencv_calib3d -lopencv_features2d -lopencv_flann -lopencv_xobjdetect -lopencv_objdetect -lopencv_ml -lopencv_xphoto -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_photo -lopencv_imgproc -lopencv_core -lpthread -lpython2.7

all: dirs $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o ../$@ $(LFLAGS) $(LIBS) $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	g++ $(CFLAGS) $(INCLUDES) -c -o $@ $<

dirs:
	@mkdir -p $(OBJDIR)

clean:
	rm ../$(TARGET) obj/*.o

