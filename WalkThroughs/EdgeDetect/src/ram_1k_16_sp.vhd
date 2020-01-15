-- Block Single Port RAM Read Before Write

LIBRARY IEEE;
   USE IEEE.STD_LOGIC_1164.ALL;
   USE IEEE.Numeric_Std.ALL;

PACKAGE ram_1k_16_sp_pkg IS
   COMPONENT ram_1k_16_sp
      GENERIC (
         data_width    : integer := 16;
         addr_width : integer := 10;
         depth         : integer := 1024
      );
      PORT (
         clk  : IN  std_logic;
         addr  : IN  std_logic_vector(addr_width-1 DOWNTO 0);
         wr_en   : IN  std_logic;
         rd_en   : IN  std_logic;
         chip_en   : IN  std_logic;
         d    : IN  std_logic_vector(data_width-1 DOWNTO 0);
         q    : OUT std_logic_vector(data_width-1  DOWNTO 0)
      );
   END COMPONENT;
END ram_1k_16_sp_pkg;

LIBRARY IEEE;
   USE IEEE.STD_LOGIC_1164.ALL;
   USE IEEE.Numeric_Std.ALL;
--LIBRARY nangate-45nm_mem;
   USE work.ram_1k_16_sp_pkg.ALL;

ENTITY ram_1k_16_sp IS
      GENERIC (
         data_width    : integer := 16;
         addr_width : integer := 10;
         depth         : integer := 1024
      );
      PORT (
         clk  : IN  std_logic;
         addr  : IN  std_logic_vector(addr_width-1 DOWNTO 0);
         wr_en   : IN  std_logic;
         rd_en   : IN  std_logic;
         chip_en   : IN  std_logic;
         d    : IN  std_logic_vector(data_width-1 DOWNTO 0);
         q    : OUT std_logic_vector(data_width-1  DOWNTO 0)
      );

END ram_1k_16_sp;

ARCHITECTURE rtl OF ram_1k_16_sp IS

   TYPE ram_t IS ARRAY (depth-1 DOWNTO 0) OF std_logic_vector(data_width-1 DOWNTO 0);
   SIGNAL mem : ram_t := (OTHERS => (OTHERS => '0'));

BEGIN
   PROCESS (clk)
   BEGIN
      --synopsys translate_off
      IF (rising_edge(clk)) THEN
         IF (chip_en ='1') THEN
            IF (rd_en='1') THEN
              q <= mem(to_integer(unsigned(addr))) ; -- read port
            END IF;
            IF (wr_en='1') THEN
               mem(to_integer(unsigned(addr))) <= d;
            END IF;
         END IF; 
      END IF;
      --synopsys translate_on
   END PROCESS;
END rtl;


