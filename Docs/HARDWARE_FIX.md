- Tie TSC low!!! FIXED.

- Two jumpers are out of order.
  Switch the jumpers on the
  schematic, not on the PCB.
  FIXED.

- The reset capacitor and the
  6809 capacitor need to be
  swapped on the PCB. FIXED.

- We can lose the RTC.
  I've left it for now until
  I can work out best how to
  rewire it.

- Don't use PE0 and PE1 as
  they are the ATMega2560
  serial I/O lines.

- Put pins for the six CPLD
  programming lines on top
  of the SBC so we can
  access them when the Arduino
  is mounted on the bottom.

- Change the data bus wiring to the
  Arduino so all wires are on port L.

- Fix up the address letters on the
  back silkscreen!!!
