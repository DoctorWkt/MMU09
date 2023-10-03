// Verilog version of the MMU09 single board computer
// (c) 2022 Warren Toomey, GPL3

`default_nettype none
`include "mc6809e.v"
`include "mc6809i.v"
`include "mmu_decode.v"
`include "ram.v"
`include "rom.v"
`include "uart.v"

module mmu09_sbc (i_qclk, i_eclk, i_reset_n,
		  i_irq_n, i_firq_n, i_nmi_n, vadr);

  input i_qclk;				// Q clock
  input i_eclk;				// E clock
  input i_reset_n;			// Reset line
  input i_irq_n;			// IRQ line
  input i_firq_n;			// FIRQ line
  input i_nmi_n;			// NMI line
  output [15:0] vadr;			// Address bus value

  // Internal signals
  wire [7:0] datain;			// Data into the CPU
  wire [7:0] dataout;			// Data out from the CPU
  wire rw;				// Read/write line
  wire bs;				// BS line
  wire ba;				// BA line
  wire avma;				// AVMA line
  wire busy;				// BUSY line
  wire lic;				// LIC line
  wire halt_n;				// HALT line

  // These wires temporarily unused so that
  // we can send interrupts from the testbed
  wire irq_n;
  wire firq_n;
  wire nmi_n;

  wire romcs_n;				// ROM chip select
  wire ramcs_n;				// RAM chip select
  wire uartrd_n;			// UART read enable
  wire uartwr_n;			// UART write enable
  wire chrd_n;				// CH375 card read
  wire chwr_n;				// CH375 card write
  wire rtccs_n;				// Real-time clock chip select
  wire [5:0] frame;			// Page frame number
  wire [18:0] padr;			// Physical RAM address
  wire pgfault_n;			// Page fault signal

  wire uartirq_n;			// UART interrupt request
  wire chirq_n;				// CH375 interrupt request
  wire rtcirq_n;			// Real-time clock interrupt request

  wire [7:0] romdata;			// Data from the ROM
  wire [7:0] ramdata;			// Data from the RAM

  // MMU and address decoding device. Use the low vadr bits as
  // the page offset for the physical address
  mmu_decode MMU(i_qclk, i_eclk, i_reset_n, rw, vadr, dataout, bs,
	uartirq_n, chirq_n, rtcirq_n,
	romcs_n, ramcs_n, uartrd_n, uartwr_n, chrd_n, chwr_n,
	rtccs_n, frame, pgfault_n, irq_n, firq_n, halt_n);
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
  uart UART(dataout, uartrd_n, uartwr_n);

  // The CPU device
  mc6809e CPU(datain, dataout, vadr, rw, i_eclk, i_qclk, bs, ba, i_irq_n,
	      i_firq_n, i_nmi_n, avma, busy, lic, halt_n, i_reset_n);

  // Get an NMI on a pagefault. Don't do IRQ or FIRQ for now.
  assign nmi_n= pgfault_n;
  assign uartirq_n= 1'b1;
  assign chirq_n=   1'b1;
  assign rtcirq_n=  1'b1;

endmodule
