SRC=src/GraphicalMain.cpp src/Board.cpp src/GenerateMain.cpp src/SolverMain.cpp src/solver.c
OBJ=obj/Board.o obj/sio_packet.o obj/sio_socket.o obj/sio_client_impl.o obj/sio_client.o obj/Auth.o obj/solver.o
LDFLAGS=-std=c++11 -Ilib/libsodium/src/libsodium/include -Ilib/libsodium/builds/msvc -Ilib/websocketpp -Ilib/rapidjson -Ilib/rapidjson/include -ggdb -lsodium -lmysqlcppconn -I/usr/local/include/websocketpp/ -I/opt/local/include -D_GNU_SOURCE=1 -D_THREAD_SAFE -L/opt/local/lib 
graphical: obj/Board.o obj/GraphicalMain.o obj/sio_packet.o obj/sio_socket.o obj/sio_client_impl.o obj/sio_client.o obj/Auth.o obj/solver.o
	g++ -o obj/$@ $(OBJ) obj/GraphicalMain.o $(LDFLAGS) -L/opt/local/lib -lboost_system -lboost_date_time -lboost_random -lboost_timer -pthread

dbtest:
	g++ dbtest.cpp $(LDFLAGS)

obj/solver.o: src/solver.c src/solver.h
	g++ $(LDFLAGS) -c src/solver.c -o $@

obj/Auth.o: src/Auth.cpp inc/Auth.hpp
	g++ $(LDFLAGS) -c src/Auth.cpp -o $@

obj/Board.o: src/Board.cpp inc/Board.hpp src/sio_client.h
	g++ $(LDFLAGS) -c src/Board.cpp -o $@
	
obj/SolverMain.o: src/SolverMain.cpp 
	g++ $(LDFLAGS) -c src/SolverMain.cpp -o $@

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

all: graphical generator
	
generate: obj/Board.o obj/GenerateMain.o obj/sio_packet.o obj/sio_socket.o obj/sio_client_impl.o obj/sio_client.o obj/Auth.o obj/solver.o
	g++ -o obj/$@ $(OBJ) obj/GenerateMain.o $(LDFLAGS) -lboost_system -lboost_date_time -lboost_random -pthread

clean:
	rm -f $(OBJ)
