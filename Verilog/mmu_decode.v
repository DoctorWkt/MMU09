// MMU and address decoding for the MMU09 SBC
// (c) 2023 Warren Toomey, BSD license

module mmu_decode(i_qclk, i_eclk, i_reset, i_rw, i_addr, i_data, i_bs,
		  i_uartirq, i_chirq, i_rtcirq,
		  romcs_n, ramcs_n, uartrd_n, uartwr_n, chrd_n, chwr_n,
		  rtccs_n, paddr, pgfault_n, irq_n, firq_n, halt_n
`ifdef TESTING
`endif
		  );

				// _n suffix lines are active low
  input i_qclk;			// 6809 Q clock signal
  input i_eclk;			// 6809 E clock signal
  input i_reset;		// Reset line, active low
  input i_rw;			// 6809 R/W signal
  input [15:0] i_addr;		// Input virtual address
  input [7:0] i_data;		// Data bus
  input i_bs;			// 6809 BS line
  input i_uartirq;		// Interrupt from the UART
  input i_chirq;		// Interrupt from the CH375
  input i_rtcirq;		// Interrupt from the real-time clock

  output romcs_n;		// ROM chip select
  output ramcs_n;		// RAM chip select
  output uartrd_n;		// UART read enable
  output uartwr_n;		// UART write enable
  output chrd_n;		// CH375 read enable
  output chwr_n;		// CH375 write enable
  output rtccs_n;		// Real-time clock chip select
  output [18:13] paddr;		// Frame number for the input virtual address
  output pgfault_n;		// Active if page table entry is invalid
  output irq_n;			// Interrupt from either UART or CH375
  output firq_n;		// Fast interrupt to the 6809
  output halt_n;		// Output to the 6809 HALT line

`ifdef TESTING
`else
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


  //////////////////////
  // ADDRESS DECODING //
  //////////////////////

  // Toggle: when true, map the I/O and page tables into memory.
  // When false, ROM is mapped at the same location.
  // Effectively, when true this indicates that we are in kernel mode.
  reg io_pte_mapped;
  initial io_pte_mapped = 1'b1;

  // Toggle: when true, most of the 32K of ROM is mapped in to memory.
  // When false, RAM pages are mapped at the same locations.
  // This can only be toggled when in kernel mode.
  reg rom_mapped;
  initial rom_mapped = 1'b0;

  // ffxx: active high for $FFxx addresses (top 256 ROM bytes)
  wire ffxx;
  assign ffxx= i_addr[15] & i_addr[14] & i_addr[13] & i_addr[12] &
               i_addr[11] & i_addr[10] & i_addr[9]  & i_addr[8];

  // fexx: active high for $FExx addresses (I/O)
  wire fexx;
  assign fexx= i_addr[15] & i_addr[14] & i_addr[13] & i_addr[12] &
               i_addr[11] & i_addr[10] & i_addr[9]  & !i_addr[8];

  // Map in the I/O area when the 6809 BS signal goes high or
  // when the reset line goes low. Also map out the 32K ROM.
  // Thus, go into "kernel mode" on these signals.
  always @(posedge i_eclk) begin
    if (i_bs || i_reset == 1'b0) begin
      io_pte_mapped <= 1'b1;
      rom_mapped <= 1'b0;
    end
  end

  // This wire is true when we have an $FExx 
  // address and the I/O area is enabled.
  wire io_active;
  assign io_active = fexx & io_pte_mapped;

  // romcs_n: Active low on addresses $FFxx, or on addresses
  // $8000 and up when rom_mapped is set and no I/O is active.
  assign romcs_n= ! (ffxx | (i_addr[15] & rom_mapped & !io_active));

  // ramcs_n: Active low when neither the ROM or I/O are active.
  assign ramcs_n= !romcs_n | io_active;

  // I/O is mapped into the address range $FE00 to $FE7F when io_active.
  // Decode some of the address bits for the various chip select lines.
  // We can only lower the select lines when i_eclk is high
  // because the data bus isn't valid until then.
  //
  // The address map for the I/O area is (in 16-byte regions):
  // $FE00: RTC chip select
  // $FE10: UART read enable
  // $FE20: UART write enable
  // $FE30: CH375 read enable
  // $FE40: CH375 write enable
  // $FE50: map in/out the 32K ROM using the low address bit
  // $FE60: map out the I/O area
  // $FE70: eight page table entries

  assign rtccs_n=  (i_eclk & io_active & i_addr[7:4] == 4'h0) ? 0 : 1;
  assign uartrd_n= (i_eclk & io_active & i_addr[7:4] == 4'h1) ? 0 : 1;
  assign uartwr_n= (i_eclk & io_active & i_addr[7:4] == 4'h2) ? 0 : 1;
  assign chrd_n=   (i_eclk & io_active & i_addr[7:4] == 4'h3) ? 0 : 1;
  assign chwr_n=   (i_eclk & io_active & i_addr[7:4] == 4'h4) ? 0 : 1;

  always @(posedge i_eclk) begin
    if (io_active & i_addr[7:4] == 4'h5) rom_mapped <= i_addr[0];
    if (io_active & i_addr[7:4] == 4'h6) io_pte_mapped <= 0;
  end


  ///////////////////////
  // MEMORY MANAGEMENT //
  ///////////////////////

  reg [7:0] pgtable[0:7];	// The page table. Low six bits are
				// the frame number. Top bit set
				// indicates that the page is invalid.

  // Get the address lines that index into the page table.
  wire [2:0] pgindex= i_addr[15:13];

  // When we are writing to the page table entries,
  // we use the low address bits so we get indices 0 .. 7.
  wire [2:0] writeindex= i_addr[2:0];

  // Page table entry found by indexing the page table.
  wire [7:0] pte;

  // Page mapping. Six bits become the frame number.
  assign pte= pgtable[pgindex];
  assign paddr= pte[5:0];

  // Page table entry updates on kernel writes to $FF7x.
  always @(posedge i_eclk)
    if (io_active & i_addr[7:4] == 4'h7) pgtable[writeindex] <= i_data;


  /////////////////////////
  // PAGE FAULT HANDLING //
  /////////////////////////

  // Send an interrupt when there is an access to an invalid
  // page in user mode. The top bit of the pte marks "invalid".
  // We have a small state machine for page fault handling, so
  // that we drop the interrupt line and raise it soon after.

  // Three states: no page fault (11), a page fault is signalled before
  // we get to kernel mode (10),  the page fault signal is disabled before
  // we get to kernel mode (01). The low bit becomes the interrupt signal.

  reg [1:0] faultstate;
  initial faultstate = 2'b11;
  assign pgfault_n= faultstate[0];	  // Output to the NMI line

  always @(posedge i_eclk) begin
	  				  // Invalid page in user mode:
					  // signal a page fault.
    if (faultstate == 2'b11 & pte[7] & !io_pte_mapped)
      faultstate <= 2'b10;
    if (faultstate == 2'b10)		  // Turn it off a clock cycle later
      faultstate <= 2'b01;
    if (faultstate == 2'b01 & io_pte_mapped) // And set to no fault once we
      faultstate <= 2'b11;		  // reach kernel mode
    if (i_reset == 1'b0)		  // No fault after a reset
      faultstate <= 2'b11;
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
//PIN: i_data_6 : 79
//PIN: i_data_7 : 80
//PIN: i_eclk : 83
//PIN: i_bs : 27
//PIN: i_qclk : 2
//PIN: irq_n : 29
//PIN: i_reset : 1
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
//PIN: uartrd_n : 56
//PIN: uartwr_n : 45
