module ram_1k_16_sp #(
  parameter data_width = 16,
  parameter addr_width = 10,
  parameter depth = 1024
)(
    addr, d, chip_en, wr_en, rd_en, clk, q
);

  input [addr_width-1:0] addr;
  input [data_width-1:0] d;
  input chip_en;
  input wr_en;
  input rd_en;
  input clk;
  output[data_width-1:0] q;

   // synopsys translate_off
  reg [data_width-1:0] q;
  reg [data_width-1:0] mem [depth-1:0];

  always @(posedge clk) begin
    if (chip_en) begin
      q <= mem[addr];
      if (wr_en) begin
        mem[addr] <= d;
      end
    end
  end

   // synopsys translate_on

endmodule
