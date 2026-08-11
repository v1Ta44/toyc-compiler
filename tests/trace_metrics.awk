# 解析 qemu-riscv32 -d in_asm,exec,nochain 输出，按实际执行的翻译块累计 RV32 指令。
function clear_pending(key) {
    for (key in pending_opcode)
        delete pending_opcode[key]
    pending_total = pending_stack_lw = pending_stack_sw = 0
    pending_cycles = 0
    pending_save = pending_restore = pending_call = pending_branch = 0
}

/^0x[[:xdigit:]]+:/ {
    opcode = $3
    ++pending_total
    if (opcode ~ /^(div|divu|rem|remu)$/)
        pending_cycles += 20
    else if (opcode ~ /^(mul|mulh|mulhu|mulhsu)$/)
        pending_cycles += 3
    else if (opcode == "lw" || opcode == "sw")
        pending_cycles += 3
    else
        pending_cycles += 1
    ++pending_opcode[opcode]
    if (opcode == "lw" && $0 ~ /\(sp\)/)
        ++pending_stack_lw
    if (opcode == "sw" && $0 ~ /\(sp\)/)
        ++pending_stack_sw
    if (opcode == "sw" && $0 ~ /[[:space:]](ra|s([0-9]|10|11)),/ && $0 ~ /\(sp\)/)
        ++pending_save
    if (opcode == "lw" && $0 ~ /[[:space:]](ra|s([0-9]|10|11)),/ && $0 ~ /\(sp\)/)
        ++pending_restore
    if (opcode == "jal" || opcode == "jalr")
        ++pending_call
    if (opcode ~ /^(beq|bne|blt|bge|bltu|bgeu|ble|bgt|bleu|bgtu|j|jal|jalr|ret)$/)
        ++pending_branch
    next
}

/^Trace 0: 0x[[:xdigit:]]+/ {
    block = $3
    if (pending_total > 0) {
        block_total[block] = pending_total
        block_cycles[block] = pending_cycles
        block_stack_lw[block] = pending_stack_lw
        block_stack_sw[block] = pending_stack_sw
        block_save[block] = pending_save
        block_restore[block] = pending_restore
        block_call[block] = pending_call
        block_branch[block] = pending_branch
        for (opcode in pending_opcode)
            block_opcode[block SUBSEP opcode] = pending_opcode[opcode]
        clear_pending()
    }
    total += block_total[block]
    cycles += block_cycles[block]
    mul += block_opcode[block SUBSEP "mul"] + block_opcode[block SUBSEP "mulh"] + \
           block_opcode[block SUBSEP "mulhu"] + block_opcode[block SUBSEP "mulhsu"]
    divrem += block_opcode[block SUBSEP "div"] + block_opcode[block SUBSEP "divu"] + \
              block_opcode[block SUBSEP "rem"] + block_opcode[block SUBSEP "remu"]
    lw += block_opcode[block SUBSEP "lw"]
    sw += block_opcode[block SUBSEP "sw"]
    stack_lw += block_stack_lw[block]
    stack_sw += block_stack_sw[block]
    saves += block_save[block]
    restores += block_restore[block]
    calls += block_call[block]
    branches += block_branch[block]
}

END {
    printf "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", total, cycles, mul, divrem, lw, sw, \
           stack_lw, stack_sw, saves, restores, calls, branches
}
