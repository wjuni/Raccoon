CC=g++
CFLAGS = -std=c++11 -g -DRASPBERRY_PI
TARGET = raccoon

SRCDIR = raspi
OBJDIR = raspi/obj

SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

INCLUDES = -I/usr/local/include/opencv -I/usr/local/include -I/usr/include/python2.7
LFLAGS = -L/usr/local/lib
LIBS = -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs -lopencv_core -lpthread -lpython2.7

all: dirs $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o ../$@ $^ $(LIBS) $(LFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	g++ $(CFLAGS) $(INCLUDES) -c -o $@ $<

dirs:
	@mkdir -p $(OBJDIR)

clean:
	rm ../$(TARGET) obj/*.o

