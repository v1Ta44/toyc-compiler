#include "compiler.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>
namespace toyc {
// 由 Bison 填充，main 在解析完成后把它交给后端。
Program *g_program = nullptr;

// 表达式求值的中间结果。常量直接放在 n 中，运行期值由 v 指向虚拟临时量或存储位置。
struct Value {
    bool constant{};
    int n{};
    std::string v;
};

// 项目自建的三地址 IR。x 通常为目标，y/z 为操作数，aux 保存运算符或被调函数名。
struct IR {
    enum Op { Label, Mov, Bin, Load, Store, Call, Ret, Jump, BrZ };
    Op op;
    std::string x, y, z, aux;
    int imm{};
    IR(Op operation, std::string target = {}, std::string left = {}, std::string right = {},
       std::string extra = {}, int immediate = 0)
        : op(operation), x(std::move(target)), y(std::move(left)), z(std::move(right)),
          aux(std::move(extra)), imm(immediate) {
    }
};

// Generator 同时负责 AST -> IR 的降低和 IR -> RV32 的线性代码生成。
class Generator {
    // IR 按源程序执行顺序保存，控制流使用显式 Label/Jump/BrZ 表示。
    std::vector<IR> ir_;
    // scopes_ 从外到内保存局部符号表；反向查找自然实现词法作用域遮蔽。
    std::vector<std::unordered_map<std::string, Value>> scopes_;
    std::unordered_map<std::string, Value> globals_;
    // 每层循环保存 break 目标和 continue 目标，以支持嵌套循环。
    std::vector<std::pair<std::string, std::string>> loops_;
    // 非 const 全局量最终输出到 .data 段。
    std::vector<std::pair<std::string, int>> data_;
    int next_ = 0;
    bool opt_;

    // 将 Call IR 中的逗号分隔参数转换为列表。
    static std::vector<std::string> callArgs(const std::string &text) {
        std::stringstream ss(text);
        std::string arg;
        std::vector<std::string> result;
        while (std::getline(ss, arg, ','))
            if (!arg.empty())
                result.push_back(arg);
        return result;
    }

    static std::string joinArgs(const std::vector<std::string> &args) {
        std::string result;
        for (const auto &arg : args)
            result += arg + ",";
        return result;
    }

    static bool isVirtual(const std::string &value) {
        return !value.empty() && (value[0] == 't' || value[0] == '@');
    }

    static int fromBits(std::uint32_t value) {
        return static_cast<int>(std::bit_cast<std::int32_t>(value));
    }

    static int wrapNeg(int value) {
        return fromBits(0U - static_cast<std::uint32_t>(value));
    }

    static std::unordered_map<std::string, int> immediateConstants(const std::vector<IR> &code) {
        std::unordered_map<std::string, int> constants;
        std::unordered_map<std::string, int> definitions;
        for (const auto &ins : code) {
            const bool defines = !ins.x.empty() && ins.x[0] == 't' &&
                                 (ins.op == IR::Mov || ins.op == IR::Bin || ins.op == IR::Load ||
                                  ins.op == IR::Call);
            if (!defines)
                continue;
            if (++definitions[ins.x] == 1 && ins.op == IR::Mov && ins.y.empty())
                constants[ins.x] = ins.imm;
            else
                constants.erase(ins.x);
        }
        return constants;
    }

    // 局部变量和形参没有地址逃逸，可直接提升为可分配的虚拟寄存器。
    void promoteLocalSlots() {
        for (auto &ins : ir_) {
            if (ins.op == IR::Load && !ins.y.empty() && ins.y[0] == '@') {
                ins.op = IR::Mov;
            } else if (ins.op == IR::Store && !ins.x.empty() && ins.x[0] == '@') {
                ins.op = IR::Mov;
            }
        }
    }

    // 把常量条件分支转换为确定跳转，并按函数 CFG 删除不可达指令。
    void simplifyControlFlow() {
        const auto constants = immediateConstants(ir_);
        std::vector<IR> folded;
        for (auto ins : ir_) {
            if (ins.op == IR::BrZ) {
                if (auto found = constants.find(ins.x); found != constants.end()) {
                    if (found->second == 0)
                        folded.push_back({IR::Jump, ins.y});
                    continue;
                }
            }
            folded.push_back(std::move(ins));
        }
        ir_ = std::move(folded);

        std::vector<IR> reachable;
        for (size_t begin = 0; begin < ir_.size();) {
            size_t end = begin + 1;
            while (end < ir_.size() &&
                   !(ir_[end].op == IR::Label && !ir_[end].x.empty() && ir_[end].x[0] != '.'))
                ++end;

            std::unordered_map<std::string, size_t> labels;
            for (size_t i = begin; i < end; ++i)
                if (ir_[i].op == IR::Label)
                    labels[ir_[i].x] = i;
            std::vector<bool> seen(end - begin);
            std::vector<size_t> work = {begin};
            while (!work.empty()) {
                size_t i = work.back();
                work.pop_back();
                if (i < begin || i >= end || seen[i - begin])
                    continue;
                seen[i - begin] = true;
                const auto &ins = ir_[i];
                if (ins.op == IR::Jump) {
                    if (auto target = labels.find(ins.x); target != labels.end())
                        work.push_back(target->second);
                } else if (ins.op == IR::BrZ) {
                    if (auto target = labels.find(ins.y); target != labels.end())
                        work.push_back(target->second);
                    work.push_back(i + 1);
                } else if (ins.op != IR::Ret) {
                    work.push_back(i + 1);
                }
            }
            for (size_t i = begin; i < end; ++i)
                if (seen[i - begin])
                    reachable.push_back(std::move(ir_[i]));
            begin = end;
        }
        ir_ = std::move(reachable);
    }

    // 将唯一使用的临时结果直接写入紧随其后的最终目标，消除中间 Mov。
    void coalesceAdjacentMoves() {
        std::unordered_map<std::string, int> definitions, uses;
        auto define = [&](const std::string &value) {
            if (!value.empty() && value[0] == 't')
                ++definitions[value];
        };
        auto use = [&](const std::string &value) {
            if (!value.empty() && value[0] == 't')
                ++uses[value];
        };
        for (const auto &ins : ir_) {
            if (ins.op == IR::Mov) {
                define(ins.x);
                use(ins.y);
            } else if (ins.op == IR::Bin) {
                define(ins.x);
                use(ins.y);
                use(ins.z);
            } else if (ins.op == IR::Load) {
                define(ins.x);
                use(ins.y);
            } else if (ins.op == IR::Store) {
                use(ins.y);
            } else if (ins.op == IR::Call) {
                define(ins.x);
                for (const auto &arg : callArgs(ins.y))
                    use(arg);
            } else if (ins.op == IR::BrZ) {
                use(ins.x);
            }
        }

        std::vector<IR> result;
        for (size_t i = 0; i < ir_.size(); ++i) {
            IR ins = std::move(ir_[i]);
            const bool definesValue =
                ins.op == IR::Mov || ins.op == IR::Bin || ins.op == IR::Load || ins.op == IR::Call;
            if (definesValue && !ins.x.empty() && ins.x[0] == 't' && definitions[ins.x] == 1 &&
                uses[ins.x] == 1 && i + 1 < ir_.size() && ir_[i + 1].op == IR::Mov &&
                ir_[i + 1].y == ins.x) {
                ins.x = ir_[i + 1].x;
                ++i;
            }
            result.push_back(std::move(ins));
        }
        ir_ = std::move(result);
    }

