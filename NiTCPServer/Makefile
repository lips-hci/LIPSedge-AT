SRC = ./NiTCPServer.cpp
CPPFLAGS = -Dlinux -I../../Include -std=c++11 -O2
LDFLAGS = -L../../Lib -lOpenNI

# zlib
CPPFLAGS += -I./zlib/include
LDFLAGS += -L./zlib/lib -lz

LOCAL_TARGET = NiTCPServer

all: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(SRC)
	$(CXX) $(SRC) $(CPPFLAGS) $(LDFLAGS) -o $@

clean:
	rm -rf $(LOCAL_TARGET)
