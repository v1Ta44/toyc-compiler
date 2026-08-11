.data
.globl total
total:
  .word 0
.text
.globl main
sink:
  slli t4, a1, 1
  add t5, a0, t4
  slli t2, a2, 1
  add t4, a2, t2
  add t6, t5, t4
  slli t4, a3, 2
  add t5, t6, t4
  slli t2, a4, 2
  add t4, a4, t2
  add t6, t5, t4
  li t2, 6
  mul t4, a5, t2
  add t5, t6, t4
  slli t2, a6, 3
  sub t4, t2, a6
  add t6, t5, t4
  slli t4, a7, 3
  add a0, t6, t4
  ret
reverse_forward:
  addi sp, sp, -16
  sw ra, 12(sp)
  mv t3, a0
  mv a0, a7
  mv a7, t3
  mv t3, a1
  mv a1, a6
  mv a6, t3
  mv t3, a2
  mv a2, a5
  mv a5, t3
  mv t3, a3
  mv a3, a4
  mv a4, t3
  call sink
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
mark:
  la t1, total
  lw t4, 0(t1)
  add t5, t4, a0
  la t1, total
  sw t5, 0(t1)
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  sw s1, 8(sp)
  li t4, 0
.L43:
  li t2, 4
  bge t4, t2, .L44
  li t2, 2
  bne t4, t2, .L49
  addi t4, t4, 1
  j .L43
.L49:
  la t1, total
  lw t5, 0(t1)
  add t6, t5, t4
  la t1, total
  sw t6, 0(t1)
  addi t4, t4, 1
  j .L43
.L44:
  la t1, total
  lw s1, 0(t1)
  li a0, 1
  li a1, 2
  li a2, 3
  li a3, 4
  li a4, 5
  li a5, 6
  li a6, 7
  li a7, 8
  call reverse_forward
  mv t4, a0
  add a0, s1, t4
  lw s1, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