    // 代数化简、无用临时量删除和相邻跳转清理，均只处理无副作用 IR。
    void simplifyIR() {
        auto isTemp = [](const std::string &v) { return !v.empty() && v[0] == 't'; };
        const auto constants = immediateConstants(ir_);
        auto isConstant = [&](const std::string &v, int expected) {
            auto found = constants.find(v);
            return found != constants.end() && found->second == expected;
        };

        for (auto &ins : ir_) {
            if (ins.op != IR::Bin)
                continue;
            // x + 0, x - 0, x * 1, x / 1 等恒等式直接变为拷贝。
            if ((ins.aux == "+" || ins.aux == "-") && isConstant(ins.z, 0)) {
                ins.op = IR::Mov;
                ins.z.clear();
                ins.aux.clear();
            } else if (ins.aux == "+" && isConstant(ins.y, 0)) {
                ins.op = IR::Mov;
                ins.y = ins.z;
                ins.z.clear();
                ins.aux.clear();
            } else if ((ins.aux == "*" || ins.aux == "/") && isConstant(ins.z, 1)) {
                ins.op = IR::Mov;
                ins.z.clear();
                ins.aux.clear();
            } else if (ins.aux == "*" && isConstant(ins.y, 1)) {
                ins.op = IR::Mov;
                ins.y = ins.z;
                ins.z.clear();
                ins.aux.clear();
            } else if (ins.aux == "*" && (isConstant(ins.y, 0) || isConstant(ins.z, 0))) {
                ins.op = IR::Mov;
                ins.y.clear();
                ins.z.clear();
                ins.aux.clear();
                ins.imm = 0;
            }
        }

        // 统计所有临时量的使用次数；删除结果完全未被使用的纯指令。
        std::unordered_map<std::string, int> uses;
        auto use = [&](const std::string &v) {
            if (isTemp(v))
                ++uses[v];
        };
        std::vector<IR> live = ir_;
        // 删除一条指令可能使其输入也变成死值，因此重复扫描直到达到不动点。
        bool changed = true;
        while (changed) {
            changed = false;
            uses.clear();
            for (const auto &ins : live) {
                if (ins.op == IR::Mov)
                    use(ins.y);
                else if (ins.op == IR::Bin) {
                    use(ins.y);
                    use(ins.z);
                } else if (ins.op == IR::Store)
                    use(ins.y);
                else if (ins.op == IR::BrZ)
                    use(ins.x);
                else if (ins.op == IR::Load)
                    use(ins.y);
                else if (ins.op == IR::Call)
                    for (const auto &arg : callArgs(ins.y))
                        use(arg);
            }
            std::vector<IR> next;
            for (const auto &ins : live) {
                const bool pure = ins.op == IR::Mov || ins.op == IR::Bin || ins.op == IR::Load;
                if (pure && isTemp(ins.x) && uses[ins.x] == 0) {
                    changed = true;
                    continue;
                }
                next.push_back(ins);
            }
            live = std::move(next);
        }
        std::vector<IR> compact;
        for (size_t i = 0; i < live.size(); ++i) {
            if (live[i].op == IR::Jump && i + 1 < live.size() && live[i + 1].op == IR::Label &&
                live[i].x == live[i + 1].x)
                continue;
            compact.push_back(std::move(live[i]));
        }
        ir_ = std::move(compact);
    }

    // 在每个函数的 CFG 上求“所有前驱均相同”的常量事实，避免跨分支和循环误传播。
    void propagateConstants() {
        std::vector<IR> result;
        for (size_t begin = 0; begin < ir_.size();) {
            size_t end = begin + 1;
            while (end < ir_.size() &&
                   !(ir_[end].op == IR::Label && !ir_[end].x.empty() && ir_[end].x[0] != '.'))
                ++end;
            std::vector<IR> code(ir_.begin() + begin, ir_.begin() + end);
            const size_t count = code.size();
            std::unordered_map<std::string, size_t> labels;
            for (size_t i = 0; i < count; ++i)
                if (code[i].op == IR::Label)
                    labels[code[i].x] = i;

            std::vector<std::vector<size_t>> predecessors(count);
            for (size_t i = 0; i < count; ++i) {
                auto addEdge = [&](size_t target) {
                    if (target < count)
                        predecessors[target].push_back(i);
                };
                if (code[i].op == IR::Jump) {
                    if (auto target = labels.find(code[i].x); target != labels.end())
                        addEdge(target->second);
                } else if (code[i].op == IR::BrZ) {
                    if (auto target = labels.find(code[i].y); target != labels.end())
                        addEdge(target->second);
                    addEdge(i + 1);
                } else if (code[i].op != IR::Ret) {
                    addEdge(i + 1);
                }
            }

            using Facts = std::unordered_map<std::string, int>;
            std::vector<Facts> in(count), out(count);
            std::vector<bool> reachable(count);
            if (count)
                reachable[0] = true;
            auto lookup = [](const Facts &facts, const std::string &value) -> std::optional<int> {
                if (value == "0")
                    return 0;
                if (auto found = facts.find(value); found != facts.end())
                    return found->second;
                return std::nullopt;
            };
            auto transfer = [&](const IR &ins, Facts facts) {
                auto forget = [&](const std::string &value) {
                    if (isVirtual(value))
                        facts.erase(value);
                };
                if (ins.op == IR::Mov) {
                    const auto source =
                        ins.y.empty() ? std::optional<int>{ins.imm} : lookup(facts, ins.y);
                    forget(ins.x);
                    if (isVirtual(ins.x) && source)
                        facts[ins.x] = *source;
                } else if (ins.op == IR::Bin) {
                    const auto left = lookup(facts, ins.y);
                    const auto right = lookup(facts, ins.z);
                    forget(ins.x);
                    if (isVirtual(ins.x) && left && right &&
                        !((ins.aux == "/" || ins.aux == "%") && *right == 0))
                        facts[ins.x] = calc(ins.aux, *left, *right);
                } else if (ins.op == IR::Load || ins.op == IR::Call) {
                    forget(ins.x);
                } else if (ins.op == IR::Store && isVirtual(ins.x)) {
                    const auto source = lookup(facts, ins.y);
                    forget(ins.x);
                    if (source)
                        facts[ins.x] = *source;
                }
                return facts;
            };

            bool changed = true;
            while (changed) {
                changed = false;
                for (size_t i = 0; i < count; ++i) {
                    bool nowReachable = i == 0;
                    Facts merged;
                    bool havePredecessor = false;
                    for (size_t pred : predecessors[i]) {
                        if (!reachable[pred])
                            continue;
                        nowReachable = true;
                        if (!havePredecessor) {
                            merged = out[pred];
                            havePredecessor = true;
                        } else {
                            for (auto it = merged.begin(); it != merged.end();) {
                                auto other = out[pred].find(it->first);
                                if (other == out[pred].end() || other->second != it->second)
                                    it = merged.erase(it);
                                else
                                    ++it;
                            }
                        }
                    }
                    if (!nowReachable)
                        continue;
                    Facts next = transfer(code[i], merged);
                    if (!reachable[i] || in[i] != merged || out[i] != next) {
                        reachable[i] = true;
                        in[i] = std::move(merged);
                        out[i] = std::move(next);
                        changed = true;
                    }
                }
            }

            std::unordered_map<int, std::string> materialized;
            std::vector<IR> prefix;
            auto constantValue = [&](const Facts &facts, const std::string &value) {
                return lookup(facts, value);
            };
            auto constantTemp = [&](int value) {
                if (auto found = materialized.find(value); found != materialized.end())
                    return found->second;
                std::string name = tmp();
                materialized[value] = name;
                prefix.emplace_back(IR::Mov, name, std::string{}, std::string{}, std::string{},
                                    value);
                return name;
            };
            std::vector<IR> rewritten;
            for (size_t i = 0; i < count; ++i) {
                IR ins = code[i];
                const Facts &facts = in[i];
                auto replace = [&](std::string &operand) {
                    if (auto value = constantValue(facts, operand))
                        operand = constantTemp(*value);
                };
                if (ins.op == IR::Mov && !ins.y.empty()) {
                    if (auto value = constantValue(facts, ins.y)) {
                        ins.y.clear();
                        ins.imm = *value;
                    }
                } else if (ins.op == IR::Bin) {
                    const auto left = constantValue(facts, ins.y);
                    const auto right = constantValue(facts, ins.z);
                    if (left && right && !((ins.aux == "/" || ins.aux == "%") && *right == 0)) {
                        ins.op = IR::Mov;
                        ins.y.clear();
                        ins.z.clear();
                        ins.imm = calc(ins.aux, *left, *right);
                        ins.aux.clear();
                    } else {
                        replace(ins.y);
                        replace(ins.z);
                    }
                } else if (ins.op == IR::Store) {
                    replace(ins.y);
                } else if (ins.op == IR::Call) {
                    auto args = callArgs(ins.y);
                    for (auto &arg : args)
                        replace(arg);
                    ins.y = joinArgs(args);
                } else if (ins.op == IR::BrZ) {
                    if (auto value = constantValue(facts, ins.x)) {
                        if (*value == 0)
                            rewritten.emplace_back(IR::Jump, ins.y);
                        continue;
                    }
                }
                rewritten.push_back(std::move(ins));
            }
            if (!rewritten.empty()) {
                result.push_back(std::move(rewritten.front()));
                result.insert(result.end(), std::make_move_iterator(prefix.begin()),
                              std::make_move_iterator(prefix.end()));
                result.insert(result.end(), std::make_move_iterator(rewritten.begin() + 1),
                              std::make_move_iterator(rewritten.end()));
            }
            begin = end;
        }
        ir_ = std::move(result);
    }

