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
main:
  addi sp, sp, -160
  sw ra, 156(sp)
  li t0, 4
  sw t0, 0(sp)
  li t0, 7
  sw t0, 4(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 8(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 12(sp)
  lw t1, 8(sp)
  lw t2, 12(sp)
  add t0, t1, t2
  sw t0, 16(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 20(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 24(sp)
  lw t1, 20(sp)
  lw t2, 24(sp)
  add t0, t1, t2
  sw t0, 28(sp)
  lw t1, 16(sp)
  lw t2, 28(sp)
  mul t0, t1, t2
  sw t0, 32(sp)
  lw t0, 32(sp)
  sw t0, 36(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 40(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 44(sp)
  lw t1, 40(sp)
  lw t2, 44(sp)
  add t0, t1, t2
  sw t0, 48(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 52(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 56(sp)
  lw t1, 52(sp)
  lw t2, 56(sp)
  add t0, t1, t2
  sw t0, 60(sp)
  lw t1, 48(sp)
  lw t2, 60(sp)
  mul t0, t1, t2
  sw t0, 64(sp)
  lw t0, 64(sp)
  sw t0, 68(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 72(sp)
  lw t1, 72(sp)
  addi t0, t1, 1
  sw t0, 76(sp)
  lw t0, 76(sp)
  sw t0, 0(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 80(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 84(sp)
  lw t1, 80(sp)
  lw t2, 84(sp)
  add t0, t1, t2
  sw t0, 88(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 92(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 96(sp)
  lw t1, 92(sp)
  lw t2, 96(sp)
  add t0, t1, t2
  sw t0, 100(sp)
  lw t1, 88(sp)
  lw t2, 100(sp)
  mul t0, t1, t2
  sw t0, 104(sp)
  lw t0, 104(sp)
  sw t0, 108(sp)
  lw t1, 36(sp)
  mv t0, t1
  sw t0, 112(sp)
  lw t1, 68(sp)
  mv t0, t1
  sw t0, 116(sp)
  lw t1, 112(sp)
  lw t2, 116(sp)
  add t0, t1, t2
  sw t0, 120(sp)
  lw t1, 108(sp)
  mv t0, t1
  sw t0, 124(sp)
  lw t0, 124(sp)
  mv a0, t0
  call identity
  sw a0, 128(sp)
  lw t1, 120(sp)
  lw t2, 128(sp)
  add t0, t1, t2
  sw t0, 132(sp)
  lw t0, 132(sp)
  sw t0, 136(sp)
  lw t1, 136(sp)
  mv t0, t1
  sw t0, 140(sp)
  lw t1, 140(sp)
  li t2, 386
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 144(sp)
  lw t1, 144(sp)
  mv a0, t1
  lw ra, 156(sp)
  addi sp, sp, 160
  ret
