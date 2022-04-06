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
movq 8(%rbp), %rdi
movq 12(%rbp), %rsi
movq 16(%rbp), %rdx
movq 20(%rbp), %rcx
movq 24(%rbp), %r8
movq 28(%rbp), %r9

movl 8(%rbp), %edi
movl 12(%rbp), %esi
movl 16(%rbp), %edx
movl 20(%rbp), %ecx
movl 24(%rbp), %r8d
movl 28(%rbp), %r9d


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


