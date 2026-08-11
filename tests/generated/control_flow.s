.data
.globl global
global:
  .word 3
.text
.globl main
main:
  addi sp, sp, -80
  li t0, 0
  sw t0, 0(sp)
  li t0, 0
  sw t0, 4(sp)
.L4:
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 8(sp)
  lw t1, 8(sp)
  li t2, 10
  slt t0, t1, t2
  sw t0, 12(sp)
  lw t0, 12(sp)
  beqz t0, .L5
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 16(sp)
  lw t1, 16(sp)
  addi t0, t1, 1
  sw t0, 20(sp)
  lw t0, 20(sp)
  sw t0, 4(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 24(sp)
  lw t1, 24(sp)
  li t2, 2
  rem t0, t1, t2
  sw t0, 28(sp)
  lw t1, 28(sp)
  li t2, 0
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 32(sp)
  lw t0, 32(sp)
  beqz t0, .L17
  j .L4
.L17:
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 36(sp)
  lw t1, 36(sp)
  li t2, 9
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 40(sp)
  lw t0, 40(sp)
  beqz t0, .L22
  j .L5
.L22:
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 44(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 48(sp)
  lw t1, 44(sp)
  lw t2, 48(sp)
  add t0, t1, t2
  sw t0, 52(sp)
  lw t0, 52(sp)
  sw t0, 0(sp)
  j .L4
.L5:
  la t1, global
  lw t0, 0(t1)
  sw t0, 56(sp)
  lw t1, 56(sp)
  li t2, 3
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 60(sp)
  li t0, 0
  sw t0, 64(sp)
  lw t0, 60(sp)
  beqz t0, .L31
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 68(sp)
  lw t1, 68(sp)
  li t2, 16
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 72(sp)
  lw t2, 72(sp)
  snez t0, t2
  sw t0, 64(sp)
.L31:
  li t0, 1
  sw t0, 76(sp)
  lw t0, 64(sp)
  beqz t0, .L37
  j .L36
.L37:
  li t2, 0
  snez t0, t2
  sw t0, 76(sp)
.L36:
  lw t1, 76(sp)
  mv a0, t1
  addi sp, sp, 80
  ret
