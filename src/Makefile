CXX = g++
# OPT = -O3
OPT = -g # debug mode
#WARN = -Wall
ERR = -Werror
#DEBUG = -D_DEBUG
# DEBUG = 

CXXFLAGS = $(OPT) $(WARN) $(ERR) $(INC) $(LIB) $(DEBUG) -std=c++11

# check https://makefiletutorial.com/#fancy-rules for why it works 

SRC = $(wildcard *.cc)
OBJ = $(subst .cc,.o,$(SRC))

# OBJ = main.o cache.o

all: smp_cache
	@echo "Compilation Done ---> nothing else to make :) "

smp_cache: $(OBJ)
	$(CXX) -o smp_cache $(CXXFLAGS) $(OBJ) -lm
	@echo "------------------------------------------------------------"
	@echo "--- ECE/CSC 406/506 FALL'22 COHERENCE PROTOCOL SIMULATOR ---"
	@echo "------------------------------------------------------------"

clean:
	rm -f *.o smp_cache

PROTOCOL = 3
#TRACE_FILE = ../trace/canneal.04t.debug
TRACE_FILE = ../trace/canneal.04t.longTrace
#VALIDATION_FILE = ../val/msi_long.val
VALIDATION_FILE = ../val/mesi_filter_long.val2

run: all
	@echo "*** Running ./smp_cache 8192 8 64 4 $(PROTOCOL) $(TRACE_FILE) ***"
	./smp_cache 8192 8 64 4 $(PROTOCOL) $(TRACE_FILE)

val: all
	@echo "*** Comparing output with $(VALIDATION_FILE) ***"
	./smp_cache 8192 8 64 4 $(PROTOCOL) $(TRACE_FILE) | diff -iwy - $(VALIDATION_FILE)

pack:
	zip -j project2.zip *.cc *.h *.pdf