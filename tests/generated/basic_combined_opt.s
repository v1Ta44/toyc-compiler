.data
.globl total
total:
  .word 0
.text
.globl main
sink:
  slli t4, a1, 1
  add t4, a0, t4
  slli t2, a2, 1
  add t5, a2, t2
  add t4, t4, t5
  slli t5, a3, 2
  add t4, t4, t5
  slli t2, a4, 2
  add t5, a4, t2
  add t4, t4, t5
  li t2, 6
  mul t5, a5, t2
  add t4, t4, t5
  slli t2, a6, 3
  sub t5, t2, a6
  add t4, t4, t5
  slli t5, a7, 3
  add a0, t4, t5
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
  add t4, t4, a0
  la t1, total
  sw t4, 0(t1)
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  sw s1, 8(sp)
  li t5, 4
  li t6, 2
  la t1, total
  lw a0, 0(t1)
  li t4, 0
  mv s1, a0
.L43:
  bge t4, t5, .L44
.L85:
  bne t4, t6, .L49
.L86:
  addi t4, t4, 1
  j .L43
.L49:
  add a0, s1, t4
  addi t4, t4, 1
  mv s1, a0
  j .L43
.L44:
  mv a1, t6
  mv a3, t5
  li a0, 1
  li a2, 3
  li a4, 5
  li a5, 6
  li a6, 7
  li a7, 8
  call reverse_forward
  mv t4, a0
  add t4, s1, t4
  la t1, total
  sw s1, 0(t1)
  mv a0, t4
  lw s1, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
