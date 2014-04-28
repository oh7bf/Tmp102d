# -*- mode: makefile -*-
# 
# Makefile for compiling 'tmp102d' on Raspberry Pi. 
#
# Tue Feb 25 21:25:42 CET 2014
# Edit: 
# Jaakko Koivuniemi

CXX           = gcc
CXXFLAGS      = -g -O -Wall 
LD            = gcc
LDFLAGS       = -O

%.o : %.c
	$(CXX) $(CXXFLAGS) -c $<

all: tmp102d 

tmp102d: tmp102d.o
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -f *.o

