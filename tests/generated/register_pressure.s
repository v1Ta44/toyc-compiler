.text
.globl main
sum12:
  addi sp, sp, -144
  sw a0, 0(sp)
  sw a1, 4(sp)
  sw a2, 8(sp)
  sw a3, 12(sp)
  sw a4, 16(sp)
  sw a5, 20(sp)
  sw a6, 24(sp)
  sw a7, 28(sp)
  lw t0, 144(sp)
  sw t0, 32(sp)
  lw t0, 148(sp)
  sw t0, 36(sp)
  lw t0, 152(sp)
  sw t0, 40(sp)
  lw t0, 156(sp)
  sw t0, 44(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 48(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 52(sp)
  lw t1, 48(sp)
  lw t2, 52(sp)
  add t0, t1, t2
  sw t0, 56(sp)
  lw t1, 8(sp)
  mv t0, t1
  sw t0, 60(sp)
  lw t1, 56(sp)
  lw t2, 60(sp)
  add t0, t1, t2
  sw t0, 64(sp)
  lw t1, 12(sp)
  mv t0, t1
  sw t0, 68(sp)
  lw t1, 64(sp)
  lw t2, 68(sp)
  add t0, t1, t2
  sw t0, 72(sp)
  lw t1, 16(sp)
  mv t0, t1
  sw t0, 76(sp)
  lw t1, 72(sp)
  lw t2, 76(sp)
  add t0, t1, t2
  sw t0, 80(sp)
  lw t1, 20(sp)
  mv t0, t1
  sw t0, 84(sp)
  lw t1, 80(sp)
  lw t2, 84(sp)
  add t0, t1, t2
  sw t0, 88(sp)
  lw t1, 24(sp)
  mv t0, t1
  sw t0, 92(sp)
  lw t1, 88(sp)
  lw t2, 92(sp)
  add t0, t1, t2
  sw t0, 96(sp)
  lw t1, 28(sp)
  mv t0, t1
  sw t0, 100(sp)
  lw t1, 96(sp)
  lw t2, 100(sp)
  add t0, t1, t2
  sw t0, 104(sp)
  lw t1, 32(sp)
  mv t0, t1
  sw t0, 108(sp)
  lw t1, 104(sp)
  lw t2, 108(sp)
  add t0, t1, t2
  sw t0, 112(sp)
  lw t1, 36(sp)
  mv t0, t1
  sw t0, 116(sp)
  lw t1, 112(sp)
  lw t2, 116(sp)
  add t0, t1, t2
  sw t0, 120(sp)
  lw t1, 40(sp)
  mv t0, t1
  sw t0, 124(sp)
  lw t1, 120(sp)
  lw t2, 124(sp)
  add t0, t1, t2
  sw t0, 128(sp)
  lw t1, 44(sp)
  mv t0, t1
  sw t0, 132(sp)
  lw t1, 128(sp)
  lw t2, 132(sp)
  add t0, t1, t2
  sw t0, 136(sp)
  lw t1, 136(sp)
  mv a0, t1
  addi sp, sp, 144
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  li t0, 1
  mv a0, t0
  li t0, 2
  mv a1, t0
  li t0, 3
  mv a2, t0
  li t0, 4
  mv a3, t0
  li t0, 5
  mv a4, t0
  li t0, 6
  mv a5, t0
  li t0, 7
  mv a6, t0
  li t0, 8
  mv a7, t0
  li t0, 9
  sw t0, -16(sp)
  li t0, 10
  sw t0, -12(sp)
  li t0, 11
  sw t0, -8(sp)
  li t0, 12
  sw t0, -4(sp)
  addi sp, sp, -16
  call sum12
  addi sp, sp, 16
  sw a0, 0(sp)
  lw t1, 0(sp)
  mv a0, t1
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
