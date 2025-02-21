set XF_PROJ_ROOT $env(XF_PROJ_ROOT)

open_project zzlab_lbl_wr
set_top lbl_wr
add_files zzlab_lbl_wr/kernel.cpp -cflags "-I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x" -csimflags "-I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x"
add_files -tb zzlab_lbl_wr/tb.cpp -cflags "-I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x" -csimflags "-I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x"

# 2019.2
# open_solution "solution1"
# set_part {xczu7ev-ffvc1156-2-e} -tool vivado

# 2020.2
open_solution "solution1" -flow_target vivado
set_part {xczu4ev-fbvb900-2-e}

create_clock -period 3.33333 -name default
csynth_design
export_design -format ip_catalog -description "ZzLab LBL Writer" -vendor "YUAN" -display_name "ZzLab LBL Writer"
exit
