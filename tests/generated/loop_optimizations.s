.data
.globl total
total:
  .word 3
.text
.globl main
read_total:
  addi sp, sp, -16
  la t1, total
  lw t0, 0(t1)
  sw t0, 0(sp)
  lw t1, 0(sp)
  mv a0, t1
  addi sp, sp, 16
  ret
set_total:
  addi sp, sp, -16
  sw a0, 0(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 4(sp)
  lw t0, 4(sp)
  la t1, total
  sw t0, 0(t1)
  la t1, total
  lw t0, 0(t1)
  sw t0, 8(sp)
  lw t1, 8(sp)
  mv a0, t1
  addi sp, sp, 16
  ret
kernel:
  addi sp, sp, -96
  sw ra, 92(sp)
  sw a0, 0(sp)
  sw a1, 4(sp)
  sw zero, 8(sp)
.L8:
  lw t1, 8(sp)
  mv t0, t1
  sw t0, 12(sp)
  lw t1, 12(sp)
  slti t0, t1, 20
  sw t0, 16(sp)
  lw t0, 16(sp)
  beqz t0, .L9
  la t1, total
  lw t0, 0(t1)
  sw t0, 20(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 24(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 28(sp)
  lw t1, 24(sp)
  lw t2, 28(sp)
  mul t0, t1, t2
  sw t0, 32(sp)
  lw t1, 20(sp)
  lw t2, 32(sp)
  add t0, t1, t2
  sw t0, 36(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 40(sp)
  lw t1, 40(sp)
  li t2, 3
  div t0, t1, t2
  sw t0, 44(sp)
  lw t1, 36(sp)
  lw t2, 44(sp)
  add t0, t1, t2
  sw t0, 48(sp)
  lw t0, 48(sp)
  la t1, total
  sw t0, 0(t1)
  lw t1, 8(sp)
  mv t0, t1
  sw t0, 52(sp)
  lw t1, 52(sp)
  addi t0, t1, 1
  sw t0, 56(sp)
  lw t0, 56(sp)
  sw t0, 8(sp)
  j .L8
.L9:
  call read_total
  sw a0, 60(sp)
  lw t0, 60(sp)
  sw t0, 64(sp)
  lw t1, 64(sp)
  mv t0, t1
  sw t0, 68(sp)
  lw t1, 68(sp)
  addi t0, t1, 1
  sw t0, 72(sp)
  lw a0, 72(sp)
  call set_total
  sw a0, 76(sp)
  la t1, total
  lw t0, 0(t1)
  sw t0, 80(sp)
  lw t1, 80(sp)
  mv a0, t1
  lw ra, 92(sp)
  addi sp, sp, 96
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  li a0, 12
  li a1, 5
  call kernel
  sw a0, 0(sp)
  lw t1, 0(sp)
  li t2, 1284
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 4(sp)
  lw t1, 4(sp)
  mv a0, t1
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
