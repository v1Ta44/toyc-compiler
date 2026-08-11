#!/usr/bin/env bash
# 在 WSL 中完成编译器构建、ToyC 编译、RV32 链接和 QEMU 运行验证。
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${repo_dir}/build-wsl"
generated_dir="${repo_dir}/tests/generated"
binary_dir="${build_dir}/rv32-tests"

cmake -S "${repo_dir}" -B "${build_dir}"
cmake --build "${build_dir}" -j2
mkdir -p "${generated_dir}" "${binary_dir}"

run_case() {
    local case_name="$1"
    local expected="$2"
    local mode="$3"
    local suffix=""
    local option=()

    if [[ "${mode}" == "opt" ]]; then
        suffix="_opt"
        option=(-opt)
    fi

    local assembly="${generated_dir}/${case_name}${suffix}.s"
    local binary="${binary_dir}/${case_name}${suffix}"

    "${build_dir}/toyc_compiler" "${option[@]}" \
        < "${repo_dir}/tests/${case_name}.tc" > "${assembly}"

    # 生成代码只依赖 RV32I/M 指令，不需要 C 运行库。
    riscv64-linux-gnu-gcc -march=rv32im -mabi=ilp32 -nostdlib -static \
        -Wl,-e,_start "${repo_dir}/tests/rv32_start.s" "${assembly}" -o "${binary}"

    set +e
    qemu-riscv32 "${binary}"
    local actual=$?
    set -e

    if [[ "${actual}" -ne "${expected}" ]]; then
        echo "FAIL ${case_name}${suffix}: expected ${expected}, got ${actual}" >&2
        return 1
    fi
    echo "PASS ${case_name}${suffix}: exit=${actual}"
}

# 动态生成极端规模源码，避免在仓库中保存数千行机械测试数据。
generate_codegen_limits() {
    printf '%s\n' 'int main() { int x = 3; return -x; }'
}

generate_large_stack() {
    printf 'int main() {'
    for i in $(seq 0 799); do
        printf ' int v%s = %s;' "${i}" "${i}"
    done
    printf ' return v799 - v0; }\n'
}

generate_many_arguments() {
    printf 'int last('
    for i in $(seq 0 599); do
        if (( i != 0 )); then
            printf ','
        fi
        printf 'int p%s' "${i}"
    done
    printf ') { return p599; } int main() { return last('
    for i in $(seq 0 599); do
        if (( i != 0 )); then
            printf ','
        fi
        printf '%s' "${i}"
    done
    printf '); }\n'
}

run_generated_case() {
    local case_name="$1"
    local expected="$2"
    local mode="$3"
    local option=()
    if [[ "${mode}" == "opt" ]]; then
        option=(-opt)
    fi

    local source="${binary_dir}/${case_name}.tc"
    local assembly="${binary_dir}/${case_name}_${mode}.s"
    local binary="${binary_dir}/${case_name}_${mode}"
    "generate_${case_name}" > "${source}"
    "${build_dir}/toyc_compiler" "${option[@]}" < "${source}" > "${assembly}"
    riscv64-linux-gnu-gcc -march=rv32im -mabi=ilp32 -nostdlib -static \
        -Wl,-e,_start "${repo_dir}/tests/rv32_start.s" "${assembly}" -o "${binary}"

    set +e
    qemu-riscv32 "${binary}"
    local actual=$?
    set -e
    if [[ "${actual}" -ne "${expected}" ]]; then
        echo "FAIL ${case_name}_${mode}: expected ${expected}, got ${actual}" >&2
        return 1
    fi
    echo "PASS ${case_name}_${mode}: exit=${actual}"
}

run_case recursion 120 normal
run_case recursion 120 opt
run_case control_flow 1 normal
run_case control_flow 1 opt
run_case optimizations 1 normal
run_case optimizations 1 opt
run_case register_pressure 78 normal
run_case register_pressure 78 opt
run_case complex_semantics 1 normal
run_case complex_semantics 1 opt
run_case const_expr_chain 1 normal
run_case const_expr_chain 1 opt
run_case advanced_matrix 110 normal
run_case advanced_matrix 110 opt
run_case basic_combined 124 normal
run_case basic_combined 124 opt
run_case optimizer_cfg 23 normal
run_case optimizer_cfg 23 opt
run_case loop_optimizations 1 normal
run_case loop_optimizations 1 opt
run_case global_effects 75 normal
run_case global_effects 75 opt
run_generated_case codegen_limits 253 normal
run_generated_case codegen_limits 253 opt
run_generated_case large_stack 31 normal
run_generated_case large_stack 31 opt
run_generated_case many_arguments 87 normal
run_generated_case many_arguments 87 opt