    // 基于 CFG 活跃性删除被覆盖或从未读取的纯定义，包括提升后的局部变量写入。
    void eliminateDeadDefinitions() {
        bool changed = true;
        while (changed) {
            changed = false;
            std::vector<IR> result;
            for (size_t begin = 0; begin < ir_.size();) {
                size_t end = begin + 1;
                while (end < ir_.size() &&
                       !(ir_[end].op == IR::Label && !ir_[end].x.empty() && ir_[end].x[0] != '.'))
                    ++end;
                std::vector<IR> code(ir_.begin() + begin, ir_.begin() + end);
                const size_t count = code.size();
                std::unordered_map<std::string, size_t> labels;
                for (size_t i = 0; i < count; ++i)
                    if (code[i].op == IR::Label)
                        labels[code[i].x] = i;
                std::vector<std::unordered_set<std::string>> uses(count), defs(count),
                    liveIn(count), liveOut(count);
                auto use = [&](size_t i, const std::string &value) {
                    if (isVirtual(value))
                        uses[i].insert(value);
                };
                auto define = [&](size_t i, const std::string &value) {
                    if (isVirtual(value))
                        defs[i].insert(value);
                };
                for (size_t i = 0; i < count; ++i) {
                    const auto &ins = code[i];
                    if (ins.op == IR::Mov) {
                        define(i, ins.x);
                        use(i, ins.y);
                    } else if (ins.op == IR::Bin) {
                        define(i, ins.x);
                        use(i, ins.y);
                        use(i, ins.z);
                    } else if (ins.op == IR::Load) {
                        define(i, ins.x);
                        use(i, ins.y);
                    } else if (ins.op == IR::Store) {
                        define(i, ins.x);
                        use(i, ins.y);
                    } else if (ins.op == IR::Call) {
                        define(i, ins.x);
                        for (const auto &arg : callArgs(ins.y))
                            use(i, arg);
                    } else if (ins.op == IR::BrZ) {
                        use(i, ins.x);
                    }
                }
                bool liveChanged = true;
                while (liveChanged) {
                    liveChanged = false;
                    for (size_t reverse = count; reverse-- > 0;) {
                        std::unordered_set<std::string> out;
                        auto merge = [&](size_t successor) {
                            if (successor < count)
                                out.insert(liveIn[successor].begin(), liveIn[successor].end());
                        };
                        const auto &ins = code[reverse];
                        if (ins.op == IR::Jump) {
                            if (auto target = labels.find(ins.x); target != labels.end())
                                merge(target->second);
                        } else if (ins.op == IR::BrZ) {
                            if (auto target = labels.find(ins.y); target != labels.end())
                                merge(target->second);
                            merge(reverse + 1);
                        } else if (ins.op != IR::Ret) {
                            merge(reverse + 1);
                        }
                        auto inSet = uses[reverse];
                        for (const auto &value : out)
                            if (!defs[reverse].contains(value))
                                inSet.insert(value);
                        if (out != liveOut[reverse] || inSet != liveIn[reverse]) {
                            liveOut[reverse] = std::move(out);
                            liveIn[reverse] = std::move(inSet);
                            liveChanged = true;
                        }
                    }
                }
                for (size_t i = 0; i < count; ++i) {
                    const auto &ins = code[i];
                    const bool pure = ins.op == IR::Mov || ins.op == IR::Bin ||
                                      ins.op == IR::Load ||
                                      (ins.op == IR::Store && isVirtual(ins.x));
                    if (pure && isVirtual(ins.x) && !liveOut[i].contains(ins.x)) {
                        changed = true;
                        continue;
                    }
                    result.push_back(ins);
                }
                begin = end;
            }
            ir_ = std::move(result);
        }
    }

