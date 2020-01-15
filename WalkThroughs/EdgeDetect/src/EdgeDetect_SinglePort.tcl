#------------------------------------------------------------
# Sliding Window Walkthrough - SinglePort 
#------------------------------------------------------------

# Establish the location of this script and use it to reference all
# other files in this example
set sfd [file dirname [info script]]

# Reset the options to the factory defaults
options defaults
options set /Input/CppStandard c++11

project new

flow package require /SCVerify
flow package option set /SCVerify/USE_CCS_BLOCK true
flow package option set /SCVerify/INVOKE_ARGS "[file join $sfd image people_gray.bmp] out_algorithm.bmp out_hw.bmp"

solution file add [file join $sfd EdgeDetect_SinglePort_tb.cpp] -type C++
solution file add {$MGC_HOME/shared/include/bmpUtil/bmp_io.cpp} -type C++ -exclude true

go analyze
directive set -DESIGN_HIERARCHY {{EdgeDetect_SinglePort} {EdgeDetect_SinglePort::verticalDerivative} {EdgeDetect_SinglePort::horizontalDerivative} {EdgeDetect_SinglePort::magnitudeAngle}}
go compile

solution library add nangate-45nm_beh -file {$MGC_HOME/pkgs/siflibs/nangate/nangate-45nm_beh.lib} -- -rtlsyntool OasysRTL
solution library add ccs_sample_mem -file {$MGC_HOME/pkgs/siflibs/ccs_sample_mem.lib}

go libraries
directive set -CLOCKS {clk {-CLOCK_PERIOD 3.33 -CLOCK_EDGE rising -CLOCK_UNCERTAINTY 0.0 -CLOCK_HIGH_TIME 1.665 -RESET_SYNC_NAME rst -RESET_ASYNC_NAME arst_n -RESET_KIND sync -RESET_SYNC_ACTIVE high -RESET_ASYNC_ACTIVE low -ENABLE_ACTIVE high}}
go assembly

directive set /EdgeDetect_SinglePort/verticalDerivative/core/VROW -PIPELINE_INIT_INTERVAL 1
directive set /EdgeDetect_SinglePort/verticalDerivative/core/VCOL -PIPELINE_INIT_INTERVAL 1
directive set /EdgeDetect_SinglePort/horizontalDerivative/core/HROW -PIPELINE_INIT_INTERVAL 1
directive set /EdgeDetect_SinglePort/horizontalDerivative/core/HCOL -PIPELINE_INIT_INTERVAL 1
directive set /EdgeDetect_SinglePort/magnitudeAngle/core/MROW -PIPELINE_INIT_INTERVAL 1
directive set /EdgeDetect_SinglePort/magnitudeAngle/core/ac_math::ac_atan2_cordic<9,9,AC_TRN,AC_WRAP,9,9,AC_TRN,AC_WRAP,8,3,AC_TRN,AC_WRAP>:for -UNROLL yes
directive set /EdgeDetect_SinglePort/verticalDerivative/core/line_buf0:rsc -MAP_TO_MODULE ccs_sample_mem.ccs_ram_sync_singleport
directive set /EdgeDetect_SinglePort/verticalDerivative/core/line_buf1:rsc -MAP_TO_MODULE ccs_sample_mem.ccs_ram_sync_singleport

go architect
go allocate
go extract

