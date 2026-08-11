.data
.globl global
global:
  .word 3
.text
.globl main
main:
  addi sp, sp, -32
  sw ra, 28(sp)
  sw s10, 24(sp)
  sw s11, 20(sp)
  sw s8, 16(sp)
  sw s9, 12(sp)
  li t0, 0
  mv s11, t0
  sw s11, 0(sp)
  li t0, 0
  mv s11, t0
  sw s11, 4(sp)
.L4:
  lw t0, 4(sp)
  mv s11, t0
  li t0, 10
  mv s10, t0
  slt t0, s11, s10
  mv s9, t0
  beqz s9, .L5
  lw t0, 4(sp)
  mv s9, t0
  li t0, 1
  mv s10, t0
  add t0, s9, s10
  mv s11, t0
  sw s11, 4(sp)
  lw t0, 4(sp)
  mv s11, t0
  li t0, 2
  mv s10, t0
  rem t0, s11, s10
  mv s9, t0
  li t0, 0
  mv s10, t0
  sub t0, s9, s10
  seqz t0, t0
  mv s11, t0
  beqz s11, .L17
  j .L4
.L17:
  lw t0, 4(sp)
  mv s11, t0
  li t0, 9
  mv s10, t0
  sub t0, s11, s10
  seqz t0, t0
  mv s9, t0
  beqz s9, .L22
  j .L5
.L22:
  lw t0, 0(sp)
  mv s9, t0
  lw t0, 4(sp)
  mv s10, t0
  add t0, s9, s10
  mv s11, t0
  sw s11, 0(sp)
  j .L4
.L5:
  la t1, global
  lw t0, 0(t1)
  mv s11, t0
  li t0, 3
  mv s10, t0
  sub t0, s11, s10
  seqz t0, t0
  mv s9, t0
  li t0, 0
  mv s10, t0
  beqz s9, .L32
  lw t0, 0(sp)
  mv s9, t0
  li t0, 16
  mv s11, t0
  sub t0, s9, s11
  seqz t0, t0
  mv s8, t0
  seqz t0, s8
  mv s10, t0
  j .L31
.L32:
.L31:
  li t0, 1
  mv s8, t0
  beqz s10, .L38
  j .L37
  li t0, 0
  mv s10, t0
  seqz t0, s10
  mv s8, t0
  j .L37
.L38:
.L37:
  mv t0, s8
  mv a0, t0
  lw s10, 24(sp)
  lw s11, 20(sp)
  lw s8, 16(sp)
  lw s9, 12(sp)
  lw ra, 28(sp)
  addi sp, sp, 32
  ret