# 专用用例中有两组重复的加法和乘法；优化版应各只保留每组的一条指令。
normal_mul_count="$(grep -c '^  mul ' "${generated_dir}/optimizations.s" || true)"
opt_mul_count="$(grep -c '^  mul ' "${generated_dir}/optimizations_opt.s" || true)"
normal_add_count="$(grep -c '^  add ' "${generated_dir}/optimizations.s" || true)"
opt_add_count="$(grep -c '^  add ' "${generated_dir}/optimizations_opt.s" || true)"
if (( opt_mul_count >= normal_mul_count || opt_add_count >= normal_add_count )); then
    echo "FAIL optimization shape: add ${normal_add_count}->${opt_add_count}, mul ${normal_mul_count}->${opt_mul_count}" >&2
    exit 1
fi
echo "PASS optimization shape: add ${normal_add_count}->${opt_add_count}, mul ${normal_mul_count}->${opt_mul_count}"

# 核心用例的优化版必须减少静态指令数；控制流热路径还要求显著降低访存和搬运。
for case_name in recursion control_flow optimizations register_pressure complex_semantics \
                 advanced_matrix basic_combined optimizer_cfg loop_optimizations global_effects; do
    normal_instruction_count="$(grep -c '^  ' "${generated_dir}/${case_name}.s")"
    opt_instruction_count="$(grep -c '^  ' "${generated_dir}/${case_name}_opt.s")"
    if (( opt_instruction_count >= normal_instruction_count )); then
        echo "FAIL instruction count ${case_name}: ${normal_instruction_count}->${opt_instruction_count}" >&2
        exit 1
    fi
    echo "PASS instruction count ${case_name}: ${normal_instruction_count}->${opt_instruction_count}"
done

normal_move_count="$(grep -Ec '^  (lw|sw|mv) ' "${generated_dir}/control_flow.s")"
opt_move_count="$(grep -Ec '^  (lw|sw|mv) ' "${generated_dir}/control_flow_opt.s")"
if (( opt_move_count * 2 >= normal_move_count )); then
    echo "FAIL control-flow data movement: ${normal_move_count}->${opt_move_count}" >&2
    exit 1
fi
echo "PASS control-flow data movement: ${normal_move_count}->${opt_move_count}"

# LICM 应把循环不变量乘除法移到循环标签之前；全局量也不应在热循环中访存。
loop_assembly="${generated_dir}/loop_optimizations_opt.s"
kernel_start="$(grep -n '^kernel:$' "${loop_assembly}" | cut -d: -f1)"
loop_start="$(tail -n "+${kernel_start}" "${loop_assembly}" | grep -n -m1 '^\.L' | cut -d: -f1)"
loop_start="$((kernel_start + loop_start - 1))"
loop_end="$(tail -n "+${kernel_start}" "${loop_assembly}" | grep -n '^\.L' | sed -n '2p' | cut -d: -f1)"
loop_end="$((kernel_start + loop_end - 1))"
mul_line="$(grep -n '^  mul ' "${loop_assembly}" | cut -d: -f1)"
div_line="$(grep -n '^  div ' "${loop_assembly}" | cut -d: -f1)"
hot_global_accesses="$(sed -n "${loop_start},${loop_end}p" "${loop_assembly}" | \
    grep -Ec '^  (la|lw|sw) ' || true)"
opt_loop_instructions="$(sed -n "${loop_start},${loop_end}p" "${loop_assembly}" | \
    grep -c '^  ')"
normal_loop_assembly="${generated_dir}/loop_optimizations.s"
normal_kernel_start="$(grep -n '^kernel:$' "${normal_loop_assembly}" | cut -d: -f1)"
normal_loop_start="$(tail -n "+${normal_kernel_start}" "${normal_loop_assembly}" | \
    grep -n -m1 '^\.L' | cut -d: -f1)"
normal_loop_start="$((normal_kernel_start + normal_loop_start - 1))"
normal_loop_end="$(tail -n "+${normal_kernel_start}" "${normal_loop_assembly}" | \
    grep -n '^\.L' | sed -n '2p' | cut -d: -f1)"
normal_loop_end="$((normal_kernel_start + normal_loop_end - 1))"
normal_loop_instructions="$(sed -n "${normal_loop_start},${normal_loop_end}p" \
    "${normal_loop_assembly}" | grep -c '^  ')"
if (( mul_line >= loop_start || div_line >= loop_start || hot_global_accesses != 0 || \
      opt_loop_instructions * 2 >= normal_loop_instructions )); then
    echo "FAIL loop optimization shape: loop=${loop_start}, mul=${mul_line}, div=${div_line}, hot-memory=${hot_global_accesses}" >&2
    exit 1
fi
echo "PASS loop optimization shape: body ${normal_loop_instructions}->${opt_loop_instructions}, invariant mul/div hoisted, hot global memory=${hot_global_accesses}"
