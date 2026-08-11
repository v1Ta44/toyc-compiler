.text
.globl main
identity:
  addi sp, sp, -16
  sw a0, 0(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 4(sp)
  lw t1, 4(sp)
  mv a0, t1
  addi sp, sp, 16
  ret
dot3:
  addi sp, sp, -80
  sw a0, 0(sp)
  sw a1, 4(sp)
  sw a2, 8(sp)
  sw a3, 12(sp)
  sw a4, 16(sp)
  sw a5, 20(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 24(sp)
  lw t1, 12(sp)
  mv t0, t1
  sw t0, 28(sp)
  lw t1, 24(sp)
  lw t2, 28(sp)
  mul t0, t1, t2
  sw t0, 32(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 36(sp)
  lw t1, 16(sp)
  mv t0, t1
  sw t0, 40(sp)
  lw t1, 36(sp)
  lw t2, 40(sp)
  mul t0, t1, t2
  sw t0, 44(sp)
  lw t1, 32(sp)
  lw t2, 44(sp)
  add t0, t1, t2
  sw t0, 48(sp)
  lw t1, 8(sp)
  mv t0, t1
  sw t0, 52(sp)
  lw t1, 20(sp)
  mv t0, t1
  sw t0, 56(sp)
  lw t1, 52(sp)
  lw t2, 56(sp)
  mul t0, t1, t2
  sw t0, 60(sp)
  lw t1, 48(sp)
  lw t2, 60(sp)
  add t0, t1, t2
  sw t0, 64(sp)
  lw t1, 64(sp)
  mv a0, t1
  addi sp, sp, 80
  ret
main:
  addi sp, sp, -64
  sw ra, 60(sp)
  li a0, 1
  call identity
  sw a0, 0(sp)
  lw t0, 0(sp)
  sw t0, 4(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 8(sp)
  lw a0, 8(sp)
  li a1, 2
  li a2, 3
  li a3, 7
  li a4, 8
  li a5, 9
  call dot3
  sw a0, 12(sp)
  lw t0, 12(sp)
  sw t0, 16(sp)
  li a0, 4
  li a1, 5
  li a2, 6
  li a3, 7
  li a4, 8
  li a5, 9
  call dot3
  sw a0, 20(sp)
  lw t0, 20(sp)
  sw t0, 24(sp)
  li a0, 7
  li a1, 8
  li a2, 9
  li a3, 7
  li a4, 8
  li a5, 9
  call dot3
  sw a0, 28(sp)
  lw t0, 28(sp)
  sw t0, 32(sp)
  lw t1, 16(sp)
  mv t0, t1
  sw t0, 36(sp)
  lw t1, 24(sp)
  mv t0, t1
  sw t0, 40(sp)
  lw t1, 36(sp)
  lw t2, 40(sp)
  add t0, t1, t2
  sw t0, 44(sp)
  lw t1, 32(sp)
  mv t0, t1
  sw t0, 48(sp)
  lw t1, 44(sp)
  lw t2, 48(sp)
  add t0, t1, t2
  sw t0, 52(sp)
  lw t1, 52(sp)
  mv a0, t1
  lw ra, 60(sp)
  addi sp, sp, 64
  ret
