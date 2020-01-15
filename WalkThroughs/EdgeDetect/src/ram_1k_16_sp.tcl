flow package require MemGen
flow run /MemGen/MemoryGenerator_BuildLib {
VENDOR           *
RTLTOOL          OasysRTL
TECHNOLOGY       *
LIBRARY          nangate-45nm_mem
MODULE           ram_1k_16_sp
OUTPUT_DIR       ./
FILES {
  { FILENAME ./ram_1k_16_sp.v           FILETYPE Verilog PATHTYPE relative MODELTYPE GENERIC   PARSE 1 STATICFILE 1 VHDL_LIB_MAPS work }
  { FILENAME ./ram_1k_16_sp.vhd         FILETYPE VHDL    PATHTYPE relative MODELTYPE GENERIC                                           }
  { FILENAME ./ram_1k_16_sp_liberty.lib FILETYPE Liberty PATHTYPE relative MODELTYPE synthesis PARSE 1 STATICFILE 1 VHDL_LIB_MAPS work }
}
LIBS_LEF         ram_1k_16_sp.lef
VHDLARRAYPATH    mem
WRITEDELAY       0.1
INITDELAY        1
READDELAY        0.4
VERILOGARRAYPATH mem
INPUTDELAY       0.01
WIDTH            data_width
AREA             99353
RDWRRESOLUTION   UNKNOWN
WRITELATENCY     1
LIBS_LIBERTY     ram_1k_16_sp_liberty.lib
VHDL_LIB_MAPS    {work ./work nangate45_mem_lib ./work}
READLATENCY      1
DEPTH            depth
PARAMETERS {
  { PARAMETER data_width TYPE hdl IGNORE 0 MIN 16 MAX 16 DEFAULT 16 }
  { PARAMETER addr_width TYPE hdl IGNORE 0 MIN 10 MAX 10 DEFAULT 10 }
  { PARAMETER depth      TYPE hdl IGNORE 0 MIN {} MAX {} DEFAULT 0  }
}
PORTS {
  { NAME port_0 MODE ReadWrite }
}
PINMAPS {
  { PHYPIN addr    LOGPIN ADDRESS      DIRECTION in  WIDTH addr_width PHASE {} DEFAULT {} PORTS port_0 }
  { PHYPIN d       LOGPIN DATA_IN      DIRECTION in  WIDTH data_width PHASE {} DEFAULT {} PORTS port_0 }
  { PHYPIN chip_en LOGPIN ENABLE       DIRECTION in  WIDTH 1.0        PHASE 1  DEFAULT {} PORTS port_0 }
  { PHYPIN wr_en   LOGPIN WRITE_ENABLE DIRECTION in  WIDTH 1.0        PHASE 1  DEFAULT {} PORTS port_0 }
  { PHYPIN rd_en   LOGPIN READ_ENABLE  DIRECTION in  WIDTH 1.0        PHASE 1  DEFAULT {} PORTS port_0 }
  { PHYPIN clk     LOGPIN CLOCK        DIRECTION in  WIDTH 1.0        PHASE 1  DEFAULT {} PORTS port_0 }
  { PHYPIN q       LOGPIN DATA_OUT     DIRECTION out WIDTH data_width PHASE {} DEFAULT {} PORTS port_0 }
}

}
