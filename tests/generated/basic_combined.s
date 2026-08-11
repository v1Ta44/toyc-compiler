.data
.globl total
total:
  .word 0
.text
.globl main
sink:
  addi sp, sp, -128
  sw a0, 0(sp)
  sw a1, 4(sp)
  sw a2, 8(sp)
  sw a3, 12(sp)
  sw a4, 16(sp)
  sw a5, 20(sp)
  sw a6, 24(sp)
  sw a7, 28(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 32(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 36(sp)
  lw t1, 36(sp)
  slli t0, t1, 1
  sw t0, 40(sp)
  lw t1, 32(sp)
  lw t2, 40(sp)
  add t0, t1, t2
  sw t0, 44(sp)
  lw t1, 8(sp)
  mv t0, t1
  sw t0, 48(sp)
  lw t1, 48(sp)
  slli t2, t1, 1
  add t0, t1, t2
  sw t0, 52(sp)
  lw t1, 44(sp)
  lw t2, 52(sp)
  add t0, t1, t2
  sw t0, 56(sp)
  lw t1, 12(sp)
  mv t0, t1
  sw t0, 60(sp)
  lw t1, 60(sp)
  slli t0, t1, 2
  sw t0, 64(sp)
  lw t1, 56(sp)
  lw t2, 64(sp)
  add t0, t1, t2
  sw t0, 68(sp)
  lw t1, 16(sp)
  mv t0, t1
  sw t0, 72(sp)
  lw t1, 72(sp)
  slli t2, t1, 2
  add t0, t1, t2
  sw t0, 76(sp)
  lw t1, 68(sp)
  lw t2, 76(sp)
  add t0, t1, t2
  sw t0, 80(sp)
  lw t1, 20(sp)
  mv t0, t1
  sw t0, 84(sp)
  lw t1, 84(sp)
  lw t1, 84(sp)
  li t2, 6
  mul t0, t1, t2
  sw t0, 88(sp)
  lw t1, 80(sp)
  lw t2, 88(sp)
  add t0, t1, t2
  sw t0, 92(sp)
  lw t1, 24(sp)
  mv t0, t1
  sw t0, 96(sp)
  lw t1, 96(sp)
  slli t2, t1, 3
  sub t0, t2, t1
  sw t0, 100(sp)
  lw t1, 92(sp)
  lw t2, 100(sp)
  add t0, t1, t2
  sw t0, 104(sp)
  lw t1, 28(sp)
  mv t0, t1
  sw t0, 108(sp)
  lw t1, 108(sp)
  slli t0, t1, 3
  sw t0, 112(sp)
  lw t1, 104(sp)
  lw t2, 112(sp)
  add t0, t1, t2
  sw t0, 116(sp)
  lw t1, 116(sp)
  mv a0, t1
  addi sp, sp, 128
  ret
reverse_forward:
  addi sp, sp, -80
  sw ra, 76(sp)
  sw a0, 0(sp)
  sw a1, 4(sp)
  sw a2, 8(sp)
  sw a3, 12(sp)
  sw a4, 16(sp)
  sw a5, 20(sp)
  sw a6, 24(sp)
  sw a7, 28(sp)
  lw t1, 28(sp)
  mv t0, t1
  sw t0, 32(sp)
  lw t1, 24(sp)
  mv t0, t1
  sw t0, 36(sp)
  lw t1, 20(sp)
  mv t0, t1
  sw t0, 40(sp)
  lw t1, 16(sp)
  mv t0, t1
  sw t0, 44(sp)
  lw t1, 12(sp)
  mv t0, t1
  sw t0, 48(sp)
  lw t1, 8(sp)
  mv t0, t1
  sw t0, 52(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 56(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 60(sp)
  lw a0, 32(sp)
  lw a1, 36(sp)
  lw a2, 40(sp)
  lw a3, 44(sp)
  lw a4, 48(sp)
  lw a5, 52(sp)
  lw a6, 56(sp)
  lw a7, 60(sp)
  call sink
  sw a0, 64(sp)
  lw t1, 64(sp)
  mv a0, t1
  lw ra, 76(sp)
  addi sp, sp, 80
  ret
mark:
  addi sp, sp, -32
  sw a0, 0(sp)
  la t1, total
  lw t0, 0(t1)
  sw t0, 4(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 8(sp)
  lw t1, 4(sp)
  lw t2, 8(sp)
  add t0, t1, t2
  sw t0, 12(sp)
  lw t0, 12(sp)
  la t1, total
  sw t0, 0(t1)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 16(sp)
  lw t1, 16(sp)
  mv a0, t1
  addi sp, sp, 32
  ret
main:
  addi sp, sp, -64
  sw ra, 60(sp)
  li t0, 0
  sw t0, 0(sp)
.L61:
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 4(sp)
  lw t1, 4(sp)
  slti t0, t1, 4
  sw t0, 8(sp)
  lw t0, 8(sp)
  beqz t0, .L62
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 12(sp)
  lw t1, 12(sp)
  li t2, 2
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 16(sp)
  lw t0, 16(sp)
  beqz t0, .L69
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 20(sp)
  lw t1, 20(sp)
  addi t0, t1, 1
  sw t0, 24(sp)
  lw t0, 24(sp)
  sw t0, 0(sp)
  j .L61
.L69:
  la t1, total
  lw t0, 0(t1)
  sw t0, 28(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 32(sp)
  lw t1, 28(sp)
  lw t2, 32(sp)
  add t0, t1, t2
  sw t0, 36(sp)
  lw t0, 36(sp)
  la t1, total
  sw t0, 0(t1)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 40(sp)
  lw t1, 40(sp)
  addi t0, t1, 1
  sw t0, 44(sp)
  lw t0, 44(sp)
  sw t0, 0(sp)
  j .L61
.L62:
  la t1, total
  lw t0, 0(t1)
  sw t0, 48(sp)
  li a0, 1
  li a1, 2
  li a2, 3
  li a3, 4
  li a4, 5
  li a5, 6
  li a6, 7
  li a7, 8
  call reverse_forward
  sw a0, 52(sp)
  lw t1, 48(sp)
  lw t2, 52(sp)
  add t0, t1, t2
  sw t0, 56(sp)
  lw t1, 56(sp)
  mv a0, t1
  lw ra, 60(sp)
  addi sp, sp, 64
  ret
