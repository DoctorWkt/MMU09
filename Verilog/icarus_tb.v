// Testbed for the MMU09 SBC
// (c) 2022 Warren Toomey, GPL3

`timescale 1ns/1ps
`default_nettype none
`include "mmu09_sbc.v"

module icarus_tb();

  parameter HalfClock = 34;		// Half a clock cycle in nS

  reg clk= 0;				// Four-speed clock
  reg qclk= 0;				// 6809 Q clock
  reg eclk= 0;				// 6809 E clock
  reg reset_n= 0;			// Reset line, active low
  reg [3:0] counter=0;			// Used to raise reset_n eventually
  wire [15:0] vadr;			// 6809 address bus

  // Initialize all variables
  initial begin        
    $dumpfile("test.vcd");
    $dumpvars(0, icarus_tb);
    #200000 $finish;      		// Terminate simulation
  end

  // Clock generator
  always begin
    #HalfClock clk = ~clk; // Toggle clk every HalfClock to make a full cycle
  end
  
  // Generate the Q and E clock signals
  // and raise reset_n after a while
  always @(posedge clk) begin
    if (counter == 4'hF)
      reset_n= 1;
    if (counter[1:0] == 2'b00)
      qclk <= 1;
    if (counter[1:0] == 2'b01)
      eclk <= 1;
    if (counter[1:0] == 2'b10)
      qclk <= 0;
    if (counter[1:0] == 2'b11)
      eclk <= 0;
    counter <= counter + 1;
  end

  // Stop if we access address $FFF0
  always @(posedge clk) begin
    if (vadr == 16'hFFF0) begin
      $finish;
    end
  end

  // Connect DUT to test bench
  mmu09_sbc dut(qclk, eclk, reset_n, vadr);

endmodule
