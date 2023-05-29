#!/bin/sh
ps -f | grep vicit-diem | awk '{print "kill -9 " $2}' | sh 
clang++ -std=c++11 `wx-config --cxxflags` main.cpp -o vicit-diem `wx-config --libs`
./vicit-diem &
