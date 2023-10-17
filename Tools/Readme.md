The `mkfs.c` code comes from the `xv6` project, but I've modified it
so that it can mirror the contents of an existing directory including
subdirectories. It also can create a big-endian filesystem even if the
program runs on a littl-endian computer.
