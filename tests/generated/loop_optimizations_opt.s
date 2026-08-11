.data
.globl total
total:
  .word 3
.text
.globl main
read_total:
  la t1, total
  lw t4, 0(t1)
  mv a0, t4
  ret
set_total:
  mv t4, a0
  mv t5, t4
  la t1, total
  sw t4, 0(t1)
  mv a0, t5
  ret
kernel:
  addi sp, sp, -16
  sw ra, 12(sp)
  sw s1, 8(sp)
  li t5, 20
  la t1, total
  lw s1, 0(t1)
  li t4, 0
  mul t6, a0, a1
  li t2, 3
  div a1, a0, t2
.L7:
  bge t4, t5, .L8
  add a0, s1, t6
  add s1, a0, a1
  addi t4, t4, 1
  j .L7
.L8:
  la t1, total
  sw s1, 0(t1)
  call read_total
  mv t4, a0
  addi t4, t4, 1
  la t1, total
  sw s1, 0(t1)
  mv a0, t4
  call set_total
  mv t4, a0
  la t1, total
  lw s1, 0(t1)
  mv t4, s1
  la t1, total
  sw s1, 0(t1)
  mv a0, t4
  lw s1, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  li a0, 12
  li a1, 5
  call kernel
  mv t4, a0
  li t2, 1284
  sub a0, t4, t2
  seqz a0, a0
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
