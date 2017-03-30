SRC = ./Ni9PixelTest.cpp
CPPFLAGS = -Dlinux -I/usr/include/ni -std=c++11
LDFLAGS = -L/usr/lib -lOpenNI

LOCAL_TARGET = Ni9PixelTest

all: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(SRC)
	$(CXX) $(SRC) $(CPPFLAGS) $(LDFLAGS) -o $@

clean:
	rm -rf $(LOCAL_TARGET)
