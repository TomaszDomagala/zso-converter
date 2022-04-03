# int fun(int)

.globl fun_stub

.text

.code32
fun_stub:
pushl %ebp
movl %esp, %ebp

# save callee-saved registers
pushl %ebx
pushl %edi
pushl %esi
# align stack
subl $4, %esp

# change to 64 mode
subl $8, %esp
lea fun_stub_out, %ebx
movl $0x23, 4(%esp)
movl %ebx, (%esp)
lretl

/*
.code64
fun_stub_64:
movl $13, %eax


movq %rax, %rdx
shrq $32, %rdx

# change to 32 mode
lea fun_stub_out, %ebx
movl $0x23, 4(%rsp)
movl %ebx, (%rsp)
lretl
*/
.code32
fun_stub_out:
# argument as return value, dont call actual function yet
movl 8(%ebp), %eax

addl $4, %esp
popl %esi
popl %edi
popl %ebx
popl %ebp
retl
/*
.code32
fun_stub_out1:
movl 8(%ebp), %eax

addl $4, %esp
popl %esi
popl %edi
popl %ebx
popl %ebp
retl

fun_addr:
.long fun_stub_out1
.long 0x23
*/
