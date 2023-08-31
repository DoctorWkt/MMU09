// RAM component
// (c) 2019 Warren Toomey, GPL3

module ram (Address, InData, CS_n, WE_n, OE_n, OutData);

  parameter AddressSize = 16;
  parameter WordSize = 8;
  parameter Filename= "empty.ram";
  parameter DELAY_RISE = 55;
  parameter DELAY_FALL = 55;

  input  [AddressSize-1:0] Address;
  input  [WordSize-1:0]    InData;
  input CS_n, WE_n, OE_n;
  output  [WordSize-1:0]   OutData;

  reg [WordSize-1:0] Mem [0:(1<<AddressSize)-1];

  // Initialise RAM to zero
  integer i;
  initial begin
    // Was $readmemh(Filename, Mem);
    for (i=0; i< (1<<AddressSize); i=i+1)
      Mem[i]= {WordSize{1'b0}};
  end

/* verilator lint_off ASSIGNDLY */
  assign #(DELAY_RISE, DELAY_FALL)
	OutData = (!CS_n && !OE_n) ? Mem[Address] : {WordSize{1'bz}};
/* verilator lint_on ASSIGNDLY */

  always @(negedge CS_n or negedge WE_n or negedge OE_n) begin
    if (!CS_n && !WE_n) begin
      Mem[Address] <= InData;
      // $write("%x to address %x\n", InData, Address);
    end

    if (!WE_n && !OE_n)
      $display("ram error: both OE_n & WE_n active");
  end

endmodule
