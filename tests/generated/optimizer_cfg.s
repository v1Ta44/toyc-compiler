.text
.globl main
choose:
  addi sp, sp, -16
  sw a0, 0(sp)
  li t0, 10
  sw t0, 4(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 8(sp)
  lw t0, 8(sp)
  beqz t0, .L4
  li t0, 20
  sw t0, 4(sp)
  j .L5
.L4:
  li t0, 30
  sw t0, 4(sp)
.L5:
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 12(sp)
  lw t1, 12(sp)
  mv a0, t1
  addi sp, sp, 16
  ret
main:
  addi sp, sp, -128
  sw ra, 124(sp)
  sw zero, 0(sp)
  li t0, 1
  sw t0, 4(sp)
.L13:
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 8(sp)
  lw t1, 8(sp)
  slti t0, t1, 5
  sw t0, 12(sp)
  lw t0, 12(sp)
  beqz t0, .L14
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 16(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 20(sp)
  lw t1, 16(sp)
  lw t2, 20(sp)
  add t0, t1, t2
  sw t0, 24(sp)
  lw t0, 24(sp)
  sw t0, 4(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 28(sp)
  lw t1, 28(sp)
  addi t0, t1, 1
  sw t0, 32(sp)
  lw t0, 32(sp)
  sw t0, 0(sp)
  j .L13
.L14:
  li t0, -4
  sw t0, 36(sp)
  li t0, -1
  sw t0, 40(sp)
  li t0, -4
  sw t0, 44(sp)
  sw zero, 48(sp)
  lw t1, 48(sp)
  mv t0, t1
  sw t0, 52(sp)
  lw t2, 52(sp)
  seqz t0, t2
  sw t0, 56(sp)
  lw t0, 56(sp)
  sw t0, 60(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 64(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 68(sp)
  lw t1, 68(sp)
  li t2, 5
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 72(sp)
  lw a0, 72(sp)
  call choose
  sw a0, 76(sp)
  lw t1, 64(sp)
  lw t2, 76(sp)
  add t0, t1, t2
  sw t0, 80(sp)
  lw t1, 36(sp)
  mv t0, t1
  sw t0, 84(sp)
  lw t1, 80(sp)
  lw t2, 84(sp)
  add t0, t1, t2
  sw t0, 88(sp)
  lw t1, 40(sp)
  mv t0, t1
  sw t0, 92(sp)
  lw t1, 88(sp)
  lw t2, 92(sp)
  add t0, t1, t2
  sw t0, 96(sp)
  lw t1, 44(sp)
  mv t0, t1
  sw t0, 100(sp)
  lw t1, 96(sp)
  lw t2, 100(sp)
  add t0, t1, t2
  sw t0, 104(sp)
  lw t1, 60(sp)
  mv t0, t1
  sw t0, 108(sp)
  lw t1, 104(sp)
  lw t2, 108(sp)
  add t0, t1, t2
  sw t0, 112(sp)
  lw t1, 112(sp)
  mv a0, t1
  lw ra, 124(sp)
  addi sp, sp, 128
  ret
