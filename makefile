LIBS=`pkg-config --libs gtk+-3.0,gstreamer-1.0 --cflags gtk+-3.0,gstreamer-1.0`
all:
	gcc main.cpp $(LIBS) -o english_tutor
install:
	install english_tutor /usr/local/bin
clean:
	rm english_tutor
