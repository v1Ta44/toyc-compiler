.text
.globl main
check_negative:
  addi sp, sp, -96
  sw a0, 0(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 4(sp)
  lw t1, 4(sp)
  lw t1, 4(sp)
  li t2, 10
  div t0, t1, t2
  sw t0, 8(sp)
  lw t1, 8(sp)
  li t2, -1234
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 12(sp)
  li t0, 0
  sw t0, 16(sp)
  lw t0, 12(sp)
  beqz t0, .L7
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 20(sp)
  lw t1, 20(sp)
  lw t1, 20(sp)
  li t2, 10
  rem t0, t1, t2
  sw t0, 24(sp)
  lw t1, 24(sp)
  li t2, -5
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 28(sp)
  lw t2, 28(sp)
  snez t0, t2
  sw t0, 16(sp)
.L7:
  li t0, 0
  sw t0, 32(sp)
  lw t0, 16(sp)
  beqz t0, .L14
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 36(sp)
  lw t1, 36(sp)
  lw t1, 36(sp)
  li t2, -7
  div t0, t1, t2
  sw t0, 40(sp)
  lw t1, 40(sp)
  li t2, 1763
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 44(sp)
  lw t2, 44(sp)
  snez t0, t2
  sw t0, 32(sp)
.L14:
  li t0, 0
  sw t0, 48(sp)
  lw t0, 32(sp)
  beqz t0, .L21
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 52(sp)
  lw t1, 52(sp)
  lw t1, 52(sp)
  li t2, -7
  rem t0, t1, t2
  sw t0, 56(sp)
  lw t1, 56(sp)
  li t2, -4
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 60(sp)
  lw t2, 60(sp)
  snez t0, t2
  sw t0, 48(sp)
.L21:
  li t0, 0
  sw t0, 64(sp)
  lw t0, 48(sp)
  beqz t0, .L28
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 68(sp)
  lw t1, 68(sp)
  lw t1, 68(sp)
  li t2, 37
  div t0, t1, t2
  sw t0, 72(sp)
  lw t1, 72(sp)
  li t2, -333
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 76(sp)
  lw t2, 76(sp)
  snez t0, t2
  sw t0, 64(sp)
.L28:
  li t0, 0
  sw t0, 80(sp)
  lw t0, 64(sp)
  beqz t0, .L35
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 84(sp)
  lw t1, 84(sp)
  lw t1, 84(sp)
  li t2, 37
  rem t0, t1, t2
  sw t0, 88(sp)
  lw t1, 88(sp)
  li t2, -24
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 92(sp)
  lw t2, 92(sp)
  snez t0, t2
  sw t0, 80(sp)
.L35:
  lw t1, 80(sp)
  mv a0, t1
  addi sp, sp, 96
  ret
check_positive:
  addi sp, sp, -96
  sw a0, 0(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 4(sp)
  lw t1, 4(sp)
  lw t1, 4(sp)
  li t2, 10
  div t0, t1, t2
  sw t0, 8(sp)
  lw t1, 8(sp)
  li t2, 1234
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 12(sp)
  li t0, 0
  sw t0, 16(sp)
  lw t0, 12(sp)
  beqz t0, .L48
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 20(sp)
  lw t1, 20(sp)
  lw t1, 20(sp)
  li t2, 10
  rem t0, t1, t2
  sw t0, 24(sp)
  lw t1, 24(sp)
  li t2, 5
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 28(sp)
  lw t2, 28(sp)
  snez t0, t2
  sw t0, 16(sp)
.L48:
  li t0, 0
  sw t0, 32(sp)
  lw t0, 16(sp)
  beqz t0, .L55
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 36(sp)
  lw t1, 36(sp)
  lw t1, 36(sp)
  li t2, -7
  div t0, t1, t2
  sw t0, 40(sp)
  lw t1, 40(sp)
  li t2, -1763
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 44(sp)
  lw t2, 44(sp)
  snez t0, t2
  sw t0, 32(sp)
.L55:
  li t0, 0
  sw t0, 48(sp)
  lw t0, 32(sp)
  beqz t0, .L62
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 52(sp)
  lw t1, 52(sp)
  lw t1, 52(sp)
  li t2, -7
  rem t0, t1, t2
  sw t0, 56(sp)
  lw t1, 56(sp)
  li t2, 4
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 60(sp)
  lw t2, 60(sp)
  snez t0, t2
  sw t0, 48(sp)
.L62:
  li t0, 0
  sw t0, 64(sp)
  lw t0, 48(sp)
  beqz t0, .L69
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 68(sp)
  lw t1, 68(sp)
  lw t1, 68(sp)
  li t2, 37
  div t0, t1, t2
  sw t0, 72(sp)
  lw t1, 72(sp)
  li t2, 333
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 76(sp)
  lw t2, 76(sp)
  snez t0, t2
  sw t0, 64(sp)
.L69:
  li t0, 0
  sw t0, 80(sp)
  lw t0, 64(sp)
  beqz t0, .L76
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 84(sp)
  lw t1, 84(sp)
  lw t1, 84(sp)
  li t2, 37
  rem t0, t1, t2
  sw t0, 88(sp)
  lw t1, 88(sp)
  li t2, 24
  sub t0, t1, t2
  seqz t0, t0
  sw t0, 92(sp)
  lw t2, 92(sp)
  snez t0, t2
  sw t0, 80(sp)
.L76:
  lw t1, 80(sp)
  mv a0, t1
  addi sp, sp, 96
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  li a0, -12345
  call check_negative
  sw a0, 0(sp)
  li t0, 0
  sw t0, 4(sp)
  lw t0, 0(sp)
  beqz t0, .L85
  li a0, 12345
  call check_positive
  sw a0, 8(sp)
  lw t2, 8(sp)
  snez t0, t2
  sw t0, 4(sp)
.L85:
  lw t1, 4(sp)
  mv a0, t1
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
