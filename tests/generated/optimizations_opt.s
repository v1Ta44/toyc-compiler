.text
.globl main
identity:
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  li a0, 144
  call identity
  mv t4, a0
  addi t4, t4, 242
  li t2, 386
  sub a0, t4, t2
  seqz a0, a0
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
