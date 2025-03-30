make

chmod 700 ./task1


N=10 #колличество прогонов тестирований
m=20000 #рзмер матрицы

T1=0
t2=0
t4=0
t6=0
t8=0
t16=0
t20=0
t40=0


for i in $(seq 1 ${N}); do
    t1=$(taskset -c 1 ./task1 ${m}) #только эта маленькая
    T1=$(echo "${T1} + ${t1}" | bc)
    t2=$(echo "${t2} + $(taskset -c 1,2 ./task1 ${m})" | bc)
    t4=$(echo "${t4} + $(taskset -c 1,2,3,4 ./task1 ${m})" | bc)
    t6=$(echo "${t6} + $(taskset -c 1,2,3,4,5,6 ./task1 ${m})" | bc)
    t8=$(echo "${t8} + $(taskset -c 1,2,3,4,5,6,7,8 ./task1 ${m})" | bc)
    t16=$(echo "${t16} + $(taskset -c 1-16 ./task1 ${m})" | bc)
    t20=$(echo "${t20} + $(taskset -c 1-20 ./task1 ${m})" | bc)
    t40=$(echo "${t40} + $(taskset -c 1-40 ./task1 ${m})" | bc)
    #t10=$(echo "${t10} + $(taskset -c 1,2,3,4,5,6,7,8,9,10 ./laba0_task1 ${m})" | bc)
done


avg_T1=$(echo "scale=5; ${T1} / ${N}" | bc)
avg_t2=$(echo "scale=5; ${t2} / ${N}" | bc)
avg_S2=$(echo "scale=5; ${T1} / ${t2}" | bc)
avg_t4=$(echo "scale=5; ${t4} / ${N}" | bc)
avg_S4=$(echo "scale=5; ${T1} / ${t4}" | bc)
avg_t6=$(echo "scale=5; ${t6} / ${N}" | bc)
avg_S6=$(echo "scale=5; ${T1} / ${t6}" | bc)
avg_t8=$(echo "scale=5; ${t8} / ${N}" | bc)
avg_S8=$(echo "scale=5; ${T1} / ${t8}" | bc)
avg_t16=$(echo "scale=5; ${t16} / ${N}" | bc)
avg_S16=$(echo "scale=5; ${T1} / ${t16}" | bc)
avg_t20=$(echo "scale=5; ${t20} / ${N}" | bc)
avg_S20=$(echo "scale=5; ${T1} / ${t20}" | bc)
avg_t40=$(echo "scale=5; ${t40} / ${N}" | bc)
avg_S40=$(echo "scale=5; ${T1} / ${t40}" | bc)



echo "m=${m} T1=${avg_T1} T2=${avg_t2} S2=${avg_S2} T4=${avg_t4} S4=${avg_S4} T6=${avg_t6} S6=${avg_S6} T8=${avg_t8} S8=${avg_S8} T16=${avg_t16} S16=${avg_S16} T20=${avg_t20} S20=${avg_S20} T40=${avg_t40} S40=${avg_S40}"
