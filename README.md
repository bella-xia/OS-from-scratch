**Steps to Start the OS**

1. `make clean`
2. `./build.sh`
3. `qemu-system-x86_64 -hda bin/os.bin -monitor stdio`

**To debug using gdb**

1. on one window: `qemu-system-x86_64 -hda bin/os.bin -S -gdb tcp::1234 -monitor stdio`
2. on another:
   
    a. `gdb`
   
    b. after inside the gdb window, `target remote localhost:1234`

    c. load the symbols, `add-symbol-file build/kernelfull.o`

    d. use `b [func_name]` to specify any function call to debug, then use `run` to start the process
