#!/bin/bash

##echo "set term png" > plot.m
echo "" > plot.m
# echo "set output 'plot.png'" >> plot.m
echo "set term svg size 2048,1024 font 'DejaVu Sans Mono Book,14'" >> plot.m
echo "set output 'plot.svg'" >> plot.m

echo "plot $(find . -maxdepth 1 -name "*.log" -exec echo -n ", '{}' with linespoints" \; | sed "s/^,//")" >> plot.m

gnuplot < plot.m
