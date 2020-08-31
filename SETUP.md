# Setup
***
File contains both `verilator` and `FPGA` setups.
As described in `README.md` file, common part of `verilator` and `FPGA` configs is installing `riscv-toolchain` and adding it to `$PATH` variable.
You can to it on your own, or use script:
```
./setup.sh
```
It will update your `.bashrc` file, if it's not what you want, then simply comment out call to `add_to_path` function at the bottom of `setup.sh`.

Then, take next step depending on your 

## `verilator` config

`riscv_verilator_model` is supported by `verilator` version `4.018`, it doesn't compile on Debian-repository releases like `4.16` or `4.20`.
After compiling `verilator 4.018` from source, we obtain several files (mainly binaries), that will be used in Docker image, all listed below:
* `bin.veri/` (TODO what's this?)
* `include.veri/` (TODO what's this?)
* `verilator_bins.veri/` (`verilator` binaries)

All of them are needed inside Docker container to work properly, thus they are copied before running.
You can run binary in container in two ways. Compiling it on host (remember about `VERILAT=1` flag!) and copying to image, or compile it inside image, i.a. using `make sw` command.
FIXME: Docker image doesn't see riscv-toolchain, thus now compilation only on host and pass it to image with COPY command.

`.elf`'s paths are passed as arguments to `riscv_soc` program throught `Dockerfile` like this:
```
CMD ./riscv_soc /rv/sw/hello_world/output/hello_world.elf
```

Finally, you can run all machinery with command
```
docker compose up --build
```

If you see only 
```
Attaching to riscv_verilator_model_verilator_1

```
Docker output and nothing more, check whether `.elf` you passed exists.


## `FPGA` config

To make it compile to FPGA bitstream, you must comment out `tx_mirror_o` occurences in `./tb/wrappers/fpga_wrapper/fpga_wrapper.sv` (it does nothing and only breaks compilation)
TODO - already applied?

then run
```
make fpga
```
It will create bitstream (`.bit` file, i.a. `artix_wrapper.bit` for Artix board). You can program FPGA with `.bit` file using `vivado` program).

To run your program, compile it to RISCV `.elf` binary and run
```
gdb -x gdb_cmd
```
where `gdb_cmd` is gdb script file (remember to adjust it's content, i.a. change `.elf` file path).

