
# Set CATAPULT_HOME to point to your catapult install Mgc_home dir
CATAPULT_HOME = /wv/hlsb/CATAPULT/10.3d/PRODUCTION/aol/Mgc_home

# allow for str.c_str() to pass as char*
CFLAGS += -g -std=c++11 

ba.exe: edge_defs.h EdgeDetect_Algorithm.h  EdgeDetect_BitAccurate.h EdgeDetect_BitAccurate_tb.cpp
	-$(MGC_HOME)/bin/g++ $(CFLAGS) -I $(MGC_HOME)/shared/include $(MGC_HOME)/shared/include/bmpUtil/bmp_io.cpp EdgeDetect_BitAccurate_tb.cpp -o $@
	ulimit -S -s 80000 && $@ image/people_gray.bmp orig1.bmp ba.bmp

clean:
	rm -f EdgeDetect_BitAccurate_tb.exe *.bmp

