// MMU and address decoding for the MMU09 SBC
// (c) 2023 Warren Toomey, BSD license
// $Revision: 1.49 $

// This version puts 256 of ROM at $FFxx, the I/O area at $FExx
// and the rest of the ROM (24K) from $2000 up to $7FFF. There is a
// toggle for the ROM from $2000 up to $7FFF.

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
  input rtccs_n;		// Real-time clock chip select

  output romcs_n;		// ROM chip select
  output ramcs_n;		// RAM chip select
  output uartrd_n;		// UART read enable
  output uartwr_n;		// UART write enable
  output chrd_n;		// CH375 read enable
  output chwr_n;		// CH375 write enable
  output [18:13] paddr;		// Frame number for the input virtual address
  output pgfault_n;		// Active if page table entry is invalid
  output irq_n;			// Normal interrupt to the 6809
  output firq_n;		// Fast interrupt to the 6809
  output halt_n;		// Output to the 6809 HALT line

  ////////////////////////////
  // INTERRUPT MULTIPLEXING //
  ////////////////////////////

  assign irq_n=  i_uartirq;
  assign firq_n= i_chirq;

  ////////////////
  // HALT LOGIC //
  ////////////////

  assign halt_n= 1'b1;		// No halts just yet :-)

  //////////////////////
  // ADDRESS DECODING //
  //////////////////////

  // We have two toggles: one for the I/O area called io_pte_mapped
  // and one for the 24K ROM area called rom_mapped. We keep a
  // stack of these so that we can go back to the previous states
  // for both of them.

  // io_pte_mapped: when true, map the I/O and page tables into memory
  // at $FExx. When false, RAM is mapped at the same location.
  // Effectively, when true this indicates that we are in kernel mode.
  reg io_pte_mapped[0:3];
  initial io_pte_mapped[0] = 1'b1;

  // rom_mapped: when true, 24K of ROM is mapped in to memory from $2000
  // to $7FFF. When false, RAM pages are mapped at the same locations.
  reg rom_mapped[0:3];
  initial rom_mapped[0] = 1'b1;

  // io_idx is the index to the above two arrays.
  reg [1:0] io_idx;
  initial io_idx = 2'b00;

  // Wires that point at the current toggle values
  wire mapped_io  = io_pte_mapped[io_idx];
  wire mapped_rom = rom_mapped[io_idx];

  // ffxx: active high for $FFxx addresses (top 256 ROM bytes)
  wire ffxx= i_addr[15] & i_addr[14] & i_addr[13] & i_addr[12] &
             i_addr[11] & i_addr[10] & i_addr[9]  & i_addr[8];

  // fexx: active high for $FExx addresses (I/O area)
  wire fexx= i_addr[15] & i_addr[14] & i_addr[13] & i_addr[12] &
             i_addr[11] & i_addr[10] & i_addr[9]  & !i_addr[8];

  // low24: active high for the 24K ROM addresses: $2000 to $7FFF
  wire low24= ! i_addr[15] & ( i_addr[14] | i_addr[13] );

  // This wire is true when we have an $FExx 
  // address and the I/O area is enabled.
  wire io_active = fexx & mapped_io;

  // romcs_n: Active low on addresses $FFxx, or on
  // addresses $2000 to $7FFF when the 24K ROM is mapped.
  assign romcs_n= ! (ffxx | (mapped_rom & low24) );

  // ramcs_n: Active low when i_eclk high and neither the ROM or I/O are active.
  assign ramcs_n= !( i_eclk & (romcs_n & !io_active));

  // I/O is mapped into the address range $FE00 to $FE7F when io_active.
  // Decode some of the address bits for the various chip select lines.
  // We can only lower the select lines when i_eclk is high
  // because the data bus isn't valid until then.
  //
  // The address map for the I/O area is (in 16-byte regions):
  // $FE10: UART read enable
  // $FE20: UART write enable
  // $FE30: CH375 read enable
  // $FE40: CH375 write enable
  // $FE50: map in/out the 24K ROM using the low address bit
  // $FE60: map out both the I/O area and the 24K ROM
  // $FE70: eight page table entries
  // $FE80: go back to the previous I/O area and 24K ROM toggle values

  assign uartrd_n= (i_eclk & io_active & i_addr[7:4] == 4'h1) ? 0 : 1;
  assign uartwr_n= (i_eclk & io_active & i_addr[7:4] == 4'h2) ? 0 : 1;
  assign chrd_n=   (i_eclk & io_active & i_addr[7:4] == 4'h3) ? 0 : 1;
  assign chwr_n=   (i_eclk & io_active & i_addr[7:4] == 4'h4) ? 0 : 1;

  // In the following code we need to make a change when i_bs is high.
  // It stays high across two clock cycles. So we keep a state variable
  // to ensure we only make the change on one of the clock cycles.
  reg prev_bs;
  initial prev_bs = 1'b0;

  // Deal with changes to the current io_pte_mapped and rom_mapped
  // values, and do the stacking and unstacking of their values
  always @(posedge i_eclk) begin

    // Set prev_bs back to zero when i_bs falls
    if (i_bs == 1'b0)
      prev_bs = 1'b0;

    // On a reset, set the I/O toggle true, the 24K ROM toggle true
    //  and go back to index zero.
    if (i_reset == 1'b0) begin
      io_idx 	        = 2'b00;
      io_pte_mapped[0] <= 1'b1;
      rom_mapped[0]    <= 1'b1;
    end

    // When BS goes high, move up to the next index position and set
    // the I/O and ROM toggle true. Do the increment first. Flip the prev_bs
    // to ensure that we only do this on one of the two clock cycles
    // when i_bs is high
    else if (i_bs == 1'b1 && prev_bs == 1'b0) begin
      io_idx		     = io_idx + 2'b01;
      io_pte_mapped[io_idx] <= 1'b1;
      rom_mapped[io_idx]    <= 1'b1;
      prev_bs		     = 1'b1;
    end

    // On an $FE5x access, en/disable the 24K ROM using the address lsb.
    else if (io_active & i_addr[7:4] == 4'h5) begin
      rom_mapped[io_idx] <= i_addr[0];
    end

    // On an $FE6x access, disable both the I/O area and the 24K ROM.
    // Also go back to io_idx zero.
    else if (io_active & i_addr[7:4] == 4'h6) begin
      io_idx 	        = 2'b00;
      io_pte_mapped[0] <= 1'b0;
      rom_mapped[0]    <= 1'b0;
    end

    // On an $FE8x access, go back to the previous I/O area & 24K ROM values
    else if (io_active & i_addr[7:4] == 4'h8) begin
      io_idx = io_idx - 2'b01;
    end
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
  wire [7:0] pte= pgtable[pgindex];

  // Six bits of the pte become the frame number.
  assign paddr= pte[5:0];

  // Page table entry updates on kernel writes to $FF7x.
  always @(posedge i_eclk)
    if (io_active & i_addr[7:4] == 4'h7) pgtable[writeindex] = i_data;

  /////////////////////////
  // PAGE FAULT HANDLING //
  /////////////////////////

  // Send an interrupt when there is an access to an invalid
  // page in user mode. The top bit of the pte marks "invalid".
  // We have a small state machine for page fault handling, so
  // that we drop the interrupt line and raise it soon after.

  // Three states: 
  //   11: no page fault
  //   10: a page fault is signalled before we get to kernel mode
  //   01: the page fault signal is disabled before we get to kernel mode
  // The low bit becomes the interrupt signal.

  reg [1:0] faultstate;
  initial faultstate = 2'b11;
  assign pgfault_n= faultstate[0];        // Output to the NMI line

  always @(posedge i_eclk) begin
                                          // Invalid page in user mode:
                                          // signal a page fault.
    if (faultstate == 2'b11 & pte[7] & !mapped_io)
      faultstate <= 2'b10;
    if (faultstate == 2'b10)              // Turn it off a clock cycle later
      faultstate <= 2'b01;
    if (faultstate == 2'b01 & mapped_io) // And set to no fault once we
      faultstate <= 2'b11;                // reach kernel mode
    if (i_reset == 1'b0)                  // No fault after a reset
      faultstate <= 2'b11;
  end

  ////////////
  // BLINKY //
  ////////////

  // XXX This code is just to test that we
  // can program the CPLD. Eventually we will
  // re-purpose it to be the clock tick for
  // the board.

  reg [19:0] counter;
  initial counter = 20'h00000;

  always @(posedge i_eclk) begin
    counter <= counter + 1;
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
