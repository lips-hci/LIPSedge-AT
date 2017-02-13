SRC = ./NiPointClient.cpp
CPPFLAGS = -Dlinux -DRES_VGA -I../../Include -std=c++11
LDFLAGS = -L../../Lib -lOpenNI

# OpenCV
CPPFLAGS += -I/usr/local/include -O2
LDFLAGS += -L/usr/local/lib -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_viz

LOCAL_TARGET = NiPointClient

all: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(SRC)
	$(CXX) $(SRC) $(CPPFLAGS) $(LDFLAGS) -o $@

clean:
	rm -rf $(LOCAL_TARGET)
