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
run_generated_case codegen_limits 253 normal
run_generated_case codegen_limits 253 opt
run_generated_case large_stack 31 normal
run_generated_case large_stack 31 opt
run_generated_case many_arguments 87 normal
run_generated_case many_arguments 87 opt

# 专用用例中有两组重复的加法和乘法；优化版应各只保留每组的一条指令。
normal_mul_count="$(grep -c '^  mul ' "${generated_dir}/optimizations.s")"
opt_mul_count="$(grep -c '^  mul ' "${generated_dir}/optimizations_opt.s")"
normal_add_count="$(grep -c '^  add ' "${generated_dir}/optimizations.s")"
opt_add_count="$(grep -c '^  add ' "${generated_dir}/optimizations_opt.s")"
if (( opt_mul_count >= normal_mul_count || opt_add_count >= normal_add_count )); then
    echo "FAIL optimization shape: add ${normal_add_count}->${opt_add_count}, mul ${normal_mul_count}->${opt_mul_count}" >&2
    exit 1
fi
echo "PASS optimization shape: add ${normal_add_count}->${opt_add_count}, mul ${normal_mul_count}->${opt_mul_count}"