    // 基本块内局部值编号：同时复用未失效的 Load 和二元表达式。
    void eliminateCommonSubexpressions() {
        std::vector<IR> result;
        std::unordered_map<std::string, std::string> available;
        std::unordered_map<std::string, std::string> alias;
        std::unordered_map<std::string, int> version;
        const std::unordered_set<std::string> commutative = {"+", "*", "==", "!="};

        auto resolve = [&](std::string value) {
            std::unordered_set<std::string> seen;
            while (alias.contains(value) && !seen.contains(value)) {
                seen.insert(value);
                value = alias[value];
            }
            return value;
        };
        auto clearBlock = [&] {
            available.clear();
            alias.clear();
        };
        auto numbered = [&](const std::string &value) {
            if (!value.empty() && value[0] == '@')
                return value + "#" + std::to_string(version[value]);
            return value;
        };
        auto forgetResult = [&](const std::string &value) {
            alias.erase(value);
            for (auto it = available.begin(); it != available.end();) {
                if (it->second == value)
                    it = available.erase(it);
                else
                    ++it;
            }
        };

        for (auto ins : ir_) {
            if (ins.op == IR::Label) {
                clearBlock();
                result.push_back(std::move(ins));
                continue;
            }

            if (ins.op == IR::Mov)
                ins.y = resolve(ins.y);
            else if (ins.op == IR::Bin) {
                ins.y = resolve(ins.y);
                ins.z = resolve(ins.z);
            } else if (ins.op == IR::Store)
                ins.y = resolve(ins.y);
            else if (ins.op == IR::BrZ)
                ins.x = resolve(ins.x);
            else if (ins.op == IR::Call) {
                auto args = callArgs(ins.y);
                for (auto &arg : args)
                    arg = resolve(arg);
                ins.y = joinArgs(args);
            }

            if (ins.op == IR::Load) {
                forgetResult(ins.x);
                const std::string key = "load\n" + ins.y;
                if (auto found = available.find(key); found != available.end()) {
                    alias[ins.x] = resolve(found->second);
                    continue;
                }
                available[key] = ins.x;
            } else if (ins.op == IR::Bin) {
                forgetResult(ins.x);
                std::string left = numbered(ins.y), right = numbered(ins.z);
                if (commutative.contains(ins.aux) && right < left)
                    std::swap(left, right);
                const std::string key = "bin\n" + ins.aux + "\n" + left + "\n" + right;
                if (auto found = available.find(key); found != available.end()) {
                    alias[ins.x] = resolve(found->second);
                    continue;
                }
                available[key] = ins.x;
            } else if (ins.op == IR::Mov || ins.op == IR::Call) {
                forgetResult(ins.x);
                if (!ins.x.empty() && ins.x[0] == '@')
                    ++version[ins.x];
            }

            if (ins.op == IR::Store)
                available.erase("load\n" + ins.x);
            if (ins.op == IR::Call) {
                // 调用可能改写全局内存，因此丢弃全部 Load 记录。
                for (auto it = available.begin(); it != available.end();) {
                    if (it->first.rfind("load\n", 0) == 0)
                        it = available.erase(it);
                    else
                        ++it;
                }
            }

            result.push_back(std::move(ins));
            if (result.back().op == IR::Jump || result.back().op == IR::BrZ ||
                result.back().op == IR::Ret)
                clearBlock();
        }
        ir_ = std::move(result);
    }

    // 按函数执行调用感知的线性扫描：短生命周期优先 t4-t6，跨调用值使用 s1-s11。
    static std::unordered_set<std::string> valuesLiveAcrossCalls(const std::vector<IR> &code) {
        const size_t count = code.size();
        std::unordered_map<std::string, size_t> labels;
        for (size_t i = 0; i < count; ++i)
            if (code[i].op == IR::Label)
                labels[code[i].x] = i;

        std::vector<std::unordered_set<std::string>> uses(count), defs(count), liveIn(count),
            liveOut(count);
        auto use = [&](size_t i, const std::string &value) {
            if (isVirtual(value))
                uses[i].insert(value);
        };
        auto define = [&](size_t i, const std::string &value) {
            if (isVirtual(value))
                defs[i].insert(value);
        };
        for (size_t i = 0; i < count; ++i) {
            const auto &ins = code[i];
            if (ins.op == IR::Mov) {
                define(i, ins.x);
                use(i, ins.y);
            } else if (ins.op == IR::Bin) {
                define(i, ins.x);
                use(i, ins.y);
                use(i, ins.z);
            } else if (ins.op == IR::Load) {
                define(i, ins.x);
                use(i, ins.y);
            } else if (ins.op == IR::Store) {
                define(i, ins.x);
                use(i, ins.y);
            } else if (ins.op == IR::Call) {
                define(i, ins.x);
                for (const auto &arg : callArgs(ins.y))
                    use(i, arg);
            } else if (ins.op == IR::BrZ) {
                use(i, ins.x);
            }
        }

        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t reverse = count; reverse-- > 0;) {
                std::unordered_set<std::string> out;
                const auto &ins = code[reverse];
                auto mergeSuccessor = [&](size_t successor) {
                    if (successor < count)
                        out.insert(liveIn[successor].begin(), liveIn[successor].end());
                };
                if (ins.op == IR::Jump) {
                    if (auto target = labels.find(ins.x); target != labels.end())
                        mergeSuccessor(target->second);
                } else if (ins.op == IR::BrZ) {
                    if (auto target = labels.find(ins.y); target != labels.end())
                        mergeSuccessor(target->second);
                    mergeSuccessor(reverse + 1);
                } else if (ins.op != IR::Ret) {
                    mergeSuccessor(reverse + 1);
                }
                auto in = uses[reverse];
                for (const auto &value : out)
                    if (!defs[reverse].contains(value))
                        in.insert(value);
                if (out != liveOut[reverse] || in != liveIn[reverse]) {
                    liveOut[reverse] = std::move(out);
                    liveIn[reverse] = std::move(in);
                    changed = true;
                }
            }
        }

