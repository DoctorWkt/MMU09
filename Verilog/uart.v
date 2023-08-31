// Simulation of UART
// (c) 2018 Warren Toomey, GPL3

module uart (
        input [7:0] data,	// Input data
	input CS_n,		// Chip select, active low
	input TX_n		// Transmit control line, active low
  );

  // UART output: write when chip is selected on a falling TX edge
  always @(negedge CS_n)
    if (!TX_n)
      $write("%c", data);

endmodule
