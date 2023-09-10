// Simulation of UART
// (c) 2018 Warren Toomey, GPL3

module uart (
        input [7:0] data,	// Input data
	input RD_n,		// Read enable, active low
	input WR_n		// Write enable, active low
  );

  // UART output: write when on a WE_n falling edge
  always @(negedge WR_n)
    $write("%c", data);

endmodule
