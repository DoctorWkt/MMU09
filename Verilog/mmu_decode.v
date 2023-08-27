// MMU and address decoding for the MMU09 SBC
// (c) 2022 Warren Toomey, GPL3

module mmu_decode(i_qclk, i_eclk, i_rw, i_addr, i_data, i_kmodeset,
		  i_uartirq, i_chirq,
		  romcs_n, ramcs_n, uartcs_n, chrd_n, chwr_n,
		  rtccs_n, paddr, pgfault_n, irq_n
`ifdef TESTING
		  , ffxx, kernio, kupper, kernel
`endif
		  );

				// _n suffix lines are active low
  input i_qclk;			// 6809 Q clock signal
  input i_eclk;			// 6809 E clock signal
  input i_rw;			// 6809 R/W signal
  input [15:0] i_addr;		// Input virtual address
  input [7:0] i_data;		// Data bus
  input i_kmodeset;		// Set kernel mode signal
  input i_uartirq;		// Interrupt from the UART
  input i_chirq;		// Interrupt from the CH375
  output romcs_n;		// ROM chip select
  output ramcs_n;		// RAM chip select
  output uartcs_n;		// UART chip select
  output chrd_n;		// CH375 read enable
  output chwr_n;		// CH375 write enable
  output rtccs_n;		// Real-time clock chip select
  output [18:13] paddr;		// Frame number for the input virtual address
  output pgfault_n;		// Active if page table entry is invalid
  output irq_n;			// Interrupt from either UART or CH375

`ifdef TESTING
  output ffxx;			// True for $FFxx addresses
  output kernio;		// True in kernel mode for $FExx addresses
  output kupper;		// True in kernel mode for top 32K addresses
  output kernel;		// Kernel mode
`else
  wire ffxx;
  wire kernio;
  wire kupper;
`endif

  ////////////////////////////
  // INTERRUPT MULTIPLEXING //
  ////////////////////////////

  // Send an IRQ when either the UART or the CH375 sends an interrupt
  assign irq_n= i_uartirq & i_chirq;

  //////////////////////
  // ADDRESS DECODING //
  //////////////////////

  // Kernel mode bit register: high when in kernel mode
  reg kmodeflag;
  initial kmodeflag <= 1'b1;

  // Update the user/kernel mode on either of the two signals.
  // i_kmodeset is connected to the 6809 /BS signal, which
  // goes high when an interrupt is taken.
  // Change to user mode when there is a kernel memory
  // access in the range $FEF0 to $FEFF.
  always @(posedge i_eclk) begin
    if (i_kmodeset)
      kmodeflag <= 1;
    if (!i_kmodeset && kernio && i_addr[7:5] == 3'b111)
      kmodeflag <= 0;
  end
`ifdef TESTING
  assign kernel= kmodeflag;
`endif

  // ffxx: active high for $FFxx addresses
  assign ffxx= i_addr[15] & i_addr[14] & i_addr[13] & i_addr[12] &
	       i_addr[11] & i_addr[10] & i_addr[9]  & i_addr[8];

  // kupper: active high for high 32K kernel addresses
  assign kupper= i_addr[15] & kmodeflag;

  // kernio: $FExx in kernel mode
  assign kernio= i_addr[14] & i_addr[13] & i_addr[12] & i_addr[11] &
		 i_addr[10] & i_addr[9]	 & !i_addr[8]  & kupper;

  // romcs_n: Active low on $FFxx address, or the top 32K in
  // kernel mode but not $FExx in kernel mode
  assign romcs_n= !(ffxx | (kupper & !kernio));

  // ramcs_n: Active low when neither the ROM or I/O are active
  assign ramcs_n= ffxx | kupper;

  // I/O is mapped into the address range $FE00 to $FEFF only in kernel mode.
  // Calculate the 3:8 decode of the low address bits
  // for the various active low chip select lines
  // We can only lower the CH375 lines when i_eclk is high
  assign uartcs_n=         (kernio && i_addr[7:5] == 3'b000) ? 1'b0 : 1'b1;
  assign rtccs_n=          (kernio && i_addr[7:5] == 3'b001) ? 1'b0 : 1'b1;
  assign chrd_n= (i_eclk && kernio && i_addr[7:5] == 3'b010) ? 1'b0 : 1'b1;
  assign chwr_n= (i_eclk && kernio && i_addr[7:5] == 3'b011) ? 1'b0 : 1'b1;


  ///////////////////////
  // MEMORY MANAGEMENT //
  ///////////////////////

  reg [7:0] pgtable[0:7];	// The page table. Low six bits are
				// the frame number. Top bit indicates
				// that the page is invalid

  // Get the address lines that index into the page table
  wire [2:0] pgindex= i_addr[15:13];

  // When we are writing to the page table entries,
  // we use the low address bits so we get indices 0 .. 7
  wire [2:0] writeindex= i_addr[2:0];

  // Page table entry found by indexing the page table
  wire [7:0] pte;

  // Page mapping. Six bits become the frame number.
  assign pte= pgtable[pgindex];
  assign paddr= pte[5:0];

  // Page table entry updates on kernel writes to $FECx
  always @(posedge i_eclk)
    if (kernio && !i_rw && i_addr[7:5] == 3'b110)
      pgtable[writeindex] <= i_data;

  // A small state machine for page fault handling.
  // Three states: no page fault (11), a page fault
  // is signalled before we get to kernel mode (10), 
  // the page fault signal is disabled before we
  // get to kernel mode (01).
  reg [1:0] faultstate;
  initial faultstate <= 2'b11;
  assign pgfault_n= faultstate[0];

  always @(posedge i_eclk) begin
	  				  // Invalid page in user mode:
    if (faultstate == 2'b11 && !pte[7] && !kmodeflag)
      faultstate <= 2'b10;		  // signal a page fault.
    if (faultstate == 2'b10)		  // Turn it off a clock cycle later
      faultstate <= 2'b01;
    if (faultstate == 2'b01 && kmodeflag) // And set to no fault once we
      faultstate <= 2'b11;		  // reach kernel mode
  end

endmodule
