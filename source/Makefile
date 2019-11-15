CCACHE_EXISTS := $(shell ccache -V)
ifdef CCACHE_EXISTS
    CXX := ccache g++-8
endif

DEBUG ?= 1
#INCLUDE = 
CXXFLAGS_BASE = -c -g -pthread -fPIC -std=c++17 -Wall -Wno-unknown-pragmas -Wno-switch $(INCLUDE)

BIN_DIR_BASE:=../../bin/
OUT_DIR_BASE := .obj/
LIBS_BASE = -lMy -ldl -lboost_system -lprotobuf -lstdc++fs 
ifeq ($(DEBUG), 1)
	OUT_DIR=$(OUT_DIR_BASE)debug
	CXXFLAGS= $(CXXFLAGS_BASE) -O0 -I$(OUT_DIR) -fsanitize=address -fno-omit-frame-pointer
	BIN_DIR=$(BIN_DIR_BASE)debug
	LIBS = -lasan $(LIBS_BASE)
else
	OUT_DIR=$(OUT_DIR_BASE)release
	CXXFLAGS= $(CXXFLAGS_BASE) -DNDEBUG -O3 -I$(OUT_DIR)
	BIN_DIR=$(BIN_DIR_BASE)release
	LIBS = $(LIBS_BASE)
endif

OUTPUT = $(BIN_DIR)/AppServer

LIBFLAGS = -pthread -L$(BIN_DIR)  -Wl,-z,origin -Wl,-rpath='$$ORIGIN' 

all: $(OUTPUT)
	$(NOECHO) $(NOOP)

OBJECTS = $(OUT_DIR)/main.o $(OUT_DIR)/Listener.o $(OUT_DIR)/Sender.o $(OUT_DIR)/Cache.o $(OUT_DIR)/LogData.o $(OUT_DIR)/LogClient.o\
			$(OUT_DIR)/WebServer.o $(OUT_DIR)/WebSession.o $(OUT_DIR)/WebSocket.o\
			$(OUT_DIR)/FromClient.pb.o $(OUT_DIR)/FromServer.pb.o

$(OUTPUT): $(OUT_DIR)/stdafx.h.gch $(OBJECTS)
	$(CXX) $(LIBFLAGS) -L/home/duffyj/code/libraries/boostorg/boost_1_68_0/stage/lib $(OBJECTS) -o$(OUTPUT) $(LIBS)

$(OUT_DIR)/stdafx.h.gch: pc.h
	$(CXX) $(CXXFLAGS) ./pc.h -o $(OUT_DIR)/stdafx.h.gch -I/home/duffyj/code/libraries/spdlog/include -I/home/duffyj/code/libraries/boostorg/boost_1_68_0 -I/home/duffyj/code/libraries/json/include

$(OUT_DIR)/main.o: ./main.cpp pc.h
	$(CXX) $(CXXFLAGS) $< -o$@  $(INCLUDE)

$(OUT_DIR)/WebSocket.o: ./WebSocket.cpp WebSocket.h pc.h
	$(CXX) $(CXXFLAGS) ./$< -o$@ $(INCLUDE)

$(OUT_DIR)/%.o: ./%.cpp %.h pc.h
	$(CXX) $(CXXFLAGS) ./$< -o$@ $(INCLUDE)

$(OUT_DIR)/%.o: ./types/proto/%.cc pc.h
	$(CXX) $(CXXFLAGS) ./$< -o$@ $(INCLUDE)

clean:
	rm -rf -d $(OUT_DIR)/*.*
	rm -f $(OUTPUT)
