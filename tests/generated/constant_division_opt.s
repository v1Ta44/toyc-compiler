.text
.globl main
check_negative:
  li t2, 1717986919
  mulh t2, a0, t2
  srai t2, t2, 2
  srli t3, t2, 31
  add t2, t2, t3
  mv t4, t2
  li t2, -1234
  sub t4, t4, t2
  seqz t4, t4
  beqz t4, .L104
.L99:
  li t2, 1717986919
  mulh t2, a0, t2
  srai t2, t2, 2
  srli t3, t2, 31
  add t2, t2, t3
  li t3, 10
  mul t3, t2, t3
  sub t4, a0, t3
  li t2, -5
  sub t4, t4, t2
  seqz t4, t4
  snez t4, t4
.L6:
  beqz t4, .L105
.L100:
  li t2, -1840700269
  mulh t2, a0, t2
  add t2, t2, a0
  srai t2, t2, 2
  srli t3, t2, 31
  add t2, t2, t3
  neg t2, t2
  mv t4, t2
  li t2, 1763
  sub t4, t4, t2
  seqz t4, t4
  snez t4, t4
.L12:
  beqz t4, .L106
.L101:
  li t2, -1840700269
  mulh t2, a0, t2
  add t2, t2, a0
  srai t2, t2, 2
  srli t3, t2, 31
  add t2, t2, t3
  neg t2, t2
  li t3, -7
  mul t3, t2, t3
  sub t4, a0, t3
  li t2, -4
  sub t4, t4, t2
  seqz t4, t4
  snez t4, t4
.L18:
  beqz t4, .L107
.L102:
  li t2, -580400985
  mulh t2, a0, t2
  add t2, t2, a0
  srai t2, t2, 5
  srli t3, t2, 31
  add t2, t2, t3
  mv t4, t2
  li t2, -333
  sub t4, t4, t2
  seqz t4, t4
  snez t4, t4
.L24:
  beqz t4, .L108
.L103:
  li t2, -580400985
  mulh t2, a0, t2
  add t2, t2, a0
  srai t2, t2, 5
  srli t3, t2, 31
  add t2, t2, t3
  li t3, 37
  mul t3, t2, t3
  sub t4, a0, t3
  li t2, -24
  sub t4, t4, t2
  seqz t4, t4
  snez t4, t4
.L30:
  mv a0, t4
  ret
.L104:
  li t4, 0
  j .L6
.L105:
  li t4, 0
  j .L12
.L106:
  li t4, 0
  j .L18
.L107:
  li t4, 0
  j .L24
.L108:
  li t4, 0
  j .L30
check_positive:
  li t2, 1717986919
  mulh t2, a0, t2
  srai t2, t2, 2
  srli t3, t2, 31
  add t2, t2, t3
  mv t4, t2
  li t2, 1234
  sub t4, t4, t2
  seqz t4, t4
  beqz t4, .L114
.L109:
  li t2, 1717986919
  mulh t2, a0, t2
  srai t2, t2, 2
  srli t3, t2, 31
  add t2, t2, t3
  li t3, 10
  mul t3, t2, t3
  sub t4, a0, t3
  li t2, 5
  sub t4, t4, t2
  seqz t4, t4
  snez t4, t4
.L41:
  beqz t4, .L115
.L110:
  li t2, -1840700269
  mulh t2, a0, t2
  add t2, t2, a0
  srai t2, t2, 2
  srli t3, t2, 31
  add t2, t2, t3
  neg t2, t2
  mv t4, t2
  li t2, -1763
  sub t4, t4, t2
  seqz t4, t4
  snez t4, t4
.L47:
  beqz t4, .L116
.L111:
  li t2, -1840700269
  mulh t2, a0, t2
  add t2, t2, a0
  srai t2, t2, 2
  srli t3, t2, 31
  add t2, t2, t3
  neg t2, t2
  li t3, -7
  mul t3, t2, t3
  sub t4, a0, t3
  li t2, 4
  sub t4, t4, t2
  seqz t4, t4
  snez t4, t4
.L53:
  beqz t4, .L117
.L112:
  li t2, -580400985
  mulh t2, a0, t2
  add t2, t2, a0
  srai t2, t2, 5
  srli t3, t2, 31
  add t2, t2, t3
  mv t4, t2
  li t2, 333
  sub t4, t4, t2
  seqz t4, t4
  snez t4, t4
.L59:
  beqz t4, .L118
.L113:
  li t2, -580400985
  mulh t2, a0, t2
  add t2, t2, a0
  srai t2, t2, 5
  srli t3, t2, 31
  add t2, t2, t3
  li t3, 37
  mul t3, t2, t3
  sub t4, a0, t3
  li t2, 24
  sub t4, t4, t2
  seqz t4, t4
  snez t4, t4
.L65:
  mv a0, t4
  ret
.L114:
  li t4, 0
  j .L41
.L115:
  li t4, 0
  j .L47
.L116:
  li t4, 0
  j .L53
.L117:
  li t4, 0
  j .L59
.L118:
  li t4, 0
  j .L65
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  li a0, -12345
  call check_negative
  mv t4, a0
  beqz t4, .L120
.L119:
  li a0, 12345
  call check_positive
  mv t4, a0
  snez t4, t4
.L73:
  mv a0, t4
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
.L120:
  li t4, 0
  j .L73
