// Test module for the decode unit
// (c) 2022 Warren Toomey, GPL3

`timescale 1ns/1ps
`default_nettype none
`define TESTING
`include "mmu_decode.v"
`include "tbhelper.v"

module tb_mmu_decode;

`TBASSERT_METHOD(tbassert)

// DUT inputs
  reg i_qclk;                 // 6809 Q clock signal
  reg i_eclk;                 // 6809 E clock signal
  reg i_rw;                   // 6809 R/W signal
  reg [15:0] i_addr;          // Input virtual address
  reg [7:0]  i_data;          // Data bus
  reg i_kmodeset;             // Set kernel mode signal

// DUT outputs
  wire romcs_n;               // ROM chip select
  wire ramcs_n;               // RAM chip select
  wire uartcs_n;              // UART chip select
  wire cfcs_n;                // CF card chip select
  wire rtccs_n;               // Real-time clock chip select
  wire [18:13] paddr;         // Frame number for the input virtual address
  wire pgfault_n;             // Active if page table entry is invalid
  wire ffxx;                  // True for $FFxx addresses
  wire kernio;                // True in kernel mode for $FExx
  wire kupper;                // True in kernel mode for top 32K addresses
  wire kernel;		      // Kernel mode

// DUT
  mmu_decode dut(i_qclk, i_eclk, i_rw, i_addr, i_data, i_kmodeset,
		 romcs_n, ramcs_n, uartcs_n, cfcs_n, rtccs_n, paddr, pgfault_n,
		 ffxx, kernio, kupper, kernel);

  initial begin

    $dumpfile("tb_decode.vcd");
    $dumpvars;
#100
    ////////////////////
    // DECODE TESTING //
    ////////////////////

    // Enable kernel mode by raising the set line
#100
    i_eclk= 1'b0;
    i_kmodeset= 1'b0;
    i_addr= 16'h0200;
#100
    i_eclk= 1'b1;
    i_kmodeset= 1'b1;
#100
    i_eclk= 1'b0;
    i_kmodeset= 1'b0;
