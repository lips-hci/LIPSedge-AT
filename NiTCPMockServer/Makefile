SRC = ./NiTCPMockServer.cpp
CPPFLAGS = -Dlinux -I../../Include -std=c++11 -O2
LDFLAGS = -L../../Lib -lOpenNI

LOCAL_TARGET = NiTCPMockServer

all: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(SRC)
	$(CXX) $(SRC) $(CPPFLAGS) $(LDFLAGS) -o $@

clean:
	rm -rf $(LOCAL_TARGET)
