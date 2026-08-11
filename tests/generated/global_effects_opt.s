.data
.globl left
left:
  .word 1
.globl right
right:
  .word 10
.text
.globl main
read_left:
  la t1, left
  lw t4, 0(t1)
  mv a0, t4
  ret
write_right:
  mv t4, a0
  mv t5, t4
  la t1, right
  sw t4, 0(t1)
  mv a0, t5
  ret
recursive_update:
  addi sp, sp, -16
  sw ra, 12(sp)
  sw s1, 8(sp)
  la t1, left
  lw t4, 0(t1)
  la t1, right
  lw s1, 0(t1)
  add t4, t4, a0
  bgt a0, zero, .L8
  mv t5, t4
  add t5, t5, s1
  la t1, left
  sw t4, 0(t1)
  mv a0, t5
  lw s1, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
.L8:
  addi t5, a0, -1
  la t1, left
  sw t4, 0(t1)
  mv a0, t5
  call recursive_update
  mv t5, a0
  la t1, left
  lw t4, 0(t1)
  add t5, t5, t4
  add t5, t5, s1
  la t1, left
  sw t4, 0(t1)
  mv a0, t5
  lw s1, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
main:
  addi sp, sp, -16
  sw ra, 12(sp)
  sw s1, 8(sp)
  li a0, 3
  call recursive_update
  mv t4, a0
  mv a0, t4
  call write_right
  mv t4, a0
  la t1, right
  lw s1, 0(t1)
  call read_left
  mv t4, a0
  add t4, t4, s1
  mv a0, t4
  lw s1, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret
