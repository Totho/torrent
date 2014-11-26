#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utp/zmq/lib/
CXX=g++-4.7 -std=c++11
ZMQ=/home/utp/zmq
ZMQ_INCLUDES=$(ZMQ)/include
ZMQ_LIBS=$(ZMQ)/lib

all: track client

track: track.o
	$(CXX) -L$(ZMQ_LIBS) -o track track.o -lzmq -lczmq

track.o: track.cc
	$(CXX) -I$(ZMQ_INCLUDES) -c track.cc

client: client.o
	$(CXX) -L$(ZMQ_LIBS) -o client client.o -lzmq -lczmq

client.o: client.cc
	$(CXX) -I$(ZMQ_INCLUDES) -c client.cc


clean:
	rm -f file file.o

