SRC = ./NiUDPServer.cpp
CPPFLAGS = -Dlinux -DRES_VGA -DCOMPRESS -I../../Include -std=c++11 -O2
LDFLAGS = -L../../Lib -lOpenNI

# zlib
CPPLAGS += -I/zlib/include
LDFLAGS += -L/zlib/lib -lz

LOCAL_TARGET = NiUDPServer

all: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(SRC)
	$(CXX) $(SRC) $(CPPFLAGS) $(LDFLAGS) -o $@

clean:
	rm -rf $(LOCAL_TARGET)
