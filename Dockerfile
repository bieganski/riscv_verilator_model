FROM debian:bullseye

LABEL maintainer="Anderson Ignacio da Silva <anderson@aignacio.com>"
RUN apt-get update && \
    apt-get install -y git gtkwave make build-essential

COPY . /rv

COPY verilator_bins.veri/* /usr/bin/

RUN mkdir -p /usr/local/share/verilator/bin/
RUN mkdir -p /usr/local/share/verilator/include/vltstd/


COPY include.veri/* /usr/local/share/verilator/include/
COPY vltstd.veri/* /usr/local/share/verilator/include/vltstd/
COPY bin.veri/* /usr/local/share/verilator/bin/


# FIXME: it doesn't compile with MAX_THREAD=1, why?
RUN cd /rv && \
	make JTAG_BOOT=1 JTAG_PORT=8080 MAX_THREAD=12 verilator 

EXPOSE 8080/tcp
WORKDIR /rv/output_verilator

CMD ./riscv_soc 
# sw/hello_world/output/hello_world.elfs
