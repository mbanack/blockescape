SRC=src/GraphicalMain.cpp src/Board.cpp src/GenerateMain.cpp
OBJ=obj/Board.o obj/sio_packet.o obj/sio_socket.o obj/sio_client_impl.o obj/sio_client.o
LDFLAGS=-std=c++11 -Ilib/websocketpp -Ilib/rapidjson -Ilib/rapidjson/include -ggdb

graphical: obj/Board.o obj/GraphicalMain.o obj/sio_packet.o obj/sio_socket.o obj/sio_client_impl.o obj/sio_client.o
	g++ -o obj/$@ $(OBJ) obj/GraphicalMain.o $(LDFLAGS) -lSDL -lSDL_image -lboost_system -lboost_date_time -lboost_random -pthread

obj/Board.o: src/Board.cpp inc/Board.hpp src/sio_client.h
	g++ $(LDFLAGS) -c src/Board.cpp -o $@
	
obj/GenerateMain.o: src/GenerateMain.cpp 
	g++ $(LDFLAGS) -c src/GenerateMain.cpp -o $@

obj/GraphicalMain.o: src/GraphicalMain.cpp src/sio_client.h
	g++ $(LDFLAGS) -c src/GraphicalMain.cpp -o $@

obj/sio_packet.o: src/internal/sio_packet.cpp
	g++ $(LDFLAGS) -c src/internal/sio_packet.cpp -o $@

obj/sio_socket.o: src/sio_socket.cpp
	g++ $(LDFLAGS) -c src/sio_socket.cpp -o $@

obj/sio_client.o: src/sio_client.cpp
	g++ $(LDFLAGS) -c src/sio_client.cpp -o $@

obj/sio_client_impl.o: src/internal/sio_client_impl.cpp
	g++ $(LDFLAGS) -c src/internal/sio_client_impl.cpp -o $@

all: solver graphical generator
	
generate: obj/Board.o obj/GenerateMain.o obj/sio_packet.o obj/sio_socket.o obj/sio_client_impl.o obj/sio_client.o
	g++ -o obj/$@ $(OBJ) obj/GenerateMain.o $(LDFLAGS) -lSDL -lSDL_image -lboost_system -lboost_date_time -lboost_random -pthread

solver:
	g++ -ggdb -o obj/solver src/SolverMain.cpp src/Board.cpp -lSDL -lSDL_image -std=c++11

clean:
	rm -f $(OBJ)
