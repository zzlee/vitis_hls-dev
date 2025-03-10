set XF_PROJ_ROOT $env(XF_PROJ_ROOT)

open_project aximm_test1
set_top aximm_test1
add_files aximm_test1/kernel.cpp -cflags "-I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x" -csimflags "-I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x"
add_files -tb aximm_test1/tb.cpp -cflags "-I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x" -csimflags "-I${XF_PROJ_ROOT}/L1/include -I ./ -D__SDSVHLS__ -std=c++0x"

# 2019.2
# open_solution "solution1"
# set_part {xczu7ev-ffvc1156-2-e} -tool vivado

# 2020.2
open_solution "solution1" -flow_target vivado
set_part {xczu4ev-fbvb900-2-e}

create_clock -period 3.33333 -name default
csynth_design
export_design -format ip_catalog -description "AXIMM Test1" -vendor "YUAN" -display_name "AXIMM Test1"
exit
