.text
.globl main
choose:
  beqz a0, .L3
.L40:
  li t4, 20
.L4:
  mv a0, t4
  ret
.L3:
  li t4, 30
  j .L4
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  sw s1, 8(sp)
  li t5, 5
  li t4, 0
  li t6, 1
  mv s1, t6
.L11:
  bge t4, t5, .L12
.L41:
  add t6, s1, t4
  addi t4, t4, 1
  mv s1, t6
  j .L11
.L12:
  sub t4, t4, t5
  seqz t4, t4
  mv a0, t4
  call choose
  mv t4, a0
  add t4, s1, t4
  addi t4, t4, -4
  addi t4, t4, -1
  addi t4, t4, -4
  addi a0, t4, 1
  lw s1, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
