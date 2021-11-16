CC = g++
CFLAGS = -c -Wall
LDFLAGS=-Wl,--no-as-needed -lrt
SOURCES = src/main.cpp src/arq_line.cpp src/gec.cpp src/mtpm.cpp src/playback_buffer.cpp src/plr_counter.cpp src/simple_c.cpp src/socket_io.cpp src/VQpriority.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=arqsink

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) src/*.o
