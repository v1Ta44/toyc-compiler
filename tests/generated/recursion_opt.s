.text
.globl main
factorial:
  addi sp, sp, -80
  sw ra, 76(sp)
  sw a0, 0(sp)
  lw t0, 0(sp)
  sw t0, 4(sp)
  li t0, 1
  sw t0, 8(sp)
  lw t1, 4(sp)
  lw t2, 8(sp)
  slt t0, t2, t1
  xori t0, t0, 1
  sw t0, 12(sp)
  lw t0, 12(sp)
  beqz t0, .L4
  li t0, 1
  mv a0, t0
  lw ra, 76(sp)
  addi sp, sp, 80
  ret
.L4:
  lw t0, 0(sp)
  sw t0, 16(sp)
  lw t0, 0(sp)
  sw t0, 20(sp)
  li t0, 1
  sw t0, 24(sp)
  lw t1, 20(sp)
  lw t2, 24(sp)
  sub t0, t1, t2
  sw t0, 28(sp)
  lw t0, 28(sp)
  mv a0, t0
  call factorial
  sw a0, 32(sp)
  lw t1, 16(sp)
  lw t2, 32(sp)
  mul t0, t1, t2
  sw t0, 40(sp)
  lw t1, 40(sp)
  mv t0, t1
  mv a0, t0
  lw ra, 76(sp)
  addi sp, sp, 80
  ret
main:
  addi sp, sp, -80
  sw ra, 76(sp)
  li t0, 5
  sw t0, 44(sp)
  lw t0, 44(sp)
  mv a0, t0
  call factorial
  sw a0, 48(sp)
  lw t0, 48(sp)
  sw t0, 56(sp)
  lw t0, 56(sp)
  sw t0, 60(sp)
  lw t1, 60(sp)
  mv t0, t1
  mv a0, t0
  lw ra, 76(sp)
  addi sp, sp, 80
  ret
