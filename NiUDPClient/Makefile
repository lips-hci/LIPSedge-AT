SRC = ./NiUDPClient.cpp
CPPFLAGS = -Dlinux -DRES_VGA -DCOMPRESS -I../../Include -std=c++11
LDFLAGS = -L../../Lib -lOpenNI

# OpenCV
CPPFLAGS += -I/usr/local/include -O2
LDFLAGS += -L/usr/local/lib -lopencv_core -lopencv_highgui -lopencv_imgproc

# zlib
CPPFLAGS += -I./zlib/include
LDFLAGS += -L./zlib/lib -lz

LOCAL_TARGET = NiUDPClient

all: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(SRC)
	$(CXX) $(SRC) $(CPPFLAGS) $(LDFLAGS) -o $@

clean:
	rm -rf $(LOCAL_TARGET)
