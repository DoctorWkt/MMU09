// Verilog version of the MMU09 single board computer
// (c) 2022 Warren Toomey, GPL3

`default_nettype none
`include "mc6809e.v"
`include "mc6809i.v"
`include "mmu_decode.v"
`include "ram.v"
`include "rom.v"
`include "uart.v"

module mmu09_sbc (i_qclk, i_eclk, i_reset_n, vadr);

  input i_qclk;				// Q clock
  input i_eclk;				// E clock
  input i_reset_n;			// Reset line
  output [15:0] vadr;			// Address bus value

  // Internal signals
  wire [7:0] datain;			// Data into the CPU
  wire [7:0] dataout;			// Data out from the CPU
  wire rw;				// Read/write line
  wire bs;				// BS line
  wire ba;				// BA line
  wire irq_n;				// IRQ line
  wire firq_n;				// FIRQ line
  wire nmi_n;				// NMI line
  wire avma;				// AVMA line
  wire busy;				// BUSY line
  wire lic;				// LIC line
  wire halt_n;				// HALT line

  wire romcs_n;				// ROM chip select
  wire ramcs_n;				// RAM chip select
  wire uartcs_n;			// UART chip select
  wire cfcs_n;				// CF card chip select
  wire rtccs_n;				// Real-time clock chip select
  wire [5:0] frame;			// Page frame number
  wire [18:0] padr;			// Physical RAM address
  wire pgfault_n;			// Page fault signal

  wire [7:0] romdata;			// Data from the ROM
  wire [7:0] ramdata;			// Data from the RAM

  // MMU and address decoding device. Use the low vadr bits as
  // the page offset for the physical address
  mmu_decode MMU(i_qclk, i_eclk, rw, vadr, dataout, bs, romcs_n, ramcs_n,
		 uartcs_n, cfcs_n, rtccs_n, frame, pgfault_n);
  assign padr[12:0]= vadr[12:0];
  assign padr[18:13]= frame;

  // Memory devices: 32K of ROM and 512K of RAM
  rom #(.AddressSize(15), .Filename("monitor.rom"),
        .DELAY_RISE(55), .DELAY_FALL(55))
        ROM(vadr[14:0], romdata, romcs_n, 1'b0);

  ram #(.AddressSize(19))
        RAM(padr[18:0], dataout, ramcs_n, rw, !rw, ramdata);

  // Multiplex the data from memory onto the 6809 datain.
  // As there is no I/O input at present, simply read from
  // RAM if not reading from ROM.
  assign datain= (!romcs_n) ? romdata : ramdata;

  // The UART device.
  uart UART(dataout, uartcs_n, rw);

  // The CPU device
  mc6809e CPU(datain, dataout, vadr, rw, i_eclk, i_qclk, bs, ba, irq_n,
	      firq_n, nmi_n, avma, busy, lic, halt_n, i_reset_n);

  // Prevent halts and interrupts
  assign irq_n= 1'b1;
  assign firq_n= 1'b1;
  assign nmi_n= pgfault_n;
  assign halt_n= 1'b1;

endmodule
