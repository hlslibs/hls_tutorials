#------------------------------------------------------------
# Sliding Window Walkthrough - Circular buffers
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

solution file add {$MGC_HOME/shared/include/bmpUtil/bmp_io.cpp} -type C++ -exclude true
solution file add [file join $sfd EdgeDetect_CircularBuf_tb.cpp] -type C++
options set Output/OutputVHDL false
options set Flows/LowPower/SWITCHING_ACTIVITY_TYPE saif

solution options set /ComponentLibs/SearchPath $sfd -append

go analyze
directive set -DESIGN_HIERARCHY {{EdgeDetect_CircularBuf<1296, 864>} {EdgeDetect_CircularBuf<1296, 864>::verticalDerivative} {EdgeDetect_CircularBuf<1296, 864>::horizontalDerivative} {EdgeDetect_CircularBuf<1296, 864>::magnitudeAngle}}
go compile
solution library add nangate-45nm_beh -file {$MGC_HOME/pkgs/siflibs/nangate/nangate-45nm_beh.lib} -- -rtlsyntool OasysRTL
solution library add ram_1k_16_sp 

go libraries
directive set -CLOCKS {clk {-CLOCK_PERIOD 3.33 -CLOCK_EDGE rising -CLOCK_UNCERTAINTY 0.0 -CLOCK_HIGH_TIME 1.665 -RESET_SYNC_NAME rst -RESET_ASYNC_NAME arst_n -RESET_KIND sync -RESET_SYNC_ACTIVE high -RESET_ASYNC_ACTIVE low -ENABLE_ACTIVE high}}
go assembly
directive set /EdgeDetect_CircularBuf<1296,864>/verticalDerivative/core/line_buf0:rsc -MAP_TO_MODULE ram_1k_16_sp.ram_1k_16_sp
directive set /EdgeDetect_CircularBuf<1296,864>/verticalDerivative/core/line_buf1:rsc -MAP_TO_MODULE ram_1k_16_sp.ram_1k_16_sp
directive set /EdgeDetect_CircularBuf<1296,864>/verticalDerivative/core/VROW -PIPELINE_INIT_INTERVAL 1
directive set /EdgeDetect_CircularBuf<1296,864>/horizontalDerivative/core/HROW -PIPELINE_INIT_INTERVAL 1
directive set /EdgeDetect_CircularBuf<1296,864>/magnitudeAngle/core/MROW -PIPELINE_INIT_INTERVAL 1
directive set /EdgeDetect_CircularBuf<1296,864>/magnitudeAngle/core/ac_math::ac_atan2_cordic<9,9,AC_TRN,AC_WRAP,9,9,AC_TRN,AC_WRAP,8,3,AC_TRN,AC_WRAP>:for -UNROLL yes
directive set /EdgeDetect_CircularBuf<1296,864>/widthIn:rsc -MAP_TO_MODULE {[DirectInput]}
directive set /EdgeDetect_CircularBuf<1296,864>/heightIn:rsc -MAP_TO_MODULE {[DirectInput]}
go architect
go extract
