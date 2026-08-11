.text
.globl main
sum12:
  addi sp, sp, -112
  sw ra, 108(sp)
  sw s1, 104(sp)
  sw s10, 100(sp)
  sw s11, 96(sp)
  sw s2, 92(sp)
  sw s3, 88(sp)
  sw s4, 84(sp)
  sw s5, 80(sp)
  sw s6, 76(sp)
  sw s7, 72(sp)
  sw s8, 68(sp)
  sw s9, 64(sp)
  sw a0, 0(sp)
  sw a1, 4(sp)
  sw a2, 8(sp)
  sw a3, 12(sp)
  sw a4, 16(sp)
  sw a5, 20(sp)
  sw a6, 24(sp)
  sw a7, 28(sp)
  lw t0, 112(sp)
  sw t0, 32(sp)
  lw t0, 116(sp)
  sw t0, 36(sp)
  lw t0, 120(sp)
  sw t0, 40(sp)
  lw t0, 124(sp)
  sw t0, 44(sp)
  lw t0, 0(sp)
  mv s11, t0
  lw t0, 4(sp)
  mv s10, t0
  add t0, s11, s10
  mv s9, t0
  lw t0, 8(sp)
  mv s10, t0
  add t0, s9, s10
  mv s11, t0
  lw t0, 12(sp)
  mv s10, t0
  add t0, s11, s10
  mv s9, t0
  lw t0, 16(sp)
  mv s10, t0
  add t0, s9, s10
  mv s11, t0
  lw t0, 20(sp)
  mv s10, t0
  add t0, s11, s10
  mv s9, t0
  lw t0, 24(sp)
  mv s10, t0
  add t0, s9, s10
  mv s11, t0
  lw t0, 28(sp)
  mv s10, t0
  add t0, s11, s10
  mv s9, t0
  lw t0, 32(sp)
  mv s10, t0
  add t0, s9, s10
  mv s11, t0
  lw t0, 36(sp)
  mv s10, t0
  add t0, s11, s10
  mv s9, t0
  lw t0, 40(sp)
  mv s10, t0
  add t0, s9, s10
  mv s11, t0
  lw t0, 44(sp)
  mv s10, t0
  add t0, s11, s10
  mv s9, t0
  mv t0, s9
  mv a0, t0
  lw s1, 104(sp)
  lw s10, 100(sp)
  lw s11, 96(sp)
  lw s2, 92(sp)
  lw s3, 88(sp)
  lw s4, 84(sp)
  lw s5, 80(sp)
  lw s6, 76(sp)
  lw s7, 72(sp)
  lw s8, 68(sp)
  lw s9, 64(sp)
  lw ra, 108(sp)
  addi sp, sp, 112
  ret
main:
  addi sp, sp, -112
  sw ra, 108(sp)
  sw s1, 104(sp)
  sw s10, 100(sp)
  sw s11, 96(sp)
  sw s2, 92(sp)
  sw s3, 88(sp)
  sw s4, 84(sp)
  sw s5, 80(sp)
  sw s6, 76(sp)
  sw s7, 72(sp)
  sw s8, 68(sp)
  sw s9, 64(sp)
  li t0, 1
  mv s9, t0
  li t0, 2
  mv s10, t0
  li t0, 3
  mv s11, t0
  li t0, 4
  mv s8, t0
  li t0, 5
  mv s7, t0
  li t0, 6
  mv s6, t0
  li t0, 7
  mv s5, t0
  li t0, 8
  mv s4, t0
  li t0, 9
  mv s3, t0
  li t0, 10
  mv s2, t0
  li t0, 11
  mv s1, t0
  li t0, 12
  sw t0, 48(sp)
  mv a0, s9
  mv a1, s10
  mv a2, s11
  mv a3, s8
  mv a4, s7
  mv a5, s6
  mv a6, s5
  mv a7, s4
  sw s3, -16(sp)
  sw s2, -12(sp)
  sw s1, -8(sp)
  lw t0, 48(sp)
  sw t0, -4(sp)
  addi sp, sp, -16
  call sum12
  addi sp, sp, 16
  sw a0, 52(sp)
  lw t1, 52(sp)
  mv t0, t1
  mv a0, t0
  lw s1, 104(sp)
  lw s10, 100(sp)
  lw s11, 96(sp)
  lw s2, 92(sp)
  lw s3, 88(sp)
  lw s4, 84(sp)
  lw s5, 80(sp)
  lw s6, 76(sp)
  lw s7, 72(sp)
  lw s8, 68(sp)
  lw s9, 64(sp)
  lw ra, 108(sp)
  addi sp, sp, 112
  ret
