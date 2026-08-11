.text
.globl main
main:
  addi sp, sp, -48
  sw zero, 0(sp)
  sw zero, 4(sp)
.L4:
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 8(sp)
  lw t1, 8(sp)
  slti t0, t1, 100
  sw t0, 12(sp)
  lw t0, 12(sp)
  beqz t0, .L5
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 16(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 20(sp)
  lw t1, 20(sp)
  lw t1, 20(sp)
  li t2, 10
  mul t0, t1, t2
  sw t0, 24(sp)
  lw t1, 16(sp)
  lw t2, 24(sp)
  add t0, t1, t2
  sw t0, 28(sp)
  lw t0, 28(sp)
  sw t0, 4(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 32(sp)
  lw t1, 32(sp)
  addi t0, t1, 1
  sw t0, 36(sp)
  lw t0, 36(sp)
  sw t0, 0(sp)
  j .L4
.L5:
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 40(sp)
  lw t1, 40(sp)
  li t2, 49500
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 44(sp)
  lw t1, 44(sp)
  mv a0, t1
  addi sp, sp, 48
  ret
