.text
.globl main
factorial:
  addi sp, sp, -48
  sw ra, 44(sp)
  sw a0, 0(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 4(sp)
  lw t1, 4(sp)
  li t2, 1
  slt t0, t2, t1
  xori t0, t0, 1
  sw t0, 8(sp)
  lw t0, 8(sp)
  beqz t0, .L4
  li a0, 1
  lw ra, 44(sp)
  addi sp, sp, 48
  ret
.L4:
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 12(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 16(sp)
  lw t1, 16(sp)
  addi t0, t1, -1
  sw t0, 20(sp)
  lw a0, 20(sp)
  call factorial
  sw a0, 24(sp)
  lw t1, 12(sp)
  lw t2, 24(sp)
  mul t0, t1, t2
  sw t0, 28(sp)
  lw t1, 28(sp)
  mv a0, t1
  lw ra, 44(sp)
  addi sp, sp, 48
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  li a0, 5
  call factorial
  sw a0, 0(sp)
  lw t0, 0(sp)
  sw t0, 4(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 8(sp)
  lw t1, 8(sp)
  mv a0, t1
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
