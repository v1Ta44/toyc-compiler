.text
.globl main
sum12:
  addi sp, sp, -208
  sw ra, 204(sp)
  sw a0, 0(sp)
  sw a1, 4(sp)
  sw a2, 8(sp)
  sw a3, 12(sp)
  sw a4, 16(sp)
  sw a5, 20(sp)
  sw a6, 24(sp)
  sw a7, 28(sp)
  lw t0, 208(sp)
  sw t0, 32(sp)
  lw t0, 212(sp)
  sw t0, 36(sp)
  lw t0, 216(sp)
  sw t0, 40(sp)
  lw t0, 220(sp)
  sw t0, 44(sp)
  lw t0, 0(sp)
  sw t0, 48(sp)
  lw t0, 4(sp)
  sw t0, 52(sp)
  lw t1, 48(sp)
  lw t2, 52(sp)
  add t0, t1, t2
  sw t0, 56(sp)
  lw t0, 8(sp)
  sw t0, 60(sp)
  lw t1, 56(sp)
  lw t2, 60(sp)
  add t0, t1, t2
  sw t0, 64(sp)
  lw t0, 12(sp)
  sw t0, 68(sp)
  lw t1, 64(sp)
  lw t2, 68(sp)
  add t0, t1, t2
  sw t0, 72(sp)
  lw t0, 16(sp)
  sw t0, 76(sp)
  lw t1, 72(sp)
  lw t2, 76(sp)
  add t0, t1, t2
  sw t0, 80(sp)
  lw t0, 20(sp)
  sw t0, 84(sp)
  lw t1, 80(sp)
  lw t2, 84(sp)
  add t0, t1, t2
  sw t0, 88(sp)
  lw t0, 24(sp)
  sw t0, 92(sp)
  lw t1, 88(sp)
  lw t2, 92(sp)
  add t0, t1, t2
  sw t0, 96(sp)
  lw t0, 28(sp)
  sw t0, 100(sp)
  lw t1, 96(sp)
  lw t2, 100(sp)
  add t0, t1, t2
  sw t0, 104(sp)
  lw t0, 32(sp)
  sw t0, 108(sp)
  lw t1, 104(sp)
  lw t2, 108(sp)
  add t0, t1, t2
  sw t0, 112(sp)
  lw t0, 36(sp)
  sw t0, 116(sp)
  lw t1, 112(sp)
  lw t2, 116(sp)
  add t0, t1, t2
  sw t0, 120(sp)
  lw t0, 40(sp)
  sw t0, 124(sp)
  lw t1, 120(sp)
  lw t2, 124(sp)
  add t0, t1, t2
  sw t0, 128(sp)
  lw t0, 44(sp)
  sw t0, 132(sp)
  lw t1, 128(sp)
  lw t2, 132(sp)
  add t0, t1, t2
  sw t0, 136(sp)
  lw t1, 136(sp)
  mv t0, t1
  mv a0, t0
  lw ra, 204(sp)
  addi sp, sp, 208
  ret
main:
  addi sp, sp, -208
  sw ra, 204(sp)
  li t0, 1
  sw t0, 140(sp)
  li t0, 2
  sw t0, 144(sp)
  li t0, 3
  sw t0, 148(sp)
  li t0, 4
  sw t0, 152(sp)
  li t0, 5
  sw t0, 156(sp)
  li t0, 6
  sw t0, 160(sp)
  li t0, 7
  sw t0, 164(sp)
  li t0, 8
  sw t0, 168(sp)
  li t0, 9
  sw t0, 172(sp)
  li t0, 10
  sw t0, 176(sp)
  li t0, 11
  sw t0, 180(sp)
  li t0, 12
  sw t0, 184(sp)
  lw t0, 140(sp)
  mv a0, t0
  lw t0, 144(sp)
  mv a1, t0
  lw t0, 148(sp)
  mv a2, t0
  lw t0, 152(sp)
  mv a3, t0
  lw t0, 156(sp)
  mv a4, t0
  lw t0, 160(sp)
  mv a5, t0
  lw t0, 164(sp)
  mv a6, t0
  lw t0, 168(sp)
  mv a7, t0
  lw t0, 172(sp)
  sw t0, -16(sp)
  lw t0, 176(sp)
  sw t0, -12(sp)
  lw t0, 180(sp)
  sw t0, -8(sp)
  lw t0, 184(sp)
  sw t0, -4(sp)
  addi sp, sp, -16
  call sum12
  addi sp, sp, 16
  sw a0, 188(sp)
  lw t1, 188(sp)
  mv t0, t1
  mv a0, t0
  lw ra, 204(sp)
  addi sp, sp, 208
  ret
