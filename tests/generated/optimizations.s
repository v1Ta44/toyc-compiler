.text
.globl main
identity:
  addi sp, sp, -176
  sw ra, 172(sp)
  sw a0, 0(sp)
  lw t0, 0(sp)
  sw t0, 4(sp)
  lw t1, 4(sp)
  mv t0, t1
  mv a0, t0
  lw ra, 172(sp)
  addi sp, sp, 176
  ret
main:
  addi sp, sp, -176
  sw ra, 172(sp)
  li t0, 4
  sw t0, 8(sp)
  lw t0, 8(sp)
  sw t0, 12(sp)
  li t0, 7
  sw t0, 16(sp)
  lw t0, 16(sp)
  sw t0, 20(sp)
  lw t0, 12(sp)
  sw t0, 24(sp)
  lw t0, 20(sp)
  sw t0, 28(sp)
  lw t1, 24(sp)
  lw t2, 28(sp)
  add t0, t1, t2
  sw t0, 32(sp)
  lw t0, 12(sp)
  sw t0, 36(sp)
  lw t0, 20(sp)
  sw t0, 40(sp)
  lw t1, 36(sp)
  lw t2, 40(sp)
  add t0, t1, t2
  sw t0, 44(sp)
  lw t1, 32(sp)
  lw t2, 44(sp)
  mul t0, t1, t2
  sw t0, 48(sp)
  lw t0, 48(sp)
  sw t0, 52(sp)
  lw t0, 12(sp)
  sw t0, 56(sp)
  lw t0, 20(sp)
  sw t0, 60(sp)
  lw t1, 56(sp)
  lw t2, 60(sp)
  add t0, t1, t2
  sw t0, 64(sp)
  lw t0, 12(sp)
  sw t0, 68(sp)
  lw t0, 20(sp)
  sw t0, 72(sp)
  lw t1, 68(sp)
  lw t2, 72(sp)
  add t0, t1, t2
  sw t0, 76(sp)
  lw t1, 64(sp)
  lw t2, 76(sp)
  mul t0, t1, t2
  sw t0, 80(sp)
  lw t0, 80(sp)
  sw t0, 84(sp)
  lw t0, 12(sp)
  sw t0, 88(sp)
  li t0, 1
  sw t0, 92(sp)
  lw t1, 88(sp)
  lw t2, 92(sp)
  add t0, t1, t2
  sw t0, 96(sp)
  lw t0, 96(sp)
  sw t0, 12(sp)
  lw t0, 12(sp)
  sw t0, 100(sp)
  lw t0, 20(sp)
  sw t0, 104(sp)
  lw t1, 100(sp)
  lw t2, 104(sp)
  add t0, t1, t2
  sw t0, 108(sp)
  lw t0, 12(sp)
  sw t0, 112(sp)
  lw t0, 20(sp)
  sw t0, 116(sp)
  lw t1, 112(sp)
  lw t2, 116(sp)
  add t0, t1, t2
  sw t0, 120(sp)
  lw t1, 108(sp)
  lw t2, 120(sp)
  mul t0, t1, t2
  sw t0, 124(sp)
  lw t0, 124(sp)
  sw t0, 128(sp)
  lw t0, 52(sp)
  sw t0, 132(sp)
  lw t0, 84(sp)
  sw t0, 136(sp)
  lw t1, 132(sp)
  lw t2, 136(sp)
  add t0, t1, t2
  sw t0, 140(sp)
  lw t0, 128(sp)
  sw t0, 144(sp)
  lw t0, 144(sp)
  mv a0, t0
  call identity
  sw a0, 148(sp)
  lw t1, 140(sp)
  lw t2, 148(sp)
  add t0, t1, t2
  sw t0, 152(sp)
  lw t0, 152(sp)
  sw t0, 156(sp)
  lw t0, 156(sp)
  sw t0, 160(sp)
  li t0, 386
  sw t0, 164(sp)
  lw t1, 160(sp)
  lw t2, 164(sp)
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 168(sp)
  lw t1, 168(sp)
  mv t0, t1
  mv a0, t0
  lw ra, 172(sp)
  addi sp, sp, 176
  ret
