.text
.globl main
choose:
  beqz a0, .L3
  li t4, 20
  j .L4
.L3:
  li t4, 30
.L4:
  mv a0, t4
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  sw s1, 8(sp)
  li t4, 0
  li s1, 1
.L11:
  li t2, 5
  bge t4, t2, .L12
  add s1, s1, t4
  addi t4, t4, 1
  j .L11
.L12:
  li t2, 5
  sub t5, t4, t2
  seqz t5, t5
  mv a0, t5
  call choose
  mv t4, a0
  add t5, s1, t4
  addi t4, t5, -4
  addi t5, t4, -1
  addi t4, t5, -4
  addi a0, t4, 1
  lw s1, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
