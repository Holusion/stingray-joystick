
all: build


build:
	g++ joystick.cpp -I./include  -std=c++11 -fPIC -DPIC -shared -o joystick.so -Wl,--whole-archive -levdev -Wl,--no-whole-archive 
