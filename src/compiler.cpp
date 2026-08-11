#include "compiler.hpp"
#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
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

    // 代数化简、无用临时量删除和相邻跳转清理，均只处理无副作用 IR。
    void simplifyIR() {
        auto isTemp = [](const std::string &v) { return !v.empty() && v[0] == 't'; };
        std::unordered_map<std::string, int> constants;
        for (const auto &ins : ir_)
            if (ins.op == IR::Mov && ins.y.empty() && isTemp(ins.x))
                constants[ins.x] = ins.imm;
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
            } else if (ins.aux == "*" &&
                       (isConstant(ins.y, 0) || isConstant(ins.z, 0))) {
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
            if (live[i].op == IR::Jump && i + 1 < live.size() &&
                live[i + 1].op == IR::Label && live[i].x == live[i + 1].x)
                continue;
            compact.push_back(std::move(live[i]));
        }
        ir_ = std::move(compact);
    }

    // 基本块内局部值编号：同时复用未失效的 Load 和二元表达式。
    void eliminateCommonSubexpressions() {
        std::vector<IR> result;
        std::unordered_map<std::string, std::string> available;
        std::unordered_map<std::string, std::string> alias;
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
                std::string left = ins.y, right = ins.z;
                if (commutative.contains(ins.aux) && right < left)
                    std::swap(left, right);
                const std::string key = "bin\n" + ins.aux + "\n" + left + "\n" + right;
                if (auto found = available.find(key); found != available.end()) {
                    alias[ins.x] = resolve(found->second);
                    continue;
                }
                available[key] = ins.x;
            } else if (ins.op == IR::Mov || ins.op == IR::Call)
                forgetResult(ins.x);

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

    // 线性扫描分配 s1-s11；寄存器不足的临时量保留在栈中。
    std::unordered_map<std::string, std::string> allocateRegisters() const {
        struct Interval {
            std::string value;
            int begin;
            int end;
        };
        std::unordered_map<std::string, std::pair<int, int>> ranges;
        auto touch = [&](const std::string &value, int position) {
            if (value.empty() || value[0] != 't')
                return;
            auto [it, inserted] = ranges.emplace(value, std::pair{position, position});
            if (!inserted)
                it->second.second = position;
        };
        for (int position = 0; position < static_cast<int>(ir_.size()); ++position) {
            const auto &ins = ir_[position];
            if (ins.op == IR::Mov) {
                touch(ins.x, position);
                touch(ins.y, position);
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
        std::sort(intervals.begin(), intervals.end(), [](const auto &a, const auto &b) {
            return a.begin < b.begin;
        });

        const std::array<std::string, 11> registers = {"s1", "s2", "s3", "s4", "s5", "s6",
                                                       "s7", "s8", "s9", "s10", "s11"};
        struct Active {
            int end;
            std::string reg;
        };
        std::vector<Active> active;
        std::vector<std::string> free(registers.begin(), registers.end());
        std::unordered_map<std::string, std::string> allocation;
        for (const auto &interval : intervals) {
            for (auto it = active.begin(); it != active.end();) {
                if (it->end < interval.begin) {
                    free.push_back(it->reg);
                    it = active.erase(it);
                } else
                    ++it;
            }
            if (free.empty())
                continue;
            const std::string reg = free.back();
            free.pop_back();
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
            return a + b;
        if (o == "-")
            return a - b;
        if (o == "*")
            return a * b;
        if (o == "/")
            return a / b;
        if (o == "%")
            return a % b;
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
        if (o == "&&")
            return !!a && !!b;
        return !!a || !!b;
    }

    // 将一个表达式降低为 IR，并返回常量值或承载结果的虚拟临时量。
    Value ex(Expr *e) {
        if (e->kind == Expr::Num)
            return {true, e->value, {}};
        if (e->kind == Expr::Var) {
            auto v = find(e->name);
            if (v.constant)
                return v;
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
                    a.n = -a.n;
                else if (e->op == "!")
                    a.n = !a.n;
                return a;
            }
            std::string t = tmp();
            emit({IR::Bin, t, "0", a.v, e->op == "-" ? "-" : "!"});
            return {false, 0, t};
        }
        if (e->op == "&&" || e->op == "||") {
            // 逻辑运算不能简单转成普通二元指令，必须用分支保证右侧按需执行。
            if (a.constant && ((e->op == "&&" && !a.n) || (e->op == "||" && a.n)))
                return {true, e->op == "||", {}};
            std::string av = a.constant ? tmp() : a.v;
            if (a.constant)
                emit({IR::Mov, av, {}, {}, {}, a.n});
            std::string t = tmp(), done = label(), skip = label();
            emit({IR::Mov, t, {}, {}, {}, e->op == "||" ? 1 : 0});
            if (e->op == "&&")
                emit({IR::BrZ, av, skip});
            else {
                emit({IR::BrZ, av, skip});
                emit({IR::Jump, done});
            }
            auto b = ex(e->b.get());
            std::string bv = b.constant ? tmp() : b.v;
            if (b.constant)
                emit({IR::Mov, bv, {}, {}, {}, b.n});
            emit({IR::Bin, t, "0", bv, "!"});
            emit({IR::Jump, done});
            emit({IR::Label, skip});
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
        if (opt_)
            eliminateCommonSubexpressions();
        if (opt_)
            simplifyIR();
    }

    // 为全部虚拟值分配栈槽，再逐条翻译为 RV32I/M 汇编文本。
    void output(std::ostream &o) {
        if (!data_.empty()) {
            o << ".data\n";
            for (auto &[n, v] : data_)
                o << ".globl " << n << "\n" << n << ":\n  .word " << v << "\n";
        }
        o << ".text\n";
        auto allocation = opt_ ? allocateRegisters() : std::unordered_map<std::string, std::string>{};
        std::unordered_set<std::string> savedSet;
        for (const auto &[value, reg] : allocation)
            savedSet.insert(reg);
        std::vector<std::string> saved(savedSet.begin(), savedSet.end());
        std::sort(saved.begin(), saved.end());

        std::unordered_map<std::string, int> slot;
        int n = 0;
        // t* 是表达式临时量，@* 是局部变量/形参槽位；二者都落在当前栈帧。
        auto addSlot = [&](const std::string &value) {
            if (!value.empty() && (value[0] == '@' || (value[0] == 't' && !allocation.contains(value))) &&
                !slot.contains(value))
                slot[value] = n++;
        };
        for (const auto &i : ir_) {
            if (i.op == IR::Mov) {
                addSlot(i.x);
                addSlot(i.y);
            } else if (i.op == IR::Bin) {
                addSlot(i.x);
                addSlot(i.y);
                addSlot(i.z);
            } else if (i.op == IR::Load) {
                addSlot(i.x);
                addSlot(i.y);
            } else if (i.op == IR::Store) {
                addSlot(i.x);
                addSlot(i.y);
            } else if (i.op == IR::Call) {
                addSlot(i.x);
                for (const auto &arg : callArgs(i.y))
                    addSlot(arg);
            } else if (i.op == IR::BrZ)
                addSlot(i.x);
        }
        int frame = ((n + 1 + static_cast<int>(saved.size())) * 4 + 15) & ~15;
        int raOffset = frame - 4;
        // 栈帧按 ABI 要求对齐到 16 字节，并预留保存 ra 的位置。
        o << ".globl main\n";
        for (auto &i : ir_) {
            auto reg = [&](const std::string &s, const char *r) {
                // 把抽象操作数物化到寄存器；普通 ABI 寄存器名可直接返回。
                if (s.empty())
                    return std::string(r);
                if (s[0] == 't') {
                    if (auto found = allocation.find(s); found != allocation.end())
                        return found->second;
                    o << "  lw " << r << ", " << (slot[s] * 4) << "(sp)\n";
                    return std::string(r);
                }
                if (s.rfind("arg", 0) == 0) {
                    o << "  lw " << r << ", " << (frame + std::stoi(s.substr(3)) * 4) << "(sp)\n";
                    return std::string(r);
                }
                return s;
            };
            auto put = [&](const std::string &s, const char *r) {
                // 临时量写回栈槽，a0 等真实寄存器则直接使用 mv。
                if (!s.empty() && s[0] == 't') {
                    if (auto found = allocation.find(s); found != allocation.end()) {
                        if (found->second != r)
                            o << "  mv " << found->second << ", " << r << "\n";
                    } else
                        o << "  sw " << r << ", " << (slot[s] * 4) << "(sp)\n";
                }
                else if (!s.empty())
                    o << "  mv " << s << ", " << r << "\n";
            };
            switch (i.op) {
            case IR::Label:
                // 函数标签需要序言；以 . 开头的局部控制流标签不创建新栈帧。
                o << i.x << ":\n";
                if (i.x != "" && i.x[0] != '.') {
                    o << "  addi sp, sp, -" << frame << "\n  sw ra, " << raOffset << "(sp)\n";
                    for (size_t k = 0; k < saved.size(); ++k)
                        o << "  sw " << saved[k] << ", " << (raOffset - 4 - static_cast<int>(k) * 4)
                          << "(sp)\n";
                }
                break;
            case IR::Mov:
                if (i.y.empty())
                    o << "  li t0, " << i.imm << "\n";
                else {
                    auto q = reg(i.y, "t1");
                    o << "  mv t0, " << q << "\n";
                }
                put(i.x, "t0");
                break;
            case IR::Bin: {
                // 关系运算使用 slt/sub/seqz 等基础指令组合，结果规范化为 0 或 1。
                auto a = reg(i.y, "t1"), b = reg(i.z, "t2");
                if (i.aux == "!")
                    o << "  seqz t0, " << b << "\n";
                else if (i.aux == ">")
                    o << "  slt t0, " << b << ", " << a << "\n";
                else if (i.aux == "<=")
                    o << "  slt t0, " << b << ", " << a << "\n  xori t0, t0, 1\n";
                else if (i.aux == ">=")
                    o << "  slt t0, " << a << ", " << b << "\n  xori t0, t0, 1\n";
                else if (i.aux == "==")
                    o << "  sub t0, " << a << ", " << b << "\n  seqz t0, t0\n";
                else if (i.aux == "!=")
                    o << "  sub t0, " << a << ", " << b << "\n  snez t0, t0\n";
                else {
                    o << "  "
                      << (i.aux == "+"   ? "add"
                          : i.aux == "-" ? "sub"
                          : i.aux == "*" ? "mul"
                          : i.aux == "/" ? "div"
                          : i.aux == "%" ? "rem"
                                         : "slt")
                      << " t0, " << a << ", " << b << "\n";
                }
                put(i.x, "t0");
                break;
            }
            case IR::Call: {
                // 前八个实参进入 a0-a7，额外实参在调用前压入 16 字节对齐的区域。
                auto av = callArgs(i.y);
                int extra = av.size() > 8 ? ((int(av.size() - 8) * 4 + 15) & ~15) : 0;
                for (size_t k = 0; k < av.size(); ++k) {
                    auto q = reg(av[k], "t0");
                    if (k < 8)
                        o << "  mv a" << k << ", " << q << "\n";
                    else
                        o << "  sw " << q << ", " << (-extra + int(k - 8) * 4) << "(sp)\n";
                }
                if (extra)
                    o << "  addi sp, sp, -" << extra << "\n";
                o << "  call " << i.aux << "\n";
                if (extra)
                    o << "  addi sp, sp, " << extra << "\n";
                put(i.x, "a0");
                break;
            }
            case IR::Store:
                // $ 前缀代表全局符号，其余位置由当前函数的栈槽表解析。
                if (i.x[0] == '$') {
                    o << "  la t1, " << i.x.substr(1) << "\n";
                    auto q = reg(i.y, "t0");
                    o << "  sw " << q << ", 0(t1)\n";
                } else {
                    auto q = reg(i.y, "t0");
                    o << "  sw " << q << ", " << (slot[i.x] * 4) << "(sp)\n";
                }
                break;
            case IR::BrZ: {
                auto q = reg(i.x, "t0");
                o << "  beqz " << q << ", " << i.y << "\n";
                break;
            }
            case IR::Jump:
                o << "  j " << i.x << "\n";
                break;
            case IR::Ret:
                for (size_t k = 0; k < saved.size(); ++k)
                    o << "  lw " << saved[k] << ", " << (raOffset - 4 - static_cast<int>(k) * 4)
                      << "(sp)\n";
                // 统一恢复返回地址和栈指针，然后交还控制权。
                o << "  lw ra, " << raOffset << "(sp)\n  addi sp, sp, " << frame
                  << "\n  ret\n";
                break;
            case IR::Load:
                // 全局变量经 la 获得地址，局部值直接按 sp 偏移读取。
                if (i.y[0] == '$') {
                    o << "  la t1, " << i.y.substr(1) << "\n  lw t0, 0(t1)\n";
                } else
                    o << "  lw t0, " << (slot[i.y] * 4) << "(sp)\n";
                put(i.x, "t0");
                break;
            }
        }
    }
};
void compile(Program &p, std::ostream &o, bool optimize) {
    Generator g(optimize);
    g.generate(p);
    g.output(o);
}
} // namespace toyc
