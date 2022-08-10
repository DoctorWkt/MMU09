## Things to Do or Consider

- Have I wired a clock signal to the DUART? Look at jberen, he has E going to X1 only.
- Wire RESET into the CPLD on GCLR
- Wire the R/W line to the ROM #OE line?
- Add another 512K of RAM and use the spare bit in the page table entries?
- Look at plasmo's JTAG design: 4.7K R TMS to Vcc, 4.7K R TCLK to Gnd.
- Wire unused CPLD lines to ground, as per plasmo (Bill Shen)?
- Or, wire them out to a pin header so we can use them later?
- plasmo uses MCP130D for the reset circuit. Doesn't need any caps.

- Check the CF card design on 68Katy.
- On the Tiny68K plasmo design, there's a 44-pin IDE socket. Check it as well.