        std::unordered_set<std::string> result;
        for (size_t i = 0; i < count; ++i)
            if (code[i].op == IR::Call)
                for (const auto &value : liveOut[i])
                    if (value != code[i].x)
                        result.insert(value);
        return result;
    }

    std::unordered_map<std::string, std::string>
    allocateRegisters(const std::vector<IR> &code,
                      const std::unordered_map<std::string, int> &constants) const {
        struct Interval {
            std::string value;
            int begin;
            int end;
        };
        std::unordered_map<std::string, std::pair<int, int>> ranges;
        std::unordered_map<std::string, std::string> incomingRegisters;
        auto touch = [&](const std::string &value, int position) {
            if (!isVirtual(value) || constants.contains(value))
                return;
            auto [it, inserted] = ranges.emplace(value, std::pair{position, position});
            if (!inserted)
                it->second.second = position;
        };
        for (int position = 0; position < static_cast<int>(code.size()); ++position) {
            const auto &ins = code[position];
            if (ins.op == IR::Mov) {
                touch(ins.x, position);
                touch(ins.y, position);
                if (isVirtual(ins.x) && ins.y.size() == 2 && ins.y[0] == 'a' && ins.y[1] >= '0' &&
                    ins.y[1] <= '7')
                    incomingRegisters[ins.x] = ins.y;
            } else if (ins.op == IR::Bin) {
                touch(ins.x, position);
                touch(ins.y, position);
                touch(ins.z, position);
            } else if (ins.op == IR::Load)
                touch(ins.x, position);
            else if (ins.op == IR::Store || ins.op == IR::BrZ)
                touch(ins.op == IR::Store ? ins.y : ins.x, position);
            else if (ins.op == IR::Call) {
                touch(ins.x, position);
                for (const auto &arg : callArgs(ins.y))
                    touch(arg, position);
            }
        }

        std::vector<Interval> intervals;
        for (const auto &[value, range] : ranges)
            intervals.push_back({value, range.first, range.second});
        std::sort(intervals.begin(), intervals.end(),
                  [](const auto &a, const auto &b) { return a.begin < b.begin; });

        const std::array<std::string, 11> temporaries = {"t4", "t5", "t6", "a0", "a1", "a2",
                                                         "a3", "a4", "a5", "a6", "a7"};
        const std::array<std::string, 11> saved = {"s1", "s2", "s3", "s4",  "s5", "s6",
                                                   "s7", "s8", "s9", "s10", "s11"};
        struct Active {
            int end;
            std::string reg;
        };
        std::vector<Active> active;
        std::unordered_set<std::string> free;
        free.insert(temporaries.begin(), temporaries.end());
        free.insert(saved.begin(), saved.end());
        std::unordered_map<std::string, std::string> allocation;
        const auto liveAcrossCalls = valuesLiveAcrossCalls(code);
        for (const auto &interval : intervals) {
            for (auto it = active.begin(); it != active.end();) {
                if (it->end < interval.begin) {
                    free.insert(it->reg);
                    it = active.erase(it);
                } else
                    ++it;
            }
            const bool crossesCall = liveAcrossCalls.contains(interval.value);
            std::string reg;
            if (!crossesCall) {
                if (auto incoming = incomingRegisters.find(interval.value);
                    incoming != incomingRegisters.end() && free.contains(incoming->second))
                    reg = incoming->second;
            }
            if (!crossesCall && reg.empty())
                for (const auto &candidate : temporaries)
                    if (free.contains(candidate)) {
                        reg = candidate;
                        break;
                    }
            if (reg.empty())
                for (const auto &candidate : saved)
                    if (free.contains(candidate)) {
                        reg = candidate;
                        break;
                    }
            if (reg.empty())
                continue;
            free.erase(reg);
            allocation[interval.value] = reg;
            active.push_back({interval.end, reg});
        }
        return allocation;
    }

    // 临时值和标签共享单调编号，确保整个编译单元内名称唯一。
    std::string tmp() {
        return "t" + std::to_string(next_++);
    }
    std::string label() {
        return ".L" + std::to_string(next_++);
    }
    void emit(IR x) {
        ir_.push_back(std::move(x));
    }

    // 按“当前块 -> 外层块 -> 全局”的顺序解析标识符。
    Value find(const std::string &n) {
        for (auto i = scopes_.rbegin(); i != scopes_.rend(); ++i)
            if (auto p = i->find(n); p != i->end())
                return p->second;
        if (auto p = globals_.find(n); p != globals_.end())
            return p->second;
        throw std::runtime_error("undefined identifier: " + n);
    }

    // 编译期计算纯常量二元表达式，是常量折叠和 const 初始化检查的基础。
    static int calc(const std::string &o, int a, int b) {
        if (o == "+")
            return fromBits(static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b));
        if (o == "-")
            return fromBits(static_cast<std::uint32_t>(a) - static_cast<std::uint32_t>(b));
        if (o == "*")
            return fromBits(static_cast<std::uint32_t>(a) * static_cast<std::uint32_t>(b));
        if (o == "/") {
            if (a == std::numeric_limits<std::int32_t>::min() && b == -1)
                return a;
            return a / b;
        }
        if (o == "%") {
            if (a == std::numeric_limits<std::int32_t>::min() && b == -1)
                return 0;
            return a % b;
        }
        if (o == "<")
            return a < b;
        if (o == ">")
            return a > b;
        if (o == "<=")
            return a <= b;
        if (o == ">=")
            return a >= b;
        if (o == "==")
            return a == b;
        if (o == "!=")
            return a != b;
        if (o == "!")
            return !b;
        if (o == "bool")
            return !!b;
        if (o == "&&")
            return !!a && !!b;
        if (o == "||")
            return !!a || !!b;
        throw std::runtime_error("unknown constant operator: " + o);
    }

    // 将一个表达式降低为 IR，并返回常量值或承载结果的虚拟临时量。
    Value ex(Expr *e) {
        if (e->kind == Expr::Num)
            return {true, e->value, {}};
        if (e->kind == Expr::Var) {
            auto v = find(e->name);
            if (v.constant)
                return v;
            if (opt_ && !v.v.empty() && v.v[0] == '@')
                return {false, 0, v.v};
            std::string t = tmp();
            emit({IR::Load, t, v.v});
            return {false, 0, t};
        }
        if (e->kind == Expr::Call) {
            // 实参先从左到右求值，再用逗号分隔的临时量列表编码到 Call IR。
            std::string a;
            for (auto &x : e->args) {
                auto q = ex(x.get());
                if (q.constant) {
                    std::string t = tmp();
                    emit({IR::Mov, t, {}, {}, {}, q.n});
                    a += t + ",";
                } else
                    a += q.v + ",";
            }
            std::string t = tmp();
            emit({IR::Call, t, a, {}, e->name});
            return {false, 0, t};
        }
        auto a = ex(e->a.get());
        if (e->kind == Expr::Unary) {
            // 常量一元运算直接折叠；运行期运算复用 Bin 指令表示。
            if (a.constant) {
                if (e->op == "-")
                    a.n = wrapNeg(a.n);
                else if (e->op == "!")
                    a.n = !a.n;
                return a;
            }
            if (e->op == "+")
                return a;
            std::string t = tmp();
            emit({IR::Bin, t, "0", a.v, e->op});
            return {false, 0, t};
        }
        if (e->op == "&&" || e->op == "||") {
            // 逻辑运算不能简单转成普通二元指令，必须用分支保证右侧按需执行。
            if (a.constant) {
                if ((e->op == "&&" && !a.n) || (e->op == "||" && a.n))
                    return {true, e->op == "||", {}};
                auto b = ex(e->b.get());
                if (b.constant)
                    return {true, !!b.n, {}};
                std::string t = tmp();
                emit({IR::Bin, t, "0", b.v, "bool"});
                return {false, 0, t};
            }
            std::string av = a.v;
            std::string t = tmp(), done = label();
            emit({IR::Mov, t, {}, {}, {}, e->op == "||" ? 1 : 0});
            if (e->op == "&&") {
                emit({IR::BrZ, av, done});
            } else {
                std::string rhs = label();
                emit({IR::BrZ, av, rhs});
                emit({IR::Jump, done});
                emit({IR::Label, rhs});
            }
            auto b = ex(e->b.get());
            std::string bv = b.constant ? tmp() : b.v;
            if (b.constant)
                emit({IR::Mov, bv, {}, {}, {}, b.n});
            emit({IR::Bin, t, "0", bv, "bool"});
            emit({IR::Label, done});
            return {false, 0, t};
        }
        auto b = ex(e->b.get());
        if (a.constant && b.constant) {
            // 合法输入不会除零，但在编译期发现时仍给出明确诊断。
            if ((e->op == "/" || e->op == "%") && !b.n)
                throw std::runtime_error("division by zero");
            return {true, calc(e->op, a.n, b.n), {}};
        }
        std::string av = a.constant ? tmp() : a.v, bv = b.constant ? tmp() : b.v;
        if (a.constant)
            emit({IR::Mov, av, {}, {}, {}, a.n});
        if (b.constant)
            emit({IR::Mov, bv, {}, {}, {}, b.n});
        std::string t = tmp();
        emit({IR::Bin, t, av, bv, e->op});
        return {false, 0, t};
    }

    // 逐条降低语句。该函数通过递归处理块、分支和循环的嵌套结构。
    void st(Stmt *s) {
        if (s->kind == Stmt::Empty)
            return;
        if (s->kind == Stmt::Block) {
            // 每个语句块拥有独立符号表，退出块时统一销毁该层名字。
            scopes_.emplace_back();
            for (auto &x : s->body)
                st(x.get());
            scopes_.pop_back();
            return;
        }
        if (s->kind == Stmt::Decl) {
            auto v = ex(s->expr.get());
            if (s->isConst) {
                // const 不分配运行期槽位，后续引用会直接传播常量值。
                if (!v.constant)
                    throw std::runtime_error("const initializer is not constant");
                scopes_.back()[s->name] = v;
                return;
            }
            std::string loc = "@l" + std::to_string(next_++), src = v.constant ? tmp() : v.v;
            // 可变局部量必须保存在槽位中，循环回边才能观察到最新赋值。
            if (v.constant)
                emit({IR::Mov, src, {}, {}, {}, v.n});
            emit({IR::Store, loc, src});
            scopes_.back()[s->name] = {false, 0, loc};
            return;
        }
        if (s->kind == Stmt::Assign) {
            auto old = find(s->name);
            if (old.constant)
                throw std::runtime_error("assignment to const");
            auto v = ex(s->expr.get());
            std::string src = v.constant ? tmp() : v.v;
            if (v.constant)
                emit({IR::Mov, src, {}, {}, {}, v.n});
            emit({IR::Store, old.v, src});
            return;
        }
        if (s->kind == Stmt::ExprStmt) {
            ex(s->expr.get());
            return;
        }
        if (s->kind == Stmt::Return) {
            // RISC-V ABI 规定整数返回值放在 a0。
            auto v = s->expr ? ex(s->expr.get()) : Value{true, 0, {}};
            if (v.constant)
                emit({IR::Mov, "a0", {}, {}, {}, v.n});
            else
                emit({IR::Mov, "a0", v.v});
            emit({IR::Ret});
            return;
        }
        if (s->kind == Stmt::Break || s->kind == Stmt::Continue) {
            if (loops_.empty())
                throw std::runtime_error("loop control outside loop");
            emit({IR::Jump, s->kind == Stmt::Break ? loops_.back().first : loops_.back().second});
            return;
        }
        if (s->kind == Stmt::If) {
            auto c = ex(s->cond.get());
            if (opt_ && c.constant) {
                // -opt 模式下只生成确定会执行的分支，省去不可达基本块。
                if (c.n)
                    st(s->thenS.get());
                else if (s->elseS)
                    st(s->elseS.get());
                return;
            }
            std::string no = label(), done = label(), cv = c.constant ? tmp() : c.v;
            if (c.constant)
                emit({IR::Mov, cv, {}, {}, {}, c.n});
            emit({IR::BrZ, cv, no});
            st(s->thenS.get());
            if (s->elseS)
                emit({IR::Jump, done});
            emit({IR::Label, no});
            if (s->elseS) {
                st(s->elseS.get());
                emit({IR::Label, done});
            }
            return;
        }
        if (s->kind == Stmt::While) {
            // while 被展开为 head 标签、条件分支、循环体、回跳和 done 标签。
            std::string head = label(), done = label();
            emit({IR::Label, head});
            auto c = ex(s->cond.get());
            std::string cv = c.constant ? tmp() : c.v;
            if (c.constant)
                emit({IR::Mov, cv, {}, {}, {}, c.n});
            emit({IR::BrZ, cv, done});
            loops_.push_back({done, head});
            st(s->thenS.get());
            loops_.pop_back();
            emit({IR::Jump, head});
            emit({IR::Label, done});
        }
    }

  public:
    explicit Generator(bool o) : opt_(o) {
    }
    void generate(Program &p) {
        // 全局初始化必须在编译期可求值；常量只进入符号表，变量进入数据段。
        for (auto &d : p.globals) {
            auto v = ex(d.init.get());
            if (!v.constant)
                throw std::runtime_error("global initializer is not constant");
            if (d.isConst)
                globals_[d.name] = {true, v.n, {}};
            else {
                globals_[d.name] = {false, 0, "$" + d.name};
                data_.push_back({d.name, v.n});
            }
        }
        for (auto &f : p.funcs) {
            // 每个函数从函数标签开始，并建立包含形参的最外层局部作用域。
            emit({IR::Label, f->name});
            scopes_.emplace_back();
            for (size_t i = 0; i < f->params.size(); ++i) {
                // 前八个参数来自 a0-a7，其余参数从调用者栈帧读取。
                std::string loc = "@p" + std::to_string(next_++);
                emit({IR::Store, loc,
                      i < 8 ? "a" + std::to_string(i) : "arg" + std::to_string(i - 8)});
                scopes_.back()[f->params[i]] = {false, 0, loc};
            }
            for (auto &s : f->body)
                st(s.get());
            if (f->ret == "void")
                emit({IR::Ret});
            scopes_.pop_back();
        }
        if (opt_) {
            promoteLocalSlots();
            eliminateCommonSubexpressions();
            simplifyIR();
            simplifyControlFlow();
            propagateConstants();
            simplifyControlFlow();
            eliminateCommonSubexpressions();
            simplifyIR();
            eliminateDeadDefinitions();
            coalesceAdjacentMoves();
            simplifyIR();
            eliminateDeadDefinitions();
        }
    }

    // 逐函数分配寄存器和栈帧，并直接把 IR 目标写入最终物理寄存器。
    void output(std::ostream &o) {
        if (!data_.empty()) {
            o << ".data\n";
            for (auto &[n, v] : data_)
                o << ".globl " << n << "\n" << n << ":\n  .word " << v << "\n";
        }
        o << ".text\n.globl main\n";

        for (size_t begin = 0; begin < ir_.size();) {
            size_t end = begin + 1;
            while (end < ir_.size() &&
                   !(ir_[end].op == IR::Label && !ir_[end].x.empty() && ir_[end].x[0] != '.'))
                ++end;
            std::vector<IR> code(ir_.begin() + begin, ir_.begin() + end);
            const auto constants = immediateConstants(code);
            auto allocation = opt_ ? allocateRegisters(code, constants)
                                   : std::unordered_map<std::string, std::string>{};
            const bool hasCall = std::any_of(code.begin(), code.end(),
                                             [](const IR &ins) { return ins.op == IR::Call; });
            std::unordered_set<std::string> savedSet;
            for (const auto &[value, reg] : allocation)
                if (!reg.empty() && reg[0] == 's')
                    savedSet.insert(reg);
            std::vector<std::string> saved(savedSet.begin(), savedSet.end());
            std::sort(saved.begin(), saved.end());

            std::unordered_map<std::string, int> slot;
            int slotCount = 0;
            auto addSlot = [&](const std::string &value) {
                if (isVirtual(value) && !allocation.contains(value) && !constants.contains(value) &&
                    !slot.contains(value))
                    slot[value] = slotCount++;
            };
            for (const auto &ins : code) {
                addSlot(ins.x);
                if (ins.op == IR::Mov || ins.op == IR::Bin || ins.op == IR::Load ||
                    ins.op == IR::Store)
                    addSlot(ins.y);
                if (ins.op == IR::Bin)
                    addSlot(ins.z);
                if (ins.op == IR::Call)
                    for (const auto &arg : callArgs(ins.y))
                        addSlot(arg);
                if (ins.op == IR::BrZ)
                    addSlot(ins.x);
            }

            const int saveCount = static_cast<int>(saved.size()) + (hasCall ? 1 : 0);
            const int frame = ((slotCount + saveCount) * 4 + 15) & ~15;
            int saveCursor = frame;
            int raOffset = -1;
            if (hasCall) {
                saveCursor -= 4;
                raOffset = saveCursor;
            }
            std::unordered_map<std::string, int> savedOffset;
            for (const auto &reg : saved) {
                saveCursor -= 4;
                savedOffset[reg] = saveCursor;
            }

            auto stackMemory = [&](const char *op, const std::string &reg, int offset) {
                if (offset >= -2048 && offset <= 2047)
                    o << "  " << op << " " << reg << ", " << offset << "(sp)\n";
                else
                    o << "  li t3, " << offset << "\n  add t3, sp, t3\n  " << op << " " << reg
                      << ", 0(t3)\n";
            };
            auto adjustStack = [&](int amount) {
                if (!amount)
                    return;
                if (amount >= -2048 && amount <= 2047)
                    o << "  addi sp, sp, " << amount << "\n";
                else
                    o << "  li t3, " << amount << "\n  add sp, sp, t3\n";
            };
            auto constant = [&](const std::string &value) -> std::optional<int> {
                if (auto found = constants.find(value); found != constants.end())
                    return found->second;
                return std::nullopt;
            };
            auto reg = [&](const std::string &value, const char *scratch) {
                if (value.empty())
                    return std::string(scratch);
                if (value == "0")
                    return std::string("zero");
                if (auto imm = constant(value)) {
                    o << "  li " << scratch << ", " << *imm << "\n";
                    return std::string(scratch);
                }
                if (isVirtual(value)) {
                    if (auto found = allocation.find(value); found != allocation.end())
                        return found->second;
                    stackMemory("lw", scratch, slot[value] * 4);
                    return std::string(scratch);
                }
                if (value.rfind("arg", 0) == 0) {
                    stackMemory("lw", scratch, frame + std::stoi(value.substr(3)) * 4);
                    return std::string(scratch);
                }
                return value;
            };
            auto destination = [&](const std::string &value, const char *scratch) {
                if (auto found = allocation.find(value); found != allocation.end())
                    return found->second;
                if (isVirtual(value))
                    return std::string(scratch);
                return value;
            };
            auto put = [&](const std::string &value, const std::string &source) {
                if (isVirtual(value)) {
                    if (auto found = allocation.find(value); found != allocation.end()) {
                        if (found->second != source)
                            o << "  mv " << found->second << ", " << source << "\n";
                    } else
                        stackMemory("sw", source, slot[value] * 4);
                } else if (!value.empty() && value != source)
                    o << "  mv " << value << ", " << source << "\n";
            };
            auto epilogue = [&] {
                for (const auto &savedReg : saved)
                    stackMemory("lw", savedReg, savedOffset[savedReg]);
                if (hasCall)
                    stackMemory("lw", "ra", raOffset);
                adjustStack(frame);
                o << "  ret\n";
            };

            std::unordered_map<std::string, int> valueUses;
            auto countUse = [&](const std::string &value) {
                if (isVirtual(value))
                    ++valueUses[value];
            };
            for (const auto &candidate : code) {
                if (candidate.op == IR::Mov)
                    countUse(candidate.y);
                else if (candidate.op == IR::Bin) {
                    countUse(candidate.y);
                    countUse(candidate.z);
                } else if (candidate.op == IR::Load)
                    countUse(candidate.y);
                else if (candidate.op == IR::Store)
                    countUse(candidate.y);
                else if (candidate.op == IR::Call)
                    for (const auto &arg : callArgs(candidate.y))
                        countUse(arg);
                else if (candidate.op == IR::BrZ)
                    countUse(candidate.x);
            }

            for (size_t position = 0; position < code.size(); ++position) {
                const auto &ins = code[position];
                if (opt_ && ins.op == IR::Bin && position + 1 < code.size() &&
                    code[position + 1].op == IR::BrZ && code[position + 1].x == ins.x &&
                    valueUses[ins.x] == 1) {
                    const std::string &targetLabel = code[position + 1].y;
                    if (ins.aux == "bool" || ins.aux == "!") {
                        const auto value = reg(ins.z, "t1");
                        o << "  " << (ins.aux == "bool" ? "beqz" : "bnez") << " " << value << ", "
                          << targetLabel << "\n";
                        ++position;
                        continue;
                    }
                    const std::unordered_map<std::string, std::string> inverseBranch = {
                        {"<", "bge"},  {">", "ble"},  {"<=", "bgt"},
                        {">=", "blt"}, {"==", "bne"}, {"!=", "beq"}};
                    if (auto branch = inverseBranch.find(ins.aux); branch != inverseBranch.end()) {
                        const auto left = reg(ins.y, "t1");
                        const auto right = reg(ins.z, "t2");
                        o << "  " << branch->second << " " << left << ", " << right << ", "
                          << targetLabel << "\n";
                        ++position;
                        continue;
                    }
                }
                switch (ins.op) {
                case IR::Label:
                    o << ins.x << ":\n";
                    if (position == 0) {
                        adjustStack(-frame);
                        if (hasCall)
                            stackMemory("sw", "ra", raOffset);
                        for (const auto &savedReg : saved)
                            stackMemory("sw", savedReg, savedOffset[savedReg]);
                    }
                    break;
                case IR::Mov: {
                    if (ins.y.empty() && constants.contains(ins.x))
                        break;
                    const std::string target = destination(ins.x, "t0");
                    if (ins.y.empty())
                        o << "  li " << target << ", " << ins.imm << "\n";
                    else if (auto imm = constant(ins.y))
                        o << "  li " << target << ", " << *imm << "\n";
                    else {
                        const std::string source = reg(ins.y, "t1");
                        if (target != source)
                            o << "  mv " << target << ", " << source << "\n";
                    }
                    put(ins.x, target);
                    break;
                }
                case IR::Bin: {
                    const std::string target = destination(ins.x, "t0");
                    const auto leftImm = constant(ins.y);
                    const auto rightImm = constant(ins.z);
                    bool emitted = false;
                    if (ins.aux == "+" && rightImm && *rightImm >= -2048 && *rightImm <= 2047) {
                        const auto left = reg(ins.y, "t1");
                        o << "  addi " << target << ", " << left << ", " << *rightImm << "\n";
                        emitted = true;
                    } else if (ins.aux == "+" && leftImm && *leftImm >= -2048 && *leftImm <= 2047) {
                        const auto right = reg(ins.z, "t2");
                        o << "  addi " << target << ", " << right << ", " << *leftImm << "\n";
                        emitted = true;
                    } else if (ins.aux == "-" && rightImm && *rightImm >= -2047 &&
                               *rightImm <= 2048) {
                        const auto left = reg(ins.y, "t1");
                        o << "  addi " << target << ", " << left << ", " << -*rightImm << "\n";
                        emitted = true;
                    } else if ((ins.aux == "==" || ins.aux == "!=") &&
                               ((leftImm && *leftImm == 0) || (rightImm && *rightImm == 0))) {
                        const auto value = reg(leftImm ? ins.z : ins.y, "t1");
                        o << "  " << (ins.aux == "==" ? "seqz" : "snez") << " " << target << ", "
                          << value << "\n";
                        emitted = true;
                    } else if (ins.aux == "<" && rightImm && *rightImm >= -2048 &&
                               *rightImm <= 2047) {
                        const auto left = reg(ins.y, "t1");
                        o << "  slti " << target << ", " << left << ", " << *rightImm << "\n";
                        emitted = true;
                    } else if (ins.aux == "*" && (leftImm || rightImm)) {
                        const int factor = leftImm ? *leftImm : *rightImm;
                        const auto source = reg(leftImm ? ins.z : ins.y, "t1");
                        const std::uint32_t magnitude =
                            factor < 0 ? 0U - static_cast<std::uint32_t>(factor)
                                       : static_cast<std::uint32_t>(factor);
                        if (magnitude && (magnitude & (magnitude - 1)) == 0) {
                            int shift = 0;
                            for (std::uint32_t value = magnitude; value > 1; value >>= 1)
                                ++shift;
                            o << "  slli " << target << ", " << source << ", " << shift << "\n";
                            if (factor < 0)
                                o << "  neg " << target << ", " << target << "\n";
                            emitted = true;
                        } else if (magnitude == 3 || magnitude == 5 || magnitude == 7 ||
                                   magnitude == 9) {
                            const int shift = magnitude == 3 ? 1 : magnitude == 5 ? 2 : 3;
                            o << "  slli t2, " << source << ", " << shift << "\n  "
                              << (magnitude == 7 ? "sub " : "add ") << target << ", "
                              << (magnitude == 7 ? "t2, " + source : source + ", t2") << "\n";
                            if (factor < 0)
                                o << "  neg " << target << ", " << target << "\n";
                            emitted = true;
                        }
                    } else if ((ins.aux == "/" || ins.aux == "%") && rightImm && *rightImm != 0) {
                        const int divisor = *rightImm;
                        const std::uint32_t magnitude =
                            divisor < 0 ? 0U - static_cast<std::uint32_t>(divisor)
                                        : static_cast<std::uint32_t>(divisor);
                        if (magnitude > 1 && (magnitude & (magnitude - 1)) == 0) {
                            int shift = 0;
                            for (std::uint32_t value = magnitude; value > 1; value >>= 1)
                                ++shift;
                            const auto source = reg(ins.y, "t1");
                            o << "  srai t2, " << source << ", 31\n"
                              << "  srli t2, t2, " << (32 - shift) << "\n"
                              << "  add t2, " << source << ", t2\n"
                              << "  srai t2, t2, " << shift << "\n";
                            if (ins.aux == "/") {
                                if (divisor < 0)
                                    o << "  neg " << target << ", t2\n";
                                else if (target != "t2")
                                    o << "  mv " << target << ", t2\n";
                            } else {
                                o << "  slli t2, t2, " << shift << "\n"
                                  << "  sub " << target << ", " << source << ", t2\n";
                            }
                            emitted = true;
                        }
                    }
                    if (!emitted) {
                        const auto left = reg(ins.y, "t1");
                        const auto right = reg(ins.z, "t2");
                        if (ins.aux == "!")
                            o << "  seqz " << target << ", " << right << "\n";
                        else if (ins.aux == "bool")
                            o << "  snez " << target << ", " << right << "\n";
                        else if (ins.aux == ">")
                            o << "  slt " << target << ", " << right << ", " << left << "\n";
                        else if (ins.aux == "<=")
                            o << "  slt " << target << ", " << right << ", " << left << "\n  xori "
                              << target << ", " << target << ", 1\n";
                        else if (ins.aux == ">=")
                            o << "  slt " << target << ", " << left << ", " << right << "\n  xori "
                              << target << ", " << target << ", 1\n";
                        else if (ins.aux == "==")
                            o << "  sub " << target << ", " << left << ", " << right << "\n  seqz "
                              << target << ", " << target << "\n";
                        else if (ins.aux == "!=")
                            o << "  sub " << target << ", " << left << ", " << right << "\n  snez "
                              << target << ", " << target << "\n";
                        else
                            o << "  "
                              << (ins.aux == "+"   ? "add"
                                  : ins.aux == "-" ? "sub"
                                  : ins.aux == "*" ? "mul"
                                  : ins.aux == "/" ? "div"
                                  : ins.aux == "%" ? "rem"
                                                   : "slt")
                              << " " << target << ", " << left << ", " << right << "\n";
                    }
                    put(ins.x, target);
                    break;
                }
                case IR::Call: {
                    const auto args = callArgs(ins.y);
                    const int extra =
                        args.size() > 8 ? ((static_cast<int>(args.size() - 8) * 4 + 15) & ~15) : 0;
                    // 栈上传递的参数先写出，避免随后重排 a0-a7 时覆盖它们的源寄存器。
                    for (size_t k = 8; k < args.size(); ++k) {
                        const auto source = reg(args[k], "t0");
                        stackMemory("sw", source, -extra + static_cast<int>(k - 8) * 4);
                    }

                    struct RegisterMove {
                        std::string target;
                        std::string source;
                    };
                    std::vector<RegisterMove> moves;
                    struct DeferredArgument {
                        std::string target;
                        std::string value;
                    };
                    std::vector<DeferredArgument> deferred;
                    for (size_t k = 0; k < std::min<size_t>(8, args.size()); ++k) {
                        const std::string target = "a" + std::to_string(k);
                        if (constant(args[k])) {
                            deferred.push_back({target, args[k]});
                        } else if (auto found = allocation.find(args[k]);
                                   found != allocation.end()) {
                            if (found->second != target)
                                moves.push_back({target, found->second});
                        } else if (isVirtual(args[k])) {
                            deferred.push_back({target, args[k]});
                        } else if (args[k] != target) {
                            moves.push_back({target, args[k]});
                        }
                    }

                    // 按并行拷贝语义重排参数寄存器；环用保留的 t3 临时打破。
                    while (!moves.empty()) {
                        auto ready = moves.end();
                        for (auto candidate = moves.begin(); candidate != moves.end();
                             ++candidate) {
                            const bool targetIsSource =
                                std::any_of(moves.begin(), moves.end(), [&](const auto &move) {
                                    return move.source == candidate->target;
                                });
                            if (!targetIsSource) {
                                ready = candidate;
                                break;
                            }
                        }
                        if (ready == moves.end()) {
                            const std::string savedTarget = moves.front().target;
                            o << "  mv t3, " << savedTarget << "\n";
                            for (auto &move : moves)
                                if (move.source == savedTarget)
                                    move.source = "t3";
                            continue;
                        }
                        o << "  mv " << ready->target << ", " << ready->source << "\n";
                        moves.erase(ready);
                    }
                    for (const auto &argument : deferred) {
                        if (auto imm = constant(argument.value))
                            o << "  li " << argument.target << ", " << *imm << "\n";
                        else
                            stackMemory("lw", argument.target, slot[argument.value] * 4);
                    }
                    adjustStack(-extra);
                    o << "  call " << ins.aux << "\n";
                    adjustStack(extra);
                    put(ins.x, "a0");
                    break;
                }
                case IR::Store:
                    if (!ins.x.empty() && ins.x[0] == '$') {
                        const auto source = reg(ins.y, "t0");
                        o << "  la t1, " << ins.x.substr(1) << "\n  sw " << source << ", 0(t1)\n";
                    } else {
                        const auto source = reg(ins.y, "t0");
                        put(ins.x, source);
                    }
                    break;
                case IR::Load: {
                    const auto target = destination(ins.x, "t0");
                    if (!ins.y.empty() && ins.y[0] == '$')
                        o << "  la t1, " << ins.y.substr(1) << "\n  lw " << target << ", 0(t1)\n";
                    else {
                        const auto source = reg(ins.y, "t1");
                        if (target != source)
                            o << "  mv " << target << ", " << source << "\n";
                    }
                    put(ins.x, target);
                    break;
                }
                case IR::BrZ: {
                    const auto condition = reg(ins.x, "t0");
                    o << "  beqz " << condition << ", " << ins.y << "\n";
                    break;
                }
                case IR::Jump:
                    o << "  j " << ins.x << "\n";
                    break;
                case IR::Ret:
                    epilogue();
                    break;
                }
            }
            begin = end;
        }
    }
};
void compile(Program &p, std::ostream &o, bool optimize) {
    Generator g(optimize);
    g.generate(p);
    g.output(o);
}
} // namespace toyc
