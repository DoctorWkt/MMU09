// MMU and address decoding for the MMU09 SBC
// (c) 2023 Warren Toomey, BSD license

module mmu_decode(i_qclk, i_eclk, i_reset, i_rw, i_addr, i_data, i_kmodeset,
		  i_uartirq, i_chirq, i_rtcirq,
		  romcs_n, ramcs_n, uartcs_n, chrd_n, chwr_n,
		  rtccs_n, paddr, pgfault_n, irq_n, firq_n, halt_n
`ifdef TESTING
		  , ffxx, kernio, kupper, kernel
`endif
		  );

				// _n suffix lines are active low
  input i_qclk;			// 6809 Q clock signal
  input i_eclk;			// 6809 E clock signal
  input i_reset;		// Reset line, active low
  input i_rw;			// 6809 R/W signal
  input [15:0] i_addr;		// Input virtual address
  input [7:0] i_data;		// Data bus
  input i_kmodeset;		// Set kernel mode signal aka the 6809 BS line
  input i_uartirq;		// Interrupt from the UART
  input i_chirq;		// Interrupt from the CH375
  input i_rtcirq;		// Interrupt from the real-time clock

  output romcs_n;		// ROM chip select
  output ramcs_n;		// RAM chip select
  output uartcs_n;		// UART chip select
  output chrd_n;		// CH375 read enable
  output chwr_n;		// CH375 write enable
  output rtccs_n;		// Real-time clock chip select
  output [18:13] paddr;		// Frame number for the input virtual address
  output pgfault_n;		// Active if page table entry is invalid
  output irq_n;			// Interrupt from either UART or CH375
  output firq_n;		// Fast interrupt to the 6809
  output halt_n;		// Output to the 6809 HALT line

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

  // Pass through the real-time clock interrupt to the FIRQ output
  assign firq_n= i_rtcirq;


  ////////////////
  // HALT LOGIC //
  ////////////////

  // For now, nothing. But we have the output line for later :-)
  assign halt_n= 1'b1;


  ////////////////////
  // RESET HANDLING //
  ////////////////////
  always @(negedge i_reset) begin
    kmodeflag <= 1'b1;
    faultstate <= 2'b11;
  end


  //////////////////////
  // ADDRESS DECODING //
  //////////////////////

  // Kernel mode bit register: high when in kernel mode
  reg kmodeflag;
  initial kmodeflag = 1'b1;

  // Update the user/kernel mode on either of the two signals.
  // i_kmodeset is connected to the 6809 /BS signal, which
  // goes high when an interrupt is taken.
  // Change to user mode when there is a kernel memory
  // access in the range $FEF0 to $FEFF.
  always @(posedge i_eclk) begin
    if (i_kmodeset)
      kmodeflag <= 1'b1;
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
  // for the various active low chip select lines.
  // We can only lower the I/O lines when i_eclk is high
  // because the data bus isn't valid until then
  assign rtccs_n=  (i_eclk && kernio && i_addr[7:5] == 3'b001) ? 1'b0 : 1'b1;
  assign uartcs_n= (i_eclk && kernio && i_addr[7:5] == 3'b000) ? 1'b0 : 1'b1;
  assign chrd_n=   (i_eclk && kernio && i_addr[7:5] == 3'b010) ? 1'b0 : 1'b1;
  assign chwr_n=   (i_eclk && kernio && i_addr[7:5] == 3'b011) ? 1'b0 : 1'b1;


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


  /////////////////////////
  // PAGE FAULT HANDLING //
  /////////////////////////

  // A small state machine for page fault handling, so
  // that we drop the NMI line and raise it soon after.

  // Three states: no page fault (11), a page fault
  // is signalled before we get to kernel mode (10), 
  // the page fault signal is disabled before we
  // get to kernel mode (01).

  reg [1:0] faultstate;
  initial faultstate = 2'b11;
  assign pgfault_n= faultstate[0];	  // Output to the NMI line

  always @(posedge i_eclk) begin
	  				  // Invalid page in user mode:
					  // signal a page fault.
    if (faultstate == 2'b11 && !pte[7] && !kmodeflag)
      faultstate <= 2'b10;
    if (faultstate == 2'b10)		  // Turn it off a clock cycle later
      faultstate <= 2'b01;
    if (faultstate == 2'b01 && kmodeflag) // And set to no fault once we
      faultstate <= 2'b11;		  // reach kernel mode
  end

endmodule

//PIN: CHIP "mmu_decode" ASSIGNED TO AN PLCC84
//PIN: chrd_n : 55
//PIN: chwr_n : 54
//PIN: firq_n : 28
//PIN: halt_n : 68
//PIN: i_addr_0 : 25
//PIN: i_addr_10 : 10
//PIN: i_addr_11 : 9
//PIN: i_addr_12 : 8
//PIN: i_addr_1 : 24
//PIN: i_addr_13 : 6
//PIN: i_addr_14 : 5
//PIN: i_addr_15 : 4
//PIN: i_addr_2 : 22
//PIN: i_addr_3 : 21
//PIN: i_addr_4 : 20
//PIN: i_addr_5 : 18
//PIN: i_addr_6 : 17
//PIN: i_addr_7 : 16
//PIN: i_addr_8 : 15
//PIN: i_addr_9 : 11
//PIN: i_chirq : 50
//PIN: i_data_0 : 70
//PIN: i_data_1 : 73
//PIN: i_data_2 : 74
//PIN: i_data_3 : 75
//PIN: i_data_4 : 76
//PIN: i_data_5 : 77
//PIN: i_eclk : 83
//PIN: irq_n : 29
//PIN: i_rtcirq : 49
//PIN: i_rw : 69
//PIN: i_uartirq : 51
//PIN: paddr_0 : 60
//PIN: paddr_1 : 61
//PIN: paddr_2 : 63
//PIN: paddr_3 : 64
//PIN: paddr_4 : 65
//PIN: paddr_5 : 67
//PIN: pgfault_n : 30
//PIN: ramcs_n : 57
//PIN: romcs_n : 58
//PIN: rtccs_n : 52
//PIN: TCK : 62
//PIN: TDI : 14
//PIN: TDO : 71
//PIN: TMS : 23
//PIN: uartcs_n : 56
