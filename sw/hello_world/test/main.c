#include <stdio.h>


int main() {
	printf("hello world from debug hell\n");


        asm (

"csrw 0x782,x0\n"
"lp.setupi x1,100,stop_loop\n"
"p.lw x10,4(x14!)\n"
"stop_loop: add x11,x11,x10\n"
"csrr x15,0x782\n"

        );



	return 0;
}
