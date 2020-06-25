OBJS	= master.o worker.o list.o whoClient.o whoServer.o circular.o
OUT	= master worker whoClient whoServer

OBJS0	= master.o
SOURCE0	= master.c
HEADER0	= 
OUT0	= master

OBJS1	= worker.o list.o circular.o
SOURCE1	= worker.c list.c circular.c
HEADER1	= 
OUT1	= worker

OBJS2	= whoClient.o
SOURCE2	= whoClient.c
HEADER2	= 
OUT2	= whoClient

OBJS3	= whoServer.o circular.o
SOURCE3	= whoServer.c circular.c
HEADER3	= 
OUT3	= whoServer

CC	 = gcc
FLAGS	 = -g -c -Wall
LFLAGS	 =
LIBS=-pthread

all: master worker whoClient whoServer

master: $(OBJS0) $(LFLAGS)
	$(CC) -g $(OBJS0) -o $(OUT0) $(LIBS)

worker: $(OBJS1) $(LFLAGS)
	$(CC) -g $(OBJS1) -o $(OUT1) $(LIBS)

whoClient: $(OBJS2) $(LFLAGS)
	$(CC) -g $(OBJS2) -o $(OUT2) $(LIBS)

whoServer: $(OBJS3) $(LFLAGS)
	$(CC) -g $(OBJS3) -o $(OUT3) $(LIBS)

master.o: master.c
	$(CC) $(FLAGS) master.c

worker.o: worker.c
	$(CC) $(FLAGS) worker.c 

whoClient.o: whoClient.c
	$(CC) $(FLAGS) whoClient.c 

whoServer.o: whoServer.c
	$(CC) $(FLAGS) whoServer.c 

list.o: list.c
	$(CC) $(FLAGS) list.c 


clean:
	rm -f $(OBJS) $(OUT)
