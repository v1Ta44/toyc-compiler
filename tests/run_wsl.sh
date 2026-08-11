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

run_case recursion 120 normal
run_case recursion 120 opt
run_case control_flow 1 normal
run_case control_flow 1 opt
