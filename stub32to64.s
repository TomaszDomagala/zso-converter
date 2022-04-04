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
# subl $4, %esp

# change to 64 mode
lea fun_stub_out, %ebx
# this $8 will be consumed by lretl instruction
subl $8, %esp
movl $0x33, 4(%esp)
movl %ebx, (%esp)
lretl

.code64
fun_stub_64:
movl 8(%ebp), %edi
movl 12(%ebp), %esi
movl 16(%ebp), %ebx
movl 20(%ebp), %ecx
movl 24(%ebp), %r8d
movl 28(%ebp), %r9d

call myfunc


movq %rax, %rdx
shrq $32, %rdx

# change to 32 mode
lea fun_stub_out, %ebx
subl $8, %esp
movl $0x23, 4(%rsp)
movl %ebx, (%rsp)
lretl
.code32
fun_stub_out:

# addl $4, %esp
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


