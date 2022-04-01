.globl fun_stub
.text

.code64
fun_stub:
subq $8, %rsp
lea fun_stub_32(%rip), %eax
movl $0x23, 4(%rsp)
movl %eax, (%rsp)
lretl


.code32
fun_stub_32:
addl $4, %eax
pushl $0x2b
popl %ds
pushl $0x2b
popl %es

call fun



.code64
fun_stub_64:
retq



.code64
fun:
movl $0x2, %eax
ret
