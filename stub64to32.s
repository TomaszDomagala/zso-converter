# int fun(int)

.globl fun_stub

.text
.code64

fun_stub:
pushq %rbx
pushq %rbp
pushq %r12
pushq %r13
pushq %r14
pushq %r15

subq $8, %rsp
movl %edi, (%rsp)

# change to 32 mode
lea fun_stub_out, %ebx
subl $8, %esp
movl $0x23, 4(%rsp)
movl %ebx, (%rsp)
lretl

.code32
fun_stub_32:
pushl $0x2b
popl %ds
pushl $0x2b
popl %es

call myfunc

# change to 64 mode
lea fun_stub_out, %ebx
# this $8 will be consumed by lretl instruction
subl $8, %esp
movl $0x33, 4(%esp)
movl %ebx, (%esp)
lretl

.code64
fun_stub_out:
# mov %eax, %eax
# shlq $32, %rdx
# orq %rdx, %rax
addq $8, %rsp
popq %r15
popq %r14
popq %r13
popq %r12
popq %rbp
popq %rbx
retq
