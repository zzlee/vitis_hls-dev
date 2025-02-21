XF_PROJ_ROOT ?= ~/Vitis_Libraries/vision/

IP ?= aximm_test0

.PHONY: all
all: gen

.PHONY: gen
gen:
	XF_PROJ_ROOT=${XF_PROJ_ROOT} vitis_hls -f ./${IP}/script.tcl
