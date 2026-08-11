#include "compiler.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
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

struct BasicBlock {
    size_t begin{};
    size_t end{};
    std::vector<size_t> predecessors;
    std::vector<size_t> successors;
};

// 所有函数级数据流分析共享同一份基本块 CFG、支配关系和自然循环深度。
struct FunctionCFG {
    static constexpr size_t noBlock = std::numeric_limits<size_t>::max();
    std::vector<BasicBlock> blocks;
    std::vector<size_t> instructionBlock;
    std::unordered_map<std::string, size_t> labelBlock;
    std::vector<bool> reachable;
    std::vector<std::unordered_set<size_t>> dominators;
    std::vector<size_t> immediateDominator;
    std::vector<std::vector<size_t>> dominatorChildren;
    std::vector<std::unordered_set<size_t>> dominanceFrontier;
    std::vector<int> loopDepth;
};

static FunctionCFG buildFunctionCFG(const std::vector<IR> &code) {
    FunctionCFG cfg;
    if (code.empty())
        return cfg;

    std::vector<bool> leader(code.size());
    leader[0] = true;
    for (size_t i = 0; i < code.size(); ++i) {
        if (code[i].op == IR::Label)
            leader[i] = true;
        if ((code[i].op == IR::Jump || code[i].op == IR::BrZ || code[i].op == IR::Ret) &&
            i + 1 < code.size())
            leader[i + 1] = true;
    }
    std::vector<size_t> starts;
    for (size_t i = 0; i < code.size(); ++i)
        if (leader[i])
            starts.push_back(i);
    cfg.instructionBlock.resize(code.size());
    for (size_t i = 0; i < starts.size(); ++i) {
        const size_t end = i + 1 < starts.size() ? starts[i + 1] : code.size();
        cfg.blocks.push_back({starts[i], end, {}, {}});
        for (size_t position = starts[i]; position < end; ++position) {
            cfg.instructionBlock[position] = i;
            if (code[position].op == IR::Label)
                cfg.labelBlock[code[position].x] = i;
        }
    }

    auto addSuccessor = [&](size_t from, size_t to) {
        if (to >= cfg.blocks.size())
            return;
        auto &successors = cfg.blocks[from].successors;
        if (std::find(successors.begin(), successors.end(), to) == successors.end())
            successors.push_back(to);
    };
    for (size_t block = 0; block < cfg.blocks.size(); ++block) {
        const auto &range = cfg.blocks[block];
        const IR &tail = code[range.end - 1];
        if (tail.op == IR::Jump) {
            if (auto target = cfg.labelBlock.find(tail.x); target != cfg.labelBlock.end())
                addSuccessor(block, target->second);
        } else if (tail.op == IR::BrZ) {
            if (auto target = cfg.labelBlock.find(tail.y); target != cfg.labelBlock.end())
                addSuccessor(block, target->second);
            addSuccessor(block, block + 1);
        } else if (tail.op != IR::Ret) {
            addSuccessor(block, block + 1);
        }
    }
    for (size_t block = 0; block < cfg.blocks.size(); ++block)
        for (size_t successor : cfg.blocks[block].successors)
            cfg.blocks[successor].predecessors.push_back(block);

    const size_t count = cfg.blocks.size();
    cfg.reachable.assign(count, false);
    std::vector<size_t> work = {0};
    while (!work.empty()) {
        const size_t block = work.back();
        work.pop_back();
        if (cfg.reachable[block])
            continue;
        cfg.reachable[block] = true;
        work.insert(work.end(), cfg.blocks[block].successors.begin(),
                    cfg.blocks[block].successors.end());
    }

    std::unordered_set<size_t> allReachable;
    for (size_t block = 0; block < count; ++block)
        if (cfg.reachable[block])
            allReachable.insert(block);
    cfg.dominators.resize(count);
    for (size_t block = 0; block < count; ++block)
        if (cfg.reachable[block])
            cfg.dominators[block] = block == 0 ? std::unordered_set<size_t>{0} : allReachable;
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t block = 1; block < count; ++block) {
            if (!cfg.reachable[block])
                continue;
            std::unordered_set<size_t> merged;
            bool first = true;
            for (size_t predecessor : cfg.blocks[block].predecessors) {
                if (!cfg.reachable[predecessor])
                    continue;
                if (first) {
                    merged = cfg.dominators[predecessor];
                    first = false;
                } else {
                    for (auto it = merged.begin(); it != merged.end();) {
                        if (!cfg.dominators[predecessor].contains(*it))
                            it = merged.erase(it);
                        else
                            ++it;
                    }
                }
            }
            merged.insert(block);
            if (merged != cfg.dominators[block]) {
                cfg.dominators[block] = std::move(merged);
                changed = true;
            }
        }
    }

    cfg.immediateDominator.assign(count, FunctionCFG::noBlock);
    cfg.dominatorChildren.resize(count);
    for (size_t block = 1; block < count; ++block) {
        if (!cfg.reachable[block])
            continue;
        size_t best = FunctionCFG::noBlock;
        for (size_t dominator : cfg.dominators[block]) {
            if (dominator == block)
                continue;
            if (best == FunctionCFG::noBlock ||
                cfg.dominators[dominator].size() > cfg.dominators[best].size())
                best = dominator;
        }
        cfg.immediateDominator[block] = best;
        if (best != FunctionCFG::noBlock)
            cfg.dominatorChildren[best].push_back(block);
    }

    cfg.dominanceFrontier.resize(count);
    for (size_t block = 0; block < count; ++block) {
        if (!cfg.reachable[block] || cfg.blocks[block].predecessors.size() < 2)
            continue;
        for (size_t predecessor : cfg.blocks[block].predecessors) {
            size_t runner = predecessor;
            while (runner != cfg.immediateDominator[block] && runner != FunctionCFG::noBlock) {
                cfg.dominanceFrontier[runner].insert(block);
                runner = cfg.immediateDominator[runner];
            }
        }
    }

    cfg.loopDepth.assign(count, 0);
    for (size_t latch = 0; latch < count; ++latch) {
        for (size_t header : cfg.blocks[latch].successors) {
            if (!cfg.dominators[latch].contains(header))
                continue;
            std::unordered_set<size_t> loop = {header, latch};
            std::vector<size_t> reverseWork;
            if (latch != header)
                reverseWork.push_back(latch);
            while (!reverseWork.empty()) {
                const size_t block = reverseWork.back();
                reverseWork.pop_back();
                for (size_t predecessor : cfg.blocks[block].predecessors)
                    if (loop.insert(predecessor).second)
                        reverseWork.push_back(predecessor);
            }
            for (size_t block : loop)
                ++cfg.loopDepth[block];
        }
    }
    return cfg;
}

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

    static bool definesVirtualValue(const IR &ins) {
        return isVirtual(ins.x) &&
               (ins.op == IR::Mov || ins.op == IR::Bin || ins.op == IR::Load ||
                ins.op == IR::Call || (ins.op == IR::Store && isVirtual(ins.x)));
    }

    static void rewriteUses(IR &ins, const std::function<void(std::string &)> &rewrite) {
        if (ins.op == IR::Mov) {
            if (!ins.y.empty())
                rewrite(ins.y);
        } else if (ins.op == IR::Bin) {
            rewrite(ins.y);
            rewrite(ins.z);
        } else if (ins.op == IR::Load || ins.op == IR::Store) {
            rewrite(ins.y);
        } else if (ins.op == IR::Call) {
            auto args = callArgs(ins.y);
            for (auto &arg : args)
                rewrite(arg);
            ins.y = joinArgs(args);
        } else if (ins.op == IR::BrZ) {
            rewrite(ins.x);
        }
    }

    static int fromBits(std::uint32_t value) {
        return static_cast<int>(std::bit_cast<std::int32_t>(value));
    }

    static int wrapNeg(int value) {
        return fromBits(0U - static_cast<std::uint32_t>(value));
    }

    struct SignedDivisionMagic {
        int multiplier{};
        int shift{};
    };

    // Hacker's Delight 的有符号常量除法 magic-number 算法；divisor 必须为正且大于 1。
    static SignedDivisionMagic signedDivisionMagic(std::uint32_t divisor) {
        const std::uint64_t two31 = std::uint64_t{1} << 31;
        const std::uint64_t anc = two31 - 1 - (two31 - 1) % divisor;
        int precision = 31;
        std::uint64_t quotient1 = two31 / anc;
        std::uint64_t remainder1 = two31 - quotient1 * anc;
        std::uint64_t quotient2 = two31 / divisor;
        std::uint64_t remainder2 = two31 - quotient2 * divisor;
        std::uint64_t delta;
        do {
            ++precision;
            quotient1 *= 2;
            remainder1 *= 2;
            if (remainder1 >= anc) {
                ++quotient1;
                remainder1 -= anc;
            }
            quotient2 *= 2;
            remainder2 *= 2;
            if (remainder2 >= divisor) {
                ++quotient2;
                remainder2 -= divisor;
            }
            delta = divisor - remainder2;
        } while (quotient1 < delta || (quotient1 == delta && remainder1 == 0));
        return {fromBits(static_cast<std::uint32_t>(quotient2 + 1)), precision - 32};
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

    // ToyC 没有指针和 volatile；全局量可在函数内缓存，只需在相关调用和返回边界同步。
    void promoteGlobals() {
        struct Effects {
            std::unordered_set<std::string> directReads;
            std::unordered_set<std::string> directWrites;
            std::unordered_set<std::string> reads;
            std::unordered_set<std::string> writes;
            std::vector<std::string> callees;
        };

        std::unordered_map<std::string, Effects> effects;
        std::string function;
        for (const auto &ins : ir_) {
            if (ins.op == IR::Label && !ins.x.empty() && ins.x[0] != '.') {
                function = ins.x;
                effects.try_emplace(function);
            } else if (!function.empty() && ins.op == IR::Load && !ins.y.empty() &&
                       ins.y[0] == '$') {
                effects[function].directReads.insert(ins.y);
            } else if (!function.empty() && ins.op == IR::Store && !ins.x.empty() &&
                       ins.x[0] == '$') {
                effects[function].directWrites.insert(ins.x);
            } else if (!function.empty() && ins.op == IR::Call) {
                effects[function].callees.push_back(ins.aux);
            }
        }
        for (auto &[name, effect] : effects) {
            effect.reads = effect.directReads;
            effect.writes = effect.directWrites;
        }

        // 递归调用图通过不动点传播；调用点据此只同步被被调函数真正访问的全局量。
        bool effectsChanged = true;
        while (effectsChanged) {
            effectsChanged = false;
            for (auto &[name, effect] : effects) {
                for (const auto &callee : effect.callees) {
                    auto found = effects.find(callee);
                    if (found == effects.end())
                        continue;
                    for (const auto &global : found->second.reads)
                        effectsChanged |= effect.reads.insert(global).second;
                    for (const auto &global : found->second.writes)
                        effectsChanged |= effect.writes.insert(global).second;
                }
            }
        }

        std::vector<IR> result;
        for (size_t begin = 0; begin < ir_.size();) {
            size_t end = begin + 1;
            while (end < ir_.size() &&
                   !(ir_[end].op == IR::Label && !ir_[end].x.empty() && ir_[end].x[0] != '.'))
                ++end;

            const std::string &name = ir_[begin].x;
            const auto effect = effects.find(name);
            if (effect == effects.end() ||
                (effect->second.directReads.empty() && effect->second.directWrites.empty())) {
                result.insert(result.end(), ir_.begin() + begin, ir_.begin() + end);
                begin = end;
                continue;
            }

            std::vector<std::string> globals(effect->second.directReads.begin(),
                                             effect->second.directReads.end());
            for (const auto &global : effect->second.directWrites)
                if (std::find(globals.begin(), globals.end(), global) == globals.end())
                    globals.push_back(global);
            std::sort(globals.begin(), globals.end());
            std::unordered_map<std::string, std::string> shadow;
            for (const auto &global : globals)
                shadow[global] = "@g" + std::to_string(next_++);

            result.push_back(ir_[begin]);
            for (const auto &global : globals)
                result.emplace_back(IR::Load, shadow[global], global);

            auto flushReturn = [&] {
                for (const auto &global : globals)
                    if (effect->second.directWrites.contains(global))
                        result.emplace_back(IR::Store, global, shadow[global]);
            };
            for (size_t i = begin + 1; i < end; ++i) {
                IR ins = ir_[i];
                // 返回值写 a0 后不能再读取可能分配到 a0 的全局缓存，故先完成写回。
                if (ins.op == IR::Mov && ins.x == "a0" && i + 1 < end && ir_[i + 1].op == IR::Ret)
                    flushReturn();

                if (ins.op == IR::Load && shadow.contains(ins.y)) {
                    ins.op = IR::Mov;
                    ins.y = shadow[ins.y];
                } else if (ins.op == IR::Store && shadow.contains(ins.x)) {
                    ins.op = IR::Mov;
                    ins.x = shadow[ins.x];
                } else if (ins.op == IR::Call) {
                    const auto callee = effects.find(ins.aux);
                    const bool unknown = callee == effects.end();
                    for (const auto &global : globals) {
                        const bool mayAccess = unknown || callee->second.reads.contains(global) ||
                                               callee->second.writes.contains(global);
                        if (mayAccess && effect->second.directWrites.contains(global))
                            result.emplace_back(IR::Store, global, shadow[global]);
                    }
                    result.push_back(std::move(ins));
                    for (const auto &global : globals)
                        if (unknown || callee->second.writes.contains(global))
                            result.emplace_back(IR::Load, shadow[global], global);
                    continue;
                } else if (ins.op == IR::Ret &&
                           !(i > begin + 1 && ir_[i - 1].op == IR::Mov && ir_[i - 1].x == "a0")) {
                    flushReturn();
                }
                result.push_back(std::move(ins));
            }
            begin = end;
        }
        ir_ = std::move(result);
    }

    // 将自然循环中仅依赖循环外值的纯计算移到循环头之前，嵌套循环由内向外反复处理。
    void hoistLoopInvariants() {
        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t begin = 0; begin < ir_.size() && !changed;) {
                size_t end = begin + 1;
                while (end < ir_.size() &&
                       !(ir_[end].op == IR::Label && !ir_[end].x.empty() && ir_[end].x[0] != '.'))
                    ++end;

                std::unordered_map<std::string, size_t> labels;
                std::unordered_map<std::string, int> definitions;
                for (size_t i = begin; i < end; ++i) {
                    const auto &ins = ir_[i];
                    if (ins.op == IR::Label)
                        labels[ins.x] = i;
                    const bool defines = ins.op == IR::Mov || ins.op == IR::Bin ||
                                         ins.op == IR::Load || ins.op == IR::Call ||
                                         (ins.op == IR::Store && isVirtual(ins.x));
                    if (defines && isVirtual(ins.x))
                        ++definitions[ins.x];
                }

                std::unordered_map<size_t, size_t> loopTails;
                for (size_t i = begin; i < end; ++i) {
                    const auto &ins = ir_[i];
                    const std::string *target = ins.op == IR::Jump  ? &ins.x
                                                : ins.op == IR::BrZ ? &ins.y
                                                                    : nullptr;
                    if (!target)
                        continue;
                    auto found = labels.find(*target);
                    if (found != labels.end() && found->second <= i)
                        loopTails[found->second] = std::max(loopTails[found->second], i);
                }
                std::vector<std::pair<size_t, size_t>> loops(loopTails.begin(), loopTails.end());
                std::sort(loops.begin(), loops.end(), [](const auto &left, const auto &right) {
                    return left.second - left.first < right.second - right.first;
                });

                for (const auto &[header, tail] : loops) {
                    std::unordered_set<std::string> definedInside;
                    for (size_t i = header; i <= tail; ++i) {
                        const auto &ins = ir_[i];
                        const bool defines = ins.op == IR::Mov || ins.op == IR::Bin ||
                                             ins.op == IR::Load || ins.op == IR::Call ||
                                             (ins.op == IR::Store && isVirtual(ins.x));
                        if (defines && isVirtual(ins.x))
                            definedInside.insert(ins.x);
                    }

                    std::unordered_set<std::string> invariant;
                    std::vector<size_t> candidates;
                    auto operandInvariant = [&](const std::string &value) {
                        return !isVirtual(value) || !definedInside.contains(value) ||
                               invariant.contains(value);
                    };
                    for (size_t i = header + 1; i <= tail; ++i) {
                        const auto &ins = ir_[i];
                        if (ins.op == IR::Bin && !ins.x.empty() && ins.x[0] == 't' &&
                            definitions[ins.x] == 1 && operandInvariant(ins.y) &&
                            operandInvariant(ins.z)) {
                            invariant.insert(ins.x);
                            candidates.push_back(i);
                        }
                    }
                    if (candidates.empty())
                        continue;

                    std::unordered_set<size_t> selected(candidates.begin(), candidates.end());
                    std::vector<IR> rewritten;
                    rewritten.reserve(ir_.size());
                    for (size_t i = 0; i < ir_.size(); ++i) {
                        if (i == header)
                            for (size_t candidate : candidates)
                                rewritten.push_back(std::move(ir_[candidate]));
                        if (!selected.contains(i))
                            rewritten.push_back(std::move(ir_[i]));
                    }
                    ir_ = std::move(rewritten);
                    changed = true;
                    break;
                }
                begin = end;
            }
        }
    }

    // 将简单线性归纳变量乘以循环常量改写为累加器，减少热循环中的乘法。
    // 仅处理单一回边、单一入口和单一递增定义，复杂循环继续保留原始表达式。
    void strengthReduceLoops() {
        bool changed = true;
        while (changed) {
            changed = false;
            for (size_t begin = 0; begin < ir_.size() && !changed;) {
                size_t end = begin + 1;
                while (end < ir_.size() &&
                       !(ir_[end].op == IR::Label && !ir_[end].x.empty() && ir_[end].x[0] != '.'))
                    ++end;
                std::vector<IR> code(ir_.begin() + begin, ir_.begin() + end);
                const auto cfg = buildFunctionCFG(code);
                if (cfg.blocks.empty()) {
                    begin = end;
                    continue;
                }
                const auto constants = immediateConstants(code);

                for (size_t latch = 0; latch < cfg.blocks.size() && !changed; ++latch) {
                    for (size_t header : cfg.blocks[latch].successors) {
                        if (header == latch || !cfg.dominators[latch].contains(header))
                            continue;
                        std::unordered_set<size_t> loopBlocks = {header, latch};
                        std::vector<size_t> reverseWork = {latch};
                        while (!reverseWork.empty()) {
                            const size_t block = reverseWork.back();
                            reverseWork.pop_back();
                            for (size_t predecessor : cfg.blocks[block].predecessors)
                                if (loopBlocks.insert(predecessor).second && predecessor != header)
                                    reverseWork.push_back(predecessor);
                        }
                        std::vector<size_t> outside;
                        for (size_t predecessor : cfg.blocks[header].predecessors)
                            if (!loopBlocks.contains(predecessor))
                                outside.push_back(predecessor);
                        if (outside.size() != 1)
                            continue;
                        const size_t preheader = outside.front();
                        const auto &loopRange = cfg.blocks[header];
                        const auto &latchRange = cfg.blocks[latch];
                        if (loopRange.begin >= code.size() || latchRange.end > code.size())
                            continue;

                        struct Induction {
                            std::string value;
                            std::string stepValue;
                            int step{};
                            size_t update{};
                        } induction;
                        int updates = 0;
                        for (size_t i = loopRange.begin; i + 1 < latchRange.end; ++i) {
                            const auto &add = code[i];
                            const auto &move = code[i + 1];
                            if (add.op != IR::Bin || add.aux != "+" || !isVirtual(add.x) ||
                                move.op != IR::Mov || !isVirtual(move.x) || move.x.empty() ||
                                move.y != add.x)
                                continue;
                            std::string stepValue;
                            if (constants.contains(add.y) && isVirtual(add.z)) {
                                stepValue = add.y;
                            } else if (constants.contains(add.z) && isVirtual(add.y)) {
                                stepValue = add.z;
                            } else {
                                continue;
                            }
                            const std::string base = move.x;
                            if ((add.y != base && add.z != base) ||
                                (add.y != base && add.y != stepValue) ||
                                (add.z != base && add.z != stepValue))
                                continue;
                            if (++updates > 1)
                                break;
                            induction = {base, stepValue, constants.at(stepValue), i + 1};
                        }
                        if (updates != 1)
                            continue;

                        // 初值必须在循环外唯一确定，避免给多个入口合成错误的初始累加器。
                        int baseDefinitions = 0;
                        for (size_t i = 0; i < loopRange.begin; ++i)
                            if (definesVirtualValue(code[i]) && code[i].x == induction.value)
                                ++baseDefinitions;
                        if (baseDefinitions != 1)
                            continue;

                        std::vector<std::pair<size_t, bool>> multiplications;
                        for (size_t i = loopRange.begin; i < latchRange.end; ++i) {
                            const auto &ins = code[i];
                            if (ins.op != IR::Bin || ins.aux != "*")
                                continue;
                            std::string factor;
                            if (ins.y == induction.value && constants.contains(ins.z))
                                factor = ins.z;
                            else if (ins.z == induction.value && constants.contains(ins.y))
                                factor = ins.y;
                            if (factor.empty())
                                continue;
                            const int multiplier = constants.at(factor);
                            const auto magnitude = multiplier < 0
                                                       ? 0U - static_cast<std::uint32_t>(multiplier)
                                                       : static_cast<std::uint32_t>(multiplier);
                            const bool powerOfTwo = magnitude && (magnitude & (magnitude - 1)) == 0;
                            if (multiplier == 0 || multiplier == 1 || powerOfTwo ||
                                multiplier == 3 || multiplier == 5 || multiplier == 7 ||
                                multiplier == 9)
                                continue;
                            // 记录归纳变量是否位于左操作数，替换时只改写该操作数。
                            multiplications.emplace_back(i, ins.y == induction.value);
                        }
                        if (multiplications.empty())
                            continue;

                        const bool baseIsLeft = multiplications.front().second;
                        const int delta =
                            fromBits(static_cast<std::uint32_t>(induction.step) *
                                     static_cast<std::uint32_t>(constants.at(
                                         baseIsLeft ? code[multiplications.front().first].z
                                                    : code[multiplications.front().first].y)));
                        const std::string factor = baseIsLeft
                                                       ? code[multiplications.front().first].z
                                                       : code[multiplications.front().first].y;
                        if (std::any_of(multiplications.begin(), multiplications.end(),
                                        [&](const auto &entry) {
                                            const auto &ins = code[entry.first];
                                            const std::string current =
                                                entry.second ? ins.z : ins.y;
                                            return current != factor;
                                        }))
                            continue;

                        const std::string accumulator = tmp();
                        const std::string deltaValue = tmp();
                        IR initial(IR::Bin, accumulator, induction.value, factor, "*");
                        IR deltaInstruction(IR::Mov, deltaValue, {}, {}, {}, delta);
                        IR update(IR::Bin, accumulator, accumulator, deltaValue, "+");

                        std::vector<IR> rewritten;
                        rewritten.reserve(code.size() + 3);
                        const auto &preheaderRange = cfg.blocks[preheader];
                        for (size_t i = 0; i < code.size(); ++i) {
                            if (i == preheaderRange.end - 1 &&
                                (code[i].op == IR::Jump || code[i].op == IR::BrZ)) {
                                rewritten.push_back(std::move(deltaInstruction));
                                rewritten.push_back(std::move(initial));
                            }
                            if (i == induction.update) {
                                rewritten.push_back(std::move(code[i]));
                                rewritten.push_back(update);
                                continue;
                            }
                            bool replaced = false;
                            for (const auto &entry : multiplications)
                                if (entry.first == i) {
                                    IR replacement = std::move(code[i]);
                                    replacement.op = IR::Mov;
                                    replacement.y = accumulator;
                                    replacement.z.clear();
                                    replacement.aux.clear();
                                    rewritten.push_back(std::move(replacement));
                                    replaced = true;
                                    break;
                                }
                            if (!replaced)
                                rewritten.push_back(std::move(code[i]));
                        }
                        if (preheaderRange.end == code.size() ||
                            (code[preheaderRange.end - 1].op != IR::Jump &&
                             code[preheaderRange.end - 1].op != IR::BrZ)) {
                            // 落空前驱块没有显式终结跳转时，初始化必须追加到其末尾。
                            const size_t insertAt = preheaderRange.end;
                            rewritten.insert(rewritten.begin() +
                                                 static_cast<std::ptrdiff_t>(insertAt),
                                             {std::move(deltaInstruction), std::move(initial)});
                        }
                        std::vector<IR> next;
                        next.reserve(ir_.size() - code.size() + rewritten.size());
                        next.insert(next.end(), ir_.begin(), ir_.begin() + begin);
                        next.insert(next.end(), std::make_move_iterator(rewritten.begin()),
                                    std::make_move_iterator(rewritten.end()));
                        next.insert(next.end(), ir_.begin() + end, ir_.end());
                        ir_ = std::move(next);
                        changed = true;
                        break;
                    }
                }
                begin = end;
            }
        }
    }

    // 按自然循环深度形成热路径 trace；重排后显式补齐被打断的落空边。
    void layoutHotBlocks() {
        std::vector<IR> laidOut;
        for (size_t begin = 0; begin < ir_.size();) {
            size_t end = begin + 1;
            while (end < ir_.size() &&
                   !(ir_[end].op == IR::Label && !ir_[end].x.empty() && ir_[end].x[0] != '.'))
                ++end;
            std::vector<IR> code(ir_.begin() + begin, ir_.begin() + end);
            const auto cfg = buildFunctionCFG(code);
            if (cfg.blocks.size() < 2) {
                laidOut.insert(laidOut.end(), std::make_move_iterator(code.begin()),
                               std::make_move_iterator(code.end()));
                begin = end;
                continue;
            }

            std::vector<size_t> order;
            std::vector<bool> placed(cfg.blocks.size(), false);
            size_t current = 0;
            while (order.size() < cfg.blocks.size()) {
                if (!placed[current]) {
                    placed[current] = true;
                    order.push_back(current);
                }
                size_t best = FunctionCFG::noBlock;
                int bestScore = std::numeric_limits<int>::min();
                for (size_t successor : cfg.blocks[current].successors) {
                    if (placed[successor])
                        continue;
                    const int fallthroughBonus = successor == current + 1 ? 8 : 0;
                    const int score = cfg.loopDepth[successor] * 100 + fallthroughBonus;
                    if (best == FunctionCFG::noBlock || score > bestScore) {
                        best = successor;
                        bestScore = score;
                    }
                }
                if (best == FunctionCFG::noBlock) {
                    for (size_t candidate = 0; candidate < cfg.blocks.size(); ++candidate)
                        if (!placed[candidate]) {
                            best = candidate;
                            break;
                        }
                }
                current = best;
            }

            std::vector<std::string> blockLabels(cfg.blocks.size());
            for (size_t block = 0; block < cfg.blocks.size(); ++block)
                for (size_t i = cfg.blocks[block].begin; i < cfg.blocks[block].end; ++i)
                    if (code[i].op == IR::Label) {
                        blockLabels[block] = code[i].x;
                        break;
                    }

            for (size_t position = 0; position < order.size(); ++position) {
                const size_t block = order[position];
                const auto &range = cfg.blocks[block];
                const IR::Op tailOp = code[range.end - 1].op;
                laidOut.insert(laidOut.end(), std::make_move_iterator(code.begin() + range.begin),
                               std::make_move_iterator(code.begin() + range.end));
                const size_t next =
                    position + 1 < order.size() ? order[position + 1] : FunctionCFG::noBlock;
                if (tailOp == IR::BrZ) {
                    const size_t fallthrough =
                        block + 1 < cfg.blocks.size() ? block + 1 : FunctionCFG::noBlock;
                    if (fallthrough != FunctionCFG::noBlock && next != fallthrough)
                        laidOut.emplace_back(IR::Jump, blockLabels[fallthrough]);
                } else if (tailOp != IR::Jump && tailOp != IR::Ret) {
                    const size_t fallthrough =
                        block + 1 < cfg.blocks.size() ? block + 1 : FunctionCFG::noBlock;
                    if (fallthrough != FunctionCFG::noBlock && next != fallthrough)
                        laidOut.emplace_back(IR::Jump, blockLabels[fallthrough]);
                }
            }
            begin = end;
        }
        ir_ = std::move(laidOut);
    }

    // 在函数基本块上构造轻量 SSA，执行支配树 GVN 后再用关键边拆分消除 phi。
    std::vector<IR> optimizeSSAFunction(const std::vector<IR> &code) {
        struct PhiNode {
            std::string base;
            std::string result;
            std::unordered_map<size_t, std::string> incoming;
            bool removed{};
        };
        struct SSABlock {
            std::vector<IR> instructions;
            std::vector<bool> removed;
            std::vector<PhiNode> phis;
            std::string label;
        };
        const auto cfg = buildFunctionCFG(code);
        if (cfg.blocks.empty())
            return code;

        std::vector<SSABlock> blocks(cfg.blocks.size());
        std::unordered_map<std::string, std::unordered_set<size_t>> definitionBlocks;
        std::unordered_map<std::string, int> definitionCount;
        for (size_t block = 0; block < cfg.blocks.size(); ++block) {
            const auto &range = cfg.blocks[block];
            blocks[block].instructions.assign(code.begin() + range.begin, code.begin() + range.end);
            if (!blocks[block].instructions.empty() &&
                blocks[block].instructions.front().op == IR::Label) {
                blocks[block].label = blocks[block].instructions.front().x;
            } else {
                blocks[block].label = label();
                blocks[block].instructions.insert(blocks[block].instructions.begin(),
                                                  {IR::Label, blocks[block].label});
            }
            for (const auto &ins : blocks[block].instructions)
                if (definesVirtualValue(ins)) {
                    definitionBlocks[ins.x].insert(block);
                    ++definitionCount[ins.x];
                }
        }

        std::vector<std::string> candidates;
        for (const auto &[value, count] : definitionCount)
            if ((!value.empty() && value[0] == '@') || count > 1)
                candidates.push_back(value);
        std::sort(candidates.begin(), candidates.end());

        // Cytron 支配边界算法：仅为真正多路径汇合的可变值插入 phi。
        for (const auto &value : candidates) {
            std::vector<size_t> work(definitionBlocks[value].begin(),
                                     definitionBlocks[value].end());
            std::unordered_set<size_t> queued(work.begin(), work.end());
            std::unordered_set<size_t> hasPhi;
            while (!work.empty()) {
                const size_t block = work.back();
                work.pop_back();
                std::vector<size_t> frontier(cfg.dominanceFrontier[block].begin(),
                                             cfg.dominanceFrontier[block].end());
                std::sort(frontier.begin(), frontier.end());
                for (size_t destination : frontier) {
                    if (!cfg.reachable[destination] || !hasPhi.insert(destination).second)
                        continue;
                    blocks[destination].phis.push_back({value, {}, {}, false});
                    if (!definitionBlocks[value].contains(destination) &&
                        queued.insert(destination).second)
                        work.push_back(destination);
                }
            }
        }
        for (auto &block : blocks)
            std::sort(block.phis.begin(), block.phis.end(),
                      [](const auto &left, const auto &right) { return left.base < right.base; });

        std::unordered_map<std::string, int> counters;
        std::unordered_map<std::string, std::vector<std::string>> stacks;
        std::unordered_set<std::string> candidateSet(candidates.begin(), candidates.end());
        auto fresh = [&](const std::string &base) {
            return base + "#" + std::to_string(counters[base]++);
        };
        std::function<void(size_t)> renameBlock = [&](size_t block) {
            std::vector<std::string> pushed;
            for (auto &phi : blocks[block].phis) {
                phi.result = fresh(phi.base);
                stacks[phi.base].push_back(phi.result);
                pushed.push_back(phi.base);
            }
            for (auto &ins : blocks[block].instructions) {
                rewriteUses(ins, [&](std::string &value) {
                    if (candidateSet.contains(value) && !stacks[value].empty())
                        value = stacks[value].back();
                });
                if (definesVirtualValue(ins) && candidateSet.contains(ins.x)) {
                    const std::string base = ins.x;
                    ins.x = fresh(base);
                    stacks[base].push_back(ins.x);
                    pushed.push_back(base);
                }
            }
            for (size_t successor : cfg.blocks[block].successors)
                for (auto &phi : blocks[successor].phis)
                    phi.incoming[block] =
                        stacks[phi.base].empty() ? phi.base : stacks[phi.base].back();
            auto children = cfg.dominatorChildren[block];
            std::sort(children.begin(), children.end());
            for (size_t child : children)
                renameBlock(child);
            for (auto it = pushed.rbegin(); it != pushed.rend(); ++it)
                stacks[*it].pop_back();
        };
        renameBlock(0);

        std::unordered_map<std::string, std::string> aliases;
        auto resolve = [&](std::string value) {
            std::unordered_set<std::string> seen;
            while (aliases.contains(value) && seen.insert(value).second)
                value = aliases[value];
            return value;
        };
        auto simplifyPhis = [&] {
            bool changed = false;
            for (auto &block : blocks)
                for (auto &phi : block.phis) {
                    if (phi.removed)
                        continue;
                    std::string common;
                    bool same = true;
                    for (auto &[predecessor, value] : phi.incoming) {
                        value = resolve(value);
                        if (value == phi.result)
                            continue;
                        if (common.empty())
                            common = value;
                        else if (common != value)
                            same = false;
                    }
                    if (same && !common.empty()) {
                        aliases[phi.result] = common;
                        phi.removed = true;
                        changed = true;
                    }
                }
            return changed;
        };
        while (simplifyPhis()) {
        }

        const std::unordered_set<std::string> commutative = {"+", "*", "==", "!="};
        std::function<void(size_t, std::unordered_map<std::string, std::string>)> numberBlock;
        numberBlock = [&](size_t block, std::unordered_map<std::string, std::string> available) {
            blocks[block].removed.assign(blocks[block].instructions.size(), false);
            for (size_t i = 0; i < blocks[block].instructions.size(); ++i) {
                auto &ins = blocks[block].instructions[i];
                rewriteUses(ins, [&](std::string &value) { value = resolve(value); });
                if (ins.op == IR::Mov && isVirtual(ins.x) && isVirtual(ins.y)) {
                    aliases[ins.x] = resolve(ins.y);
                    blocks[block].removed[i] = true;
                } else if (ins.op == IR::Bin && isVirtual(ins.x)) {
                    std::string left = resolve(ins.y), right = resolve(ins.z);
                    if (commutative.contains(ins.aux) && right < left)
                        std::swap(left, right);
                    const std::string key = ins.aux + "\n" + left + "\n" + right;
                    if (auto found = available.find(key); found != available.end()) {
                        aliases[ins.x] = resolve(found->second);
                        blocks[block].removed[i] = true;
                    } else {
                        available[key] = ins.x;
                    }
                }
            }
            auto children = cfg.dominatorChildren[block];
            std::sort(children.begin(), children.end());
            for (size_t child : children)
                numberBlock(child, available);
        };
        numberBlock(0, {});
        while (simplifyPhis()) {
        }
        for (auto &block : blocks) {
            for (auto &ins : block.instructions)
                rewriteUses(ins, [&](std::string &value) { value = resolve(value); });
            for (auto &phi : block.phis)
                for (auto &[predecessor, value] : phi.incoming)
                    value = resolve(value);
        }

        using Edge = std::pair<size_t, size_t>;
        std::map<Edge, std::vector<std::pair<std::string, std::string>>> edgeMoves;
        for (size_t block = 0; block < blocks.size(); ++block)
            for (const auto &phi : blocks[block].phis) {
                if (phi.removed)
                    continue;
                for (size_t predecessor : cfg.blocks[block].predecessors) {
                    auto found = phi.incoming.find(predecessor);
                    if (found == phi.incoming.end())
                        continue;
                    const std::string source = resolve(found->second);
                    if (source != phi.result)
                        edgeMoves[{predecessor, block}].push_back({phi.result, source});
                }
            }

        auto parallelCopies = [&](std::vector<std::pair<std::string, std::string>> moves) {
            std::vector<IR> copies;
            while (!moves.empty()) {
                auto ready = moves.end();
                for (auto candidate = moves.begin(); candidate != moves.end(); ++candidate) {
                    const bool targetIsSource =
                        std::any_of(moves.begin(), moves.end(), [&](const auto &move) {
                            return move.second == candidate->first;
                        });
                    if (!targetIsSource) {
                        ready = candidate;
                        break;
                    }
                }
                if (ready == moves.end()) {
                    const std::string savedTarget = moves.front().first;
                    const std::string temporary = tmp();
                    copies.emplace_back(IR::Mov, temporary, savedTarget);
                    for (auto &move : moves)
                        if (move.second == savedTarget)
                            move.second = temporary;
                    continue;
                }
                copies.emplace_back(IR::Mov, ready->first, ready->second);
                moves.erase(ready);
            }
            return copies;
        };

        struct EdgeBlock {
            std::string label;
            std::string target;
            std::vector<IR> copies;
        };
        std::vector<EdgeBlock> splitEdges;
        std::vector<IR> result;
        for (size_t block = 0; block < blocks.size(); ++block) {
            std::vector<IR> instructions;
            for (size_t i = 0; i < blocks[block].instructions.size(); ++i)
                if (blocks[block].removed.empty() || !blocks[block].removed[i])
                    instructions.push_back(std::move(blocks[block].instructions[i]));
            if (instructions.empty())
                continue;

            if (cfg.blocks[block].successors.size() == 1) {
                const size_t successor = cfg.blocks[block].successors.front();
                auto found = edgeMoves.find({block, successor});
                if (found != edgeMoves.end()) {
                    auto copies = parallelCopies(found->second);
                    const bool beforeTerminator =
                        instructions.back().op == IR::Jump || instructions.back().op == IR::BrZ;
                    instructions.insert(beforeTerminator ? instructions.end() - 1
                                                         : instructions.end(),
                                        std::make_move_iterator(copies.begin()),
                                        std::make_move_iterator(copies.end()));
                }
            } else if (cfg.blocks[block].successors.size() > 1 &&
                       instructions.back().op == IR::BrZ) {
                const auto originalTarget = cfg.labelBlock.find(instructions.back().y);
                const size_t taken = originalTarget == cfg.labelBlock.end()
                                         ? FunctionCFG::noBlock
                                         : originalTarget->second;
                std::optional<std::string> fallthroughEdge;
                for (size_t successor : cfg.blocks[block].successors) {
                    auto found = edgeMoves.find({block, successor});
                    if (found == edgeMoves.end())
                        continue;
                    const std::string edgeLabel = label();
                    splitEdges.push_back(
                        {edgeLabel, blocks[successor].label, parallelCopies(found->second)});
                    if (successor == taken)
                        instructions.back().y = edgeLabel;
                    else
                        fallthroughEdge = edgeLabel;
                }
                if (fallthroughEdge)
                    instructions.emplace_back(IR::Jump, *fallthroughEdge);
            }
            result.insert(result.end(), std::make_move_iterator(instructions.begin()),
                          std::make_move_iterator(instructions.end()));
        }
        for (auto &edge : splitEdges) {
            result.emplace_back(IR::Label, edge.label);
            result.insert(result.end(), std::make_move_iterator(edge.copies.begin()),
                          std::make_move_iterator(edge.copies.end()));
            result.emplace_back(IR::Jump, edge.target);
        }
        return result;
    }

    void optimizeSSA() {
        std::vector<IR> result;
        for (size_t begin = 0; begin < ir_.size();) {
            size_t end = begin + 1;
            while (end < ir_.size() &&
                   !(ir_[end].op == IR::Label && !ir_[end].x.empty() && ir_[end].x[0] != '.'))
                ++end;
            std::vector<IR> code(ir_.begin() + begin, ir_.begin() + end);
            auto optimized = optimizeSSAFunction(code);
            result.insert(result.end(), std::make_move_iterator(optimized.begin()),
                          std::make_move_iterator(optimized.end()));
            begin = end;
        }
        ir_ = std::move(result);
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
            if (ins.op == IR::Mov && !ins.y.empty() && !ins.x.empty() && ins.x[0] == 't' &&
                definitions[ins.x] == 1 && uses[ins.x] == 1 && i + 1 < ir_.size()) {
                IR next = ir_[i + 1];
                bool replaced = false;
                auto replace = [&](std::string &value) {
                    if (value == ins.x) {
                        value = ins.y;
                        replaced = true;
                    }
                };
                if (next.op == IR::Mov)
                    replace(next.y);
                else if (next.op == IR::Bin) {
                    replace(next.y);
                    replace(next.z);
                } else if (next.op == IR::Load || next.op == IR::Store)
                    replace(next.y);
                else if (next.op == IR::Call) {
                    auto args = callArgs(next.y);
                    for (auto &arg : args)
                        replace(arg);
                    next.y = joinArgs(args);
                } else if (next.op == IR::BrZ)
                    replace(next.x);
                if (replaced) {
                    result.push_back(std::move(next));
                    ++i;
                    continue;
                }
            }
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
        const std::array<std::string, 11> temporaries = {"t4", "t5", "t6", "a0", "a1", "a2",
                                                         "a3", "a4", "a5", "a6", "a7"};
        const std::array<std::string, 11> saved = {"s1", "s2", "s3", "s4",  "s5", "s6",
                                                   "s7", "s8", "s9", "s10", "s11"};
        const size_t count = code.size();
        std::unordered_map<std::string, size_t> labels;
        for (size_t i = 0; i < count; ++i)
            if (code[i].op == IR::Label)
                labels[code[i].x] = i;

        std::vector<std::unordered_set<std::string>> uses(count), defs(count), liveIn(count),
            liveOut(count);
        std::unordered_set<std::string> values;
        std::unordered_map<std::string, long long> spillWeight;
        std::unordered_map<std::string, std::string> incomingRegisters;
        const auto cfg = buildFunctionCFG(code);
        std::vector<int> loopDepth(count);
        for (size_t i = 0; i < count; ++i)
            loopDepth[i] = cfg.loopDepth[cfg.instructionBlock[i]];

        // 只让循环内原本需要 li 的常量参与分配；可直接编码为立即数的常量仍按使用点折叠。
        std::unordered_map<std::string, long long> constantBenefit;
        std::unordered_map<std::string, int> useCount;
        auto countValue = [&](const std::string &value) {
            if (isVirtual(value))
                ++useCount[value];
        };
        for (const auto &ins : code) {
            if (ins.op == IR::Mov)
                countValue(ins.y);
            else if (ins.op == IR::Bin) {
                countValue(ins.y);
                countValue(ins.z);
            } else if (ins.op == IR::Load || ins.op == IR::Store)
                countValue(ins.y);
            else if (ins.op == IR::Call)
                for (const auto &arg : callArgs(ins.y))
                    countValue(arg);
            else if (ins.op == IR::BrZ)
                countValue(ins.x);
        }
        auto magnitude = [](int value) {
            return value < 0 ? 0U - static_cast<std::uint32_t>(value)
                             : static_cast<std::uint32_t>(value);
        };
        for (size_t i = 0; i < count; ++i) {
            const auto &ins = code[i];
            if (loopDepth[i] == 0 || ins.op != IR::Bin)
                continue;
            const bool fused = i + 1 < count && code[i + 1].op == IR::BrZ &&
                               code[i + 1].x == ins.x && useCount[ins.x] == 1;
            auto needsRegister = [&](const std::string &value, bool left) {
                auto found = constants.find(value);
                if (found == constants.end() || found->second == 0)
                    return false;
                const int immediate = found->second;
                if (fused && (ins.aux == "<" || ins.aux == ">" || ins.aux == "<=" ||
                              ins.aux == ">=" || ins.aux == "==" || ins.aux == "!="))
                    return true;
                if (ins.aux == "+")
                    return immediate < -2048 || immediate > 2047;
                if (ins.aux == "-" && !left)
                    return immediate < -2047 || immediate > 2048;
                if ((ins.aux == "==" || ins.aux == "!=") &&
                    ((constants.contains(ins.y) && constants.at(ins.y) == 0) ||
                     (constants.contains(ins.z) && constants.at(ins.z) == 0)))
                    return false;
                if (ins.aux == "<" && !left && immediate >= -2048 && immediate <= 2047)
                    return false;
                if (ins.aux == "*") {
                    const auto absolute = magnitude(immediate);
                    const bool powerOfTwo = absolute && (absolute & (absolute - 1)) == 0;
                    return !powerOfTwo && absolute != 3 && absolute != 5 && absolute != 7 &&
                           absolute != 9;
                }
                if ((ins.aux == "/" || ins.aux == "%") && !left) {
                    const auto absolute = magnitude(immediate);
                    if (absolute > 1 && (absolute & (absolute - 1)) == 0)
                        return false;
                }
                return true;
            };
            if (needsRegister(ins.y, true))
                constantBenefit[ins.y] += 1LL << std::min(loopDepth[i], 12);
            if (needsRegister(ins.z, false))
                constantBenefit[ins.z] += 1LL << std::min(loopDepth[i], 12);
        }
        std::unordered_set<std::string> registerConstants;
        for (const auto &[value, benefit] : constantBenefit)
            if (benefit > 1)
                registerConstants.insert(value);

        auto recordUse = [&](size_t i, const std::string &value) {
            if (!isVirtual(value) ||
                (constants.contains(value) && !registerConstants.contains(value)))
                return;
            uses[i].insert(value);
            values.insert(value);
            spillWeight[value] += 1LL << std::min(loopDepth[i], 12);
        };
        auto recordDef = [&](size_t i, const std::string &value) {
            if (!isVirtual(value) ||
                (constants.contains(value) && !registerConstants.contains(value)))
                return;
            defs[i].insert(value);
            values.insert(value);
            spillWeight[value] += 1LL << std::min(loopDepth[i], 12);
        };
        for (size_t i = 0; i < count; ++i) {
            const auto &ins = code[i];
            if (ins.op == IR::Mov) {
                recordDef(i, ins.x);
                recordUse(i, ins.y);
                if (isVirtual(ins.x) && ins.y.size() == 2 && ins.y[0] == 'a' && ins.y[1] >= '0' &&
                    ins.y[1] <= '7')
                    incomingRegisters[ins.x] = ins.y;
            } else if (ins.op == IR::Bin) {
                recordDef(i, ins.x);
                recordUse(i, ins.y);
                recordUse(i, ins.z);
            } else if (ins.op == IR::Load) {
                recordDef(i, ins.x);
                recordUse(i, ins.y);
            } else if (ins.op == IR::Store) {
                recordDef(i, ins.x);
                recordUse(i, ins.y);
            } else if (ins.op == IR::Call) {
                recordDef(i, ins.x);
                for (const auto &arg : callArgs(ins.y))
                    recordUse(i, arg);
            } else if (ins.op == IR::BrZ) {
                recordUse(i, ins.x);
            }
        }

        bool changed = true;
        while (changed) {
            changed = false;
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

        // 冲突图按真实 CFG 活跃集合建立，避免线性首末区间把循环中的空洞误判为长期占用。
        std::unordered_map<std::string, std::unordered_set<std::string>> interference;
        auto addClique = [&](const std::unordered_set<std::string> &valuesAtInstruction) {
            for (const auto &left : valuesAtInstruction)
                for (const auto &right : valuesAtInstruction)
                    if (left != right)
                        interference[left].insert(right);
        };
        for (size_t i = 0; i < count; ++i) {
            // 同一指令的多个源操作数必须同时可读；其余冲突由标准 def-liveOut 关系给出。
            addClique(uses[i]);
            for (const auto &defined : defs[i])
                for (const auto &live : liveOut[i])
                    if (defined != live) {
                        interference[defined].insert(live);
                        interference[live].insert(defined);
                    }
        }

        const auto liveAcrossCalls = valuesLiveAcrossCalls(code);
        std::vector<std::string> order(values.begin(), values.end());
        auto degree = [&](const std::string &value) {
            auto found = interference.find(value);
            return found == interference.end() ? size_t{0} : found->second.size();
        };
        std::sort(order.begin(), order.end(), [&](const auto &left, const auto &right) {
            const auto leftScore = spillWeight[left] * static_cast<long long>(degree(left) + 1);
            const auto rightScore = spillWeight[right] * static_cast<long long>(degree(right) + 1);
            if (leftScore != rightScore)
                return leftScore > rightScore;
            if (degree(left) != degree(right))
                return degree(left) > degree(right);
            return left < right;
        });

        std::unordered_map<std::string, std::string> allocation;
        // 形参入口拷贝按顺序发射；预着色到原 ABI 寄存器可防止提前覆盖尚未保存的后续实参。
        for (const auto &[value, reg] : incomingRegisters)
            if (!liveAcrossCalls.contains(value))
                allocation[value] = reg;
        for (const auto &value : order) {
            if (allocation.contains(value))
                continue;
            std::unordered_set<std::string> unavailable;
            if (auto neighbors = interference.find(value); neighbors != interference.end())
                for (const auto &neighbor : neighbors->second)
                    if (auto found = allocation.find(neighbor); found != allocation.end())
                        unavailable.insert(found->second);
            auto choose = [&](const auto &candidates) {
                for (const auto &candidate : candidates)
                    if (!unavailable.contains(candidate))
                        return candidate;
                return std::string{};
            };
            std::string reg;
            if (!liveAcrossCalls.contains(value))
                reg = choose(temporaries);
            if (reg.empty())
                reg = choose(saved);
            if (!reg.empty())
                allocation[value] = std::move(reg);
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
            promoteGlobals();
            promoteLocalSlots();
            eliminateCommonSubexpressions();
            simplifyIR();
            simplifyControlFlow();
            propagateConstants();
            simplifyControlFlow();
            eliminateCommonSubexpressions();
            simplifyIR();
            hoistLoopInvariants();
            strengthReduceLoops();
            eliminateCommonSubexpressions();
            simplifyIR();
            eliminateDeadDefinitions();
            optimizeSSA();
            simplifyIR();
            simplifyControlFlow();
            eliminateDeadDefinitions();
            coalesceAdjacentMoves();
            simplifyIR();
            eliminateDeadDefinitions();
            layoutHotBlocks();
            simplifyIR();
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
            auto constants = immediateConstants(code);
            // 多个控制流分支若都定义同一个立即数，重物化比保存临时值更便宜。
            std::unordered_map<std::string, int> rematerialized;
            std::unordered_set<std::string> rematerializationCandidates;
            std::unordered_set<std::string> invalidRematerialization;
            for (const auto &ins : code) {
                if (!definesVirtualValue(ins) || ins.x.empty() || ins.x[0] != 't')
                    continue;
                if (ins.op == IR::Mov && ins.y.empty() &&
                    !invalidRematerialization.contains(ins.x)) {
                    if (auto found = rematerialized.find(ins.x); found == rematerialized.end())
                        rematerialized[ins.x] = ins.imm;
                    else if (found->second != ins.imm)
                        invalidRematerialization.insert(ins.x);
                    rematerializationCandidates.insert(ins.x);
                } else {
                    invalidRematerialization.insert(ins.x);
                    rematerializationCandidates.erase(ins.x);
                }
            }
            for (const auto &value : rematerializationCandidates)
                if (!invalidRematerialization.contains(value))
                    constants[value] = rematerialized[value];
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
                if (auto found = allocation.find(value); found != allocation.end())
                    return found->second;
                if (auto imm = constant(value)) {
                    if (*imm == 0)
                        return std::string("zero");
                    o << "  li " << scratch << ", " << *imm << "\n";
                    return std::string(scratch);
                }
                if (isVirtual(value)) {
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
                    if (ins.y.empty() && constants.contains(ins.x) && !allocation.contains(ins.x))
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
                        const auto source = reg(ins.y, "t1");
                        if (magnitude == 1) {
                            if (ins.aux == "%")
                                o << "  li " << target << ", 0\n";
                            else if (divisor < 0)
                                o << "  neg " << target << ", " << source << "\n";
                            else if (target != source)
                                o << "  mv " << target << ", " << source << "\n";
                            emitted = true;
                        } else if ((magnitude & (magnitude - 1)) == 0) {
                            int shift = 0;
                            for (std::uint32_t value = magnitude; value > 1; value >>= 1)
                                ++shift;
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
                        } else if (opt_) {
                            // RV32IM 的 div/rem 通常远慢于 mulh；5-9 条定长序列更适合常量除数。
                            const auto magic = signedDivisionMagic(magnitude);
                            o << "  li t2, " << magic.multiplier << "\n"
                              << "  mulh t2, " << source << ", t2\n";
                            if (magic.multiplier < 0)
                                o << "  add t2, t2, " << source << "\n";
                            if (magic.shift)
                                o << "  srai t2, t2, " << magic.shift << "\n";
                            o << "  srli t3, t2, 31\n"
                              << "  add t2, t2, t3\n";
                            if (divisor < 0)
                                o << "  neg t2, t2\n";
                            if (ins.aux == "/") {
                                if (target != "t2")
                                    o << "  mv " << target << ", t2\n";
                            } else {
                                o << "  li t3, " << divisor << "\n"
                                  << "  mul t3, t2, t3\n"
                                  << "  sub " << target << ", " << source << ", t3\n";
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
                        if (auto found = allocation.find(args[k]); found != allocation.end()) {
                            if (found->second != target)
                                moves.push_back({target, found->second});
                        } else if (constant(args[k])) {
                            deferred.push_back({target, args[k]});
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
