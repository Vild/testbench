#!/bin/bash

dataMS=""
dataFPS=""
dataDiff="'< paste $(find . -maxdepth 1 -name "*.log" -and \! -name "*-tmp.log" -exec echo -n "\"{}\" " \;)' using ((100*(1000/\$2)/(1000/\$1))) with lines"

for f in $(find . -maxdepth 1 -name "*.log" -and \! -name "*-tmp.log" | sed "s/\.\///"); do
		tail -n18000 $f > $f-tmp.log

		short="$(echo $f | head -c2)"
		dataMS="$dataMS, '${f}-tmp.log' with lines title '$short'"
		dataFPS="$dataFPS, '${f}-tmp.log' using (1000/\$1) with lines title '$short'"
done


gnuplot <<EOF
set term png size 1024,512 font 'DejaVu Sans Mono Book,14'
set output 'plotMS.png'

set style line 1 lt 2 lw 2 pt 3 ps 0.5
set yrange [4:9]

set title "Milliseconds per frame (Lower is better)"

plot $(echo $dataMS | sed "s/^,//")
EOF

gnuplot <<EOF
set term png size 1024,512 font 'DejaVu Sans Mono Book,14'
set output 'plotFPS.png'

set style line 1 lt 2 lw 2 pt 3 ps 0.5
set yrange [120:225]
set title "Frames per second (Higher is better)"

plot $(echo $dataFPS | sed "s/^,//")
EOF
gnuplot <<EOF
set term png size 1024,512 font 'DejaVu Sans Mono Book,14'
set output 'plotDiff.png'

set style line 1 lt 2 lw 2 pt 3 ps 0.5
set yrange [60:140]
set ytics 5
set title "Procentual performance gain from Vulkan (100% means same as OpenGL)"

plot 100 with lines title "OpenGL Performance", $(echo $dataDiff | sed "s/^,//") title "Vulkan Performance"
EOF
