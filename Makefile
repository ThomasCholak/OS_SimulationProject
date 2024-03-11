CC = g++
CFLAGS = -g3
TARGET1 = oss
TARGET2 = worker

OBJS1 = oss.o
OBJS2 = worker.o

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1)
	$(CC) -o $@ $(OBJS1)

$(TARGET2): $(OBJS2)
	$(CC) -o $@ $(OBJS2)

oss.o: oss.cpp oss.h
	$(CC) $(CFLAGS) -c oss.cpp

worker.o: worker.cpp oss.h
	$(CC) $(CFLAGS) -c worker.cpp

clean:
	/bin/rm -f *.o $(TARGET1) $(TARGET2)