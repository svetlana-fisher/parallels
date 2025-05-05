HEIGHT = 20
WIDTH = 20

default: show_image

.PHONY:
show_image: result.dat
	gnuplot -e "plot 'result.dat' binary \
		array=($(WIDTH), $(HEIGHT)) format='%lf' with image; pause -1"


