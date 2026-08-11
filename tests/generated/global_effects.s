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
  addi sp, sp, -16
  la t1, left
  lw t0, 0(t1)
  sw t0, 0(sp)
  lw t1, 0(sp)
  mv a0, t1
  addi sp, sp, 16
  ret
write_right:
  addi sp, sp, -16
  sw a0, 0(sp)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 4(sp)
  lw t0, 4(sp)
  la t1, right
  sw t0, 0(t1)
  la t1, right
  lw t0, 0(t1)
  sw t0, 8(sp)
  lw t1, 8(sp)
  mv a0, t1
  addi sp, sp, 16
  ret
recursive_update:
  addi sp, sp, -80
  sw ra, 76(sp)
  sw a0, 0(sp)
  la t1, left
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
  la t1, left
  sw t0, 0(t1)
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 16(sp)
  lw t1, 16(sp)
  slt t0, zero, t1
  xori t0, t0, 1
  sw t0, 20(sp)
  lw t0, 20(sp)
  beqz t0, .L11
  la t1, left
  lw t0, 0(t1)
  sw t0, 24(sp)
  la t1, right
  lw t0, 0(t1)
  sw t0, 28(sp)
  lw t1, 24(sp)
  lw t2, 28(sp)
  add t0, t1, t2
  sw t0, 32(sp)
  lw t1, 32(sp)
  mv a0, t1
  lw ra, 76(sp)
  addi sp, sp, 80
  ret
.L11:
  lw t1, 0(sp)
  mv t0, t1
  sw t0, 36(sp)
  lw t1, 36(sp)
  addi t0, t1, -1
  sw t0, 40(sp)
  lw a0, 40(sp)
  call recursive_update
  sw a0, 44(sp)
  lw t0, 44(sp)
  sw t0, 48(sp)
  lw t1, 48(sp)
  mv t0, t1
  sw t0, 52(sp)
  la t1, left
  lw t0, 0(t1)
  sw t0, 56(sp)
  lw t1, 52(sp)
  lw t2, 56(sp)
  add t0, t1, t2
  sw t0, 60(sp)
  la t1, right
  lw t0, 0(t1)
  sw t0, 64(sp)
  lw t1, 60(sp)
  lw t2, 64(sp)
  add t0, t1, t2
  sw t0, 68(sp)
  lw t1, 68(sp)
  mv a0, t1
  lw ra, 76(sp)
  addi sp, sp, 80
  ret
main:
  addi sp, sp, -32
  sw ra, 28(sp)
  li a0, 3
  call recursive_update
  sw a0, 0(sp)
  lw t0, 0(sp)
  sw t0, 4(sp)
  lw t1, 4(sp)
  mv t0, t1
  sw t0, 8(sp)
  lw a0, 8(sp)
  call write_right
  sw a0, 12(sp)
  call read_left
  sw a0, 16(sp)
  la t1, right
  lw t0, 0(t1)
  sw t0, 20(sp)
  lw t1, 16(sp)
  lw t2, 20(sp)
  add t0, t1, t2
  sw t0, 24(sp)
  lw t1, 24(sp)
  mv a0, t1
  lw ra, 28(sp)
  addi sp, sp, 32
  ret
