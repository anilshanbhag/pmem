SRC = src
BIN = bin
OBJ = obj
INC = includes

CFLAGS = -O3 -march=native -std=c++14 -ffast-math
#CFLAGS = -g -std=c++14 -march=native
LDFLAGS = -Wl,-rpath=/root/pmdk/src/debug -ltbb -lpmem
CINCLUDES = -I$(INC)
CXX = clang++

PCMSO=/root/nvm_perf/includes/pcm/pcm.so/

all: $(BIN)/bandwidth $(BIN)/select $(BIN)/join ssb

$(OBJ)/perf_counters.o: $(SRC)/perf_counters.cpp
	g++ -std=c++11 -c $< -I$(INC) -o $@

$(BIN)/bandwidth-loop: $(SRC)/bandwidth-loop.cpp
	g++ -O3 -march=native -std=c++14 -ffast-math $< -ltbb -o $@

$(BIN)/bandwidth: $(SRC)/bandwidth.cpp
	g++ -O3 -march=native -std=c++14 -ffast-math $< -Wl,-rpath=/root/pmdk/src/debug -ltbb -lpmem -o $@

$(BIN)/bandwidth-perf:  $(SRC)/bandwidth-perf.cpp $(OBJ)/perf_counters.o
	g++ -O3 -I$(INC) -march=native -std=c++14 -ffast-math $^ -ltbb -lpcm -L$(PCMSO) -o $@

$(BIN)/bandwidth-nvm-perf: $(SRC)/bandwidth-nvm-perf.cpp $(OBJ)/perf_counters.o
	g++ -O3 -I$(INC) -march=native -std=c++14 -ffast-math $^ -Wl,-rpath=/root/pmdk/src/debug -ltbb -lpmem -lpcm -L$(PCMSO) -o $@

$(BIN)/bandwidth-nvm: $(SRC)/bandwidth-nvm.cpp
	g++ -O3 -march=native -std=c++14 -ffast-math $< -Wl,-rpath=/root/pmdk/src/debug -ltbb -lpmem -o $@

$(BIN)/bandwidth-loop-nvm: $(SRC)/bandwidth-loop-nvm.cpp
	g++ -O3 -march=native -std=c++14 -ffast-math $< -Wl,-rpath=/root/pmdk/src/debug -ltbb -lpmem -o $@

$(BIN)/join: $(SRC)/join.cpp
	g++ -O3 -march=native -std=c++14 -ffast-math $< -Wl,-rpath=/root/pmdk/src/debug -ltbb -lpmem -o $@

$(BIN)/join-perf: $(SRC)/join-perf.cpp $(OBJ)/perf_counters.o
	g++ -O3 -I$(INC) -march=native -std=c++14 -ffast-math $^ -Wl,-rpath=/root/pmdk/src/debug -ltbb -lpmem -lpcm -L$(PCMSO) -o $@

$(OBJ)/%.o: $(SRC)/%.cpp
	$(CXX) $(CFLAGS) $(CINCLUDES) -c $< -o $@

$(BIN)/%: $(OBJ)/%.o
	$(CXX) $(LDFLAGS) $^ -o $@

ssb_q11: bin/ssb/q11
ssb_q12: bin/ssb/q12
ssb_q13: bin/ssb/q13
ssb_q21: bin/ssb/q21
ssb_q22: bin/ssb/q22
ssb_q23: bin/ssb/q23
ssb_q31: bin/ssb/q31
ssb_q32: bin/ssb/q32
ssb_q33: bin/ssb/q33
ssb_q34: bin/ssb/q34
ssb_q41: bin/ssb/q41
ssb_q42: bin/ssb/q42
ssb_q43: bin/ssb/q43

ssb:ssb_q11 ssb_q12  ssb_q13 ssb_q21 ssb_q22 ssb_q23 ssb_q31 ssb_q43 ssb_q32 ssb_q33 ssb_q34 ssb_q41 ssb_q42 ssb_q43