#100
    tbassert(kernel == 1'b1, "Test 1, kernel mode");

    // Address $0200 should be RAM
    tbassert(ffxx == 1'b0,      "Test 2a, kernel addr $0200");
    tbassert(kernio == 1'b0,    "Test 2b");
    tbassert(kupper == 1'b0,    "Test 2bb");
    tbassert(romcs_n == 1'b1,   "Test 2c");
    tbassert(ramcs_n == 1'b0,   "Test 2d");
    tbassert(uartcs_n == 1'b1,  "Test 2e");
    tbassert(cfcs_n == 1'b1,    "Test 2f");
    tbassert(rtccs_n == 1'b1,   "Test 2g");
    tbassert(kernel == 1'b1,    "Test 2k");

    // Try address $8000, should be ROM
    i_addr= 16'h8000;
#100
    tbassert(ffxx == 1'b0,      "Test 3a, kernel addr $8000");
    tbassert(kernio == 1'b0,    "Test 3b");
    tbassert(kupper == 1'b1,    "Test 3bb");
    tbassert(romcs_n == 1'b0,   "Test 3c");
    tbassert(ramcs_n == 1'b1,   "Test 3d");
    tbassert(uartcs_n == 1'b1,  "Test 3e");
    tbassert(cfcs_n == 1'b1,    "Test 3f");
    tbassert(rtccs_n == 1'b1,   "Test 3g");
    tbassert(kernel == 1'b1,    "Test 3k");

    // Try address $FE00, should be UART
    i_addr= 16'hFE00;
#100
    tbassert(ffxx == 1'b0,      "Test 4a, kernel addr $FE00");
    tbassert(kernio == 1'b1,    "Test 4b");
    tbassert(kupper == 1'b1,    "Test 4bb");
    tbassert(romcs_n == 1'b1,   "Test 4c");
    tbassert(ramcs_n == 1'b1,   "Test 4d");
    tbassert(uartcs_n == 1'b0,  "Test 4e");
    tbassert(cfcs_n == 1'b1,    "Test 4f");
    tbassert(rtccs_n == 1'b1,   "Test 4g");

    // Try address $FF00, ROM should be on
    i_addr= 16'hFF00;
#100
    tbassert(ffxx == 1'b1,      "Test 5a, kernel addr $FF00");
    tbassert(kernio == 1'b0,    "Test 5b");
    tbassert(romcs_n == 1'b0,   "Test 5c");
    tbassert(ramcs_n == 1'b1,   "Test 5d");
    tbassert(uartcs_n == 1'b1,  "Test 5e");
    tbassert(cfcs_n == 1'b1,    "Test 5f");
    tbassert(rtccs_n == 1'b1,   "Test 5g");

    // Try address $FE20, should be CF card
    i_addr= 16'hFE20;
#100
    tbassert(ffxx == 1'b0,      "Test 6a, kernel addr $FE20");
    tbassert(kernio == 1'b1,    "Test 6b");
    tbassert(romcs_n == 1'b1,   "Test 6c");
    tbassert(ramcs_n == 1'b1,   "Test 6d");
    tbassert(uartcs_n == 1'b1,  "Test 6e");
    tbassert(cfcs_n == 1'b0,    "Test 6f");
    tbassert(rtccs_n == 1'b1,   "Test 6g");

    // Try address $FEFF, should move to user mode
    i_eclk= 1'b1;
    i_addr= 16'hFEFF;
#100
    i_eclk= 1'b0; i_addr= 16'h0200;
#100
    tbassert(kernel == 1'b0, "Test 7");

    // Now that we are in user mode, let's see what's mapped in where.
    i_addr= 16'h0000;
#100
    tbassert(ffxx == 1'b0,      "Test 7a, user mode addr $0000");
    tbassert(kernio == 1'b0,    "Test 7b");
    tbassert(romcs_n == 1'b1,   "Test 7c");
    tbassert(ramcs_n == 1'b0,   "Test 7d");
    tbassert(uartcs_n == 1'b1,  "Test 7e");
    tbassert(cfcs_n == 1'b1,    "Test 7f");
    tbassert(rtccs_n == 1'b1,   "Test 7g");

    i_addr= 16'h8000;
#100
    tbassert(ffxx == 1'b0,      "Test 8a, user mode addr $8000");
    tbassert(kernio == 1'b0,    "Test 8b");
    tbassert(romcs_n == 1'b1,   "Test 8c");
    tbassert(ramcs_n == 1'b0,   "Test 8d");
    tbassert(uartcs_n == 1'b1,  "Test 8e");
    tbassert(cfcs_n == 1'b1,    "Test 8f");
    tbassert(rtccs_n == 1'b1,   "Test 8g");

    i_addr= 16'hFE00;
#100
    tbassert(ffxx == 1'b0,      "Test 9a, user mode addr $FE00");
    tbassert(kernio == 1'b0,    "Test 9b");
    tbassert(romcs_n == 1'b1,   "Test 9c");
    tbassert(ramcs_n == 1'b0,   "Test 9d");
    tbassert(uartcs_n == 1'b1,  "Test 9e");
    tbassert(cfcs_n == 1'b1,    "Test 9f");
    tbassert(rtccs_n == 1'b1,   "Test 9g");

    i_addr= 16'hFF00;
#100
    tbassert(ffxx == 1'b1,      "Test 10a, user mode addr $FF00");
    tbassert(kernio == 1'b0,    "Test 10b");
    tbassert(romcs_n == 1'b0,   "Test 10c");
    tbassert(ramcs_n == 1'b1,   "Test 10d");
    tbassert(uartcs_n == 1'b1,  "Test 10e");
    tbassert(cfcs_n == 1'b1,    "Test 10f");
    tbassert(rtccs_n == 1'b1,   "Test 10g");

    /////////////////
    // MMU TESTING //
    /////////////////

    // Enable kernel mode by raising the set line
    i_eclk= 1'b0;
#100
    i_eclk= 1'b1;
    i_kmodeset= 1'b1;
#100
    i_eclk= 1'b0; i_rw= 1'b1; i_kmodeset= 1'b0;

    // Set up some low 32K page table entries
#100
    i_addr= 16'hFEC0; i_data=8'h80;
#100
    i_eclk= 1'b1; i_rw= 1'b0;
#100
    i_eclk= 1'b0; i_rw= 1'b1;
#100
    i_addr= 16'hFEC1; i_data=8'h82;
#100
    i_eclk= 1'b1; i_rw= 1'b0;
#100
    i_eclk= 1'b0; i_rw= 1'b1;
#100
    i_addr= 16'hFEC2; i_data=8'h83;
#100
    i_eclk= 1'b1; i_rw= 1'b0;
#100
    i_eclk= 1'b0; i_rw= 1'b1;
#100
    i_addr= 16'hFEC3; i_data=8'h84;
#100
    i_eclk= 1'b1; i_rw= 1'b0;
#100
    i_eclk= 1'b0; i_rw= 1'b1;
#100
    // Set up some high 32K page table entries. Make one an invalid pte
#100
    i_addr= 16'hFEC4; i_data=8'h8c;
#100
    i_eclk= 1'b1; i_rw= 1'b0;
#100
    i_eclk= 1'b0; i_rw= 1'b1;
#100
    i_addr= 16'hFEC5; i_data=8'h8d;
#100
    i_eclk= 1'b1; i_rw= 1'b0;
#100
    i_eclk= 1'b0; i_rw= 1'b1;
#100
    i_addr= 16'hFEC6; i_data=8'h0e;
#100
    i_eclk= 1'b1; i_rw= 1'b0;
#100
    i_eclk= 1'b0; i_rw= 1'b1;
#100
    i_addr= 16'hFEC7; i_data=8'h8f;
#100
    i_eclk= 1'b1; i_rw= 1'b0;
#100
    i_eclk= 1'b0; i_rw= 1'b1;
#100
    // Access address $FEFF to move to user mode
    i_eclk= 1'b1;
    i_addr= 16'hFEFF;
#100
    i_eclk= 1'b0; i_addr= 16'h0200;

    // Now access some low memory locations and check the page frames
#100
    i_addr= 16'h1234;
#100
    tbassert(paddr == 6'h0, "Test 11a"); tbassert(pgfault_n == 1'h1, "Test 11b");
    i_addr= 16'h2222;
#100
    tbassert(paddr == 6'h2, "Test 11c"); tbassert(pgfault_n == 1'h1, "Test 11d");
    i_addr= 16'h4004;
#100
    tbassert(paddr == 6'h3, "Test 11e"); tbassert(pgfault_n == 1'h1, "Test 11f");
    i_addr= 16'h6543;
#100
    tbassert(paddr == 6'h4, "Test 11g"); tbassert(pgfault_n == 1'h1, "Test 11h");

   // Now access some high memory locations and check the page frames
#100
    i_addr= 16'h8000;
#100
    tbassert(paddr == 6'hc, "Test 12a"); tbassert(pgfault_n == 1'h1, "Test 12b");
    i_addr= 16'habcd;
#100
    tbassert(paddr == 6'hd, "Test 12c"); tbassert(pgfault_n == 1'h1, "Test 12d");
    i_addr= 16'hdddd;
#100
    tbassert(paddr == 6'he, "Test 12e"); tbassert(pgfault_n == 1'h0, "Test 12f");
    i_addr= 16'hfeed;
#100
    tbassert(paddr == 6'hf, "Test 12g"); tbassert(pgfault_n == 1'h1, "Test 12h");

    // Now go into kernel mode and try to access
    // the page which is invalid in user mode
    i_eclk= 1'b0;
    i_kmodeset= 1'b0;
    i_addr= 16'h0200;
#100
    i_eclk= 1'b1;
    i_kmodeset= 1'b1;
#100
    i_eclk= 1'b0;
    i_kmodeset= 1'b0;
    i_addr= 16'hdddd;
#100
    tbassert(paddr == 6'he, "Test 13a"); tbassert(pgfault_n == 1'h1, "Test 13b");

  end

endmodule
