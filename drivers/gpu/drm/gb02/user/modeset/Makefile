FLAGS=`pkg-config --cflags --libs libdrm`
FLAGS+=-Wall -O0 -g
FLAGS+=-D_FILE_OFFSET_BITS=64

all:

	gcc -g -o modeset modeset.c $(FLAGS)
	gcc -g -o modeset-double-buffered modeset-double-buffered.c $(FLAGS)
	gcc -g -o modeset-vsync modeset-vsync.c $(FLAGS)
	gcc -g -o modeset-atomic-crtc modeset-atomic-crtc.c $(FLAGS)
	gcc -g -o modeset-atomic-plane modeset-atomic-plane.c $(FLAGS)
	gcc -g -o main main.c $(FLAGS)

clean:
	rm -rf main modeset modeset-double-buffered modeset-vsync modeset-atomic-plane modeset-atomic-crtc