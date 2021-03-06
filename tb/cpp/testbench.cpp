#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <signal.h>

#include "jtag_rbb.hpp"
#include "elfio/elfio/elfio.hpp"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vriscv_soc.h"
#include "Vriscv_soc__Syms.h"

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)

using namespace std;

void goingout(int s){
    printf("Caught signal %d\n",s);
    exit(1);
}

template<class module> class testbench {
	VerilatedVcdC *trace = new VerilatedVcdC;
    unsigned long tick_counter;
    bool getDataNextCycle;

    public:
        module *core = new module;
        bool loaded = false;

        testbench() {
            Verilated::traceEverOn(true);
            tick_counter = 0l;
        }

        ~testbench(void) {
            delete core;
            core = NULL;
        }

        virtual void reset_n(int rst_cyc) {
            for (int i=0;i<rst_cyc;i++) {
                core->reset_n = 0;
                core->jtag_trstn = 0;
                this->tick();
            }
            core->reset_n = 1;
            core->jtag_trstn = 1;
            this->tick();
        }

        virtual	void opentrace(const char *vcdname) {
            core->trace(trace, 99);
            trace->open(vcdname);
        }

        virtual void close(void) {
            if (trace) {
                trace->close();
                trace = NULL;
            }
        }

        virtual void tick(void) {

            if (getDataNextCycle) {
                getDataNextCycle = false;
                printf("%c",core->riscv_soc->getbufferReq());
            }

            if (core->riscv_soc->printfbufferReq())
                getDataNextCycle = true;

            core->core_clk = 0;
            core->eval();
            tick_counter++;
            if(trace) trace->dump(tick_counter);

            core->core_clk = 1;
            core->eval();
            tick_counter++;
            if(trace) trace->dump(tick_counter);
        }

        virtual bool done(void) {
            return (Verilated::gotFinish());
        }
};

bool loadELF(testbench<Vriscv_soc> *cpu, const char *program_path, const bool en_print){
    ELFIO::elfio program;

    program.load(program_path);

    if (program.get_class() != ELFCLASS32 ||
        program.get_machine() != 0xf3){
        cout << "\n[ERROR] Error loading ELF file, headers does not match with ELFCLASS32/RISC-V!" << endl;
        return false;
    }

    ELFIO::Elf_Half seg_num = program.segments.size();

    if (en_print){
        printf( "\n[ELF Loader]"        \
                "\nProgram path: %s"    \
                "\nNumber of segments (program headers): %d",program_path,seg_num);
    }

    for (uint8_t i = 0; i<seg_num; i++){
        const ELFIO::segment *p_seg = program.segments[i];
        const ELFIO::Elf64_Addr lma_addr = (uint32_t)p_seg->get_physical_address();
        const ELFIO::Elf64_Addr vma_addr = (uint32_t)p_seg->get_virtual_address();
        const uint32_t mem_size = (uint32_t)p_seg->get_memory_size();
        const uint32_t file_size = (uint32_t)p_seg->get_file_size();
        // const char *data_pointer = p_seg->get_data();

        if (en_print){
            printf("\nSegment [%d] - LMA[0x%x] VMA[0x%x]", i,(uint32_t)lma_addr,(uint32_t)vma_addr);
            printf("\nFile size [%d] - Memory size [%d]",file_size,mem_size);
        }

        // Notes about loading .data and .bss
        // > According to:
        // https://www.cs.bgu.ac.il/~caspl112/wiki.files/lab9/elf.pdf
        // Page 34:
        // The array element specifies a loadable segment, described by p_filesz and p_memsz.
        // The bytes from the file are mapped to the beginning of the memory segment. If the
        // segment’s memory size (p_memsz) is larger than the file size (p_filesz), the ‘‘extra’’
        // bytes are defined to hold the value 0 and to follow the segment’s initialized area. The file
        // size may not be larger than the memory size. Loadable segment entries in the program
        // header table appear in ascending order, sorted on the p_vaddr member.
        if (mem_size >= (IRAM_KB_SIZE*1024)){
            printf("\n\n[ELF Loader] ERROR:");
            printf("\nELF program: %d bytes", mem_size);
            printf("\nVerilator model memory size: %d bytes", (IRAM_KB_SIZE*1024));
            if (lma_addr >= 0xA0000000 && lma_addr < 0xB0000000)
                printf("\nIncrease your verilator model IRAM by %d kb\n", (mem_size - (IRAM_KB_SIZE*1024))/1024);
            else
                printf("\nIncrease your verilator model DRAM by %d kb\n", ((mem_size - (IRAM_KB_SIZE*1024))/1024)+1);
            return false;
        }

        if (lma_addr >= 0x20000000 && lma_addr <0x30000000){
            // IRAM Address
            if (en_print) printf("\nIRAM address space");
            for (uint32_t p = 0; p < mem_size; p+=4){
                uint32_t word_line = ((uint8_t)p_seg->get_data()[p+3]<<24)+((uint8_t)p_seg->get_data()[p+2]<<16)+((uint8_t)p_seg->get_data()[p+1]<<8)+(uint8_t)p_seg->get_data()[p];
                // if (en_print) printf("\nIRAM = %8x - %8x", p, word_line);
                cpu->core->riscv_soc->writeWordIRAM(p/4,word_line);
            }
        }
        else {
            // DRAM Address
            if (en_print) printf("\nDRAM address space");
            for (uint32_t p = 0; p < mem_size; p+=4){
                uint32_t word_line;
                if (p >= file_size) {
                    word_line = 0;
                }
                else {
                    word_line = ((uint8_t)p_seg->get_data()[p+3]<<24)+((uint8_t)p_seg->get_data()[p+2]<<16)+((uint8_t)p_seg->get_data()[p+1]<<8)+(uint8_t)p_seg->get_data()[p];
                }
                // if (en_print) printf("\nDRAM = %8x - %8x", p, word_line);
                cpu->core->riscv_soc->writeWordDRAM(p/4,word_line);
            }
        }
    }

    ELFIO::Elf64_Addr entry_point = program.get_entry();

    if(en_print) printf("\nEntry point: %8x", (uint32_t) entry_point);

    cpu->core->riscv_soc->writeRstAddr((uint32_t) entry_point);
    cpu->loaded = true;
    cout << endl << endl;
    return true;
}

int main(int argc, char** argv, char** env){
    Verilated::commandArgs(argc, argv);
    auto *rbb = new jtag_rbb(JTAG_PORT);
    unsigned char srst_pin;
    unsigned char trst_pin;
    auto *soc = new testbench<Vriscv_soc>;
    int test = 50;

    if (EN_VCD)
        soc->opentrace(STRINGIZE_VALUE_OF(WAVEFORM_VCD));

    rbb->tck_pin   = &soc->core->jtag_tck;
    rbb->tms_pin   = &soc->core->jtag_tms;
    rbb->tdi_pin   = &soc->core->jtag_tdi;
    rbb->tdo_pin   = &soc->core->jtag_tdo;
    rbb->trst_pin  = &trst_pin; // Must pass a address to the object member
    rbb->srst_pin  = &srst_pin; // Must pass a address to the object member
    rbb->trstn_pin = &soc->core->jtag_trstn;

    cout << "\n[RISCV SoC] Emulator started";
    if (EN_VCD)
        cout << "\n[VCD File] " << STRINGIZE_VALUE_OF(WAVEFORM_VCD);
    cout << "\n[IRAM KB Size] " << STRINGIZE_VALUE_OF(IRAM_KB_SIZE) << "KB";
    cout << "\n[DRAM KB Size] " << STRINGIZE_VALUE_OF(DRAM_KB_SIZE) << "KB \n";

    soc->core->rx_i = 1;
    soc->reset_n(1);

    if (JTAG_BOOT){
        rbb->accept_client();
        soc->core->riscv_soc->writeRstAddr(0x1A000000);
    }
    else
        if (!loadELF(soc, argv[1], true)) exit(1);

    soc->core->fetch_enable_i = 1;

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = goingout;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    while(true) {
        if (JTAG_BOOT)
            rbb->read_cmd(false);
		soc->tick();
	}
    soc->close();
    exit(EXIT_SUCCESS);
}

static vluint64_t  cpuTime = 0;

double sc_time_stamp (){
    return cpuTime;
}
