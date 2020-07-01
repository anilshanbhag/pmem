PCMSO=/big_fast_drive/anil/pcm_test/pcm/pcm.so/
PCMSO=/root/nvm_perf/pcm_utils/pcm/pcm.so/

perf_counters.o: perf_counters.cpp
	g++ -o perf_counters.o -std=c++11 -c perf_counters.cpp  -Ipcm/

pcm_test.o: pcm_test.cpp
	g++ -o pcm_test.o -std=c++11 -c pcm_test.cpp  -Ipcm/

pcm_test: perf_counters.o pcm_test.o
	g++ -o pcm_test pcm_test.o perf_counters.o -lpcm -L$(PCMSO)
