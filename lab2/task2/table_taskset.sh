#!/bin/bash



# Описать вычислительный узел: наименование и краткая характеристика
# CPU (lscpu), наименование сервера (cat /sys/devices/virtual/dmi/id/product_name),
# сколько NUMA node (numactl --hardware), сколько памяти у каждой ноды,
# операционная система (cat /etc/os-release).


make

N=10

T1=0
t2=0
t4=0
t7=0
t8=0
t16=0
t20=0
t40=0


for i in $(seq 1 ${N}); do
    t1=$(taskset -c 1 ./task2)
    T1=$(echo "${T1} + ${t1}" | bc)
    t2=$(echo "${t2} + $(taskset -c 0-1 ./task2)" | bc)
    t4=$(echo "${t4} + $(taskset -c 0-3 ./task2)" | bc)
    t7=$(echo "${t7} + $(taskset -c 0-6 ./task2)" | bc)
    t8=$(echo "${t8} + $(taskset -c 0-7 ./task2)" | bc)
    t16=$(echo "${t16} + $(taskset -c 0-15 ./task2)" | bc)
    t20=$(echo "${t20} + $(taskset -c 0-19 ./task2)" | bc)
    t40=$(echo "${t40} + $(taskset -c 0-39 ./task2)" | bc)
done


avg_T1=$(echo "scale=5; ${T1} / ${N}" | bc)
avg_t2=$(echo "scale=5; ${t2} / ${N}" | bc)
avg_S2=$(echo "scale=5; ${T1} / ${t2}" | bc)
avg_t4=$(echo "scale=5; ${t4} / ${N}" | bc)
avg_S4=$(echo "scale=5; ${T1} / ${t4}" | bc)
avg_t7=$(echo "scale=5; ${t7} / ${N}" | bc)
avg_S7=$(echo "scale=5; ${T1} / ${t7}" | bc)
avg_t8=$(echo "scale=5; ${t8} / ${N}" | bc)
avg_S8=$(echo "scale=5; ${T1} / ${t8}" | bc)
avg_t16=$(echo "scale=5; ${t16} / ${N}" | bc)
avg_S16=$(echo "scale=5; ${T1} / ${t16}" | bc)
avg_t20=$(echo "scale=5; ${t20} / ${N}" | bc)
avg_S20=$(echo "scale=5; ${T1} / ${t20}" | bc)
avg_t40=$(echo "scale=5; ${t40} / ${N}" | bc)
avg_S40=$(echo "scale=5; ${T1} / ${t40}" | bc)


echo "T1=${avg_T1} T2=${avg_t2} S2=${avg_S2}"
echo "T4=${avg_t4} S4=${avg_S4}"
echo "T7=${avg_t7} S6=${avg_S7}"
echo "T8=${avg_t8} S8=${avg_S8}"
echo "T16=${avg_t16} S10=${avg_S16}"
echo "T20=${avg_t20} S10=${avg_S20}"
echo "T40=${avg_t40} S10=${avg_S40}"