#ifndef __PROGTEST__
    #include <algorithm>
    // #include <array>
    #include <cassert>
    // #include <cctype>
    // #include <cfloat>
    // #include <charconv>
    #include <climits>
    #include <cmath>
    #include <compare>
    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    // #include <fstream>
    // #include <functional>
    // #include <iomanip>
    #include <iostream>
    // #include <iterator>
    // #include <list>
    #include <map>
    #include <memory>
    // #include <optional>
    // #include <queue>
    #include <set>
    // #include <span>
    // #include <sstream>
    // #include <stack>
    #include <stdexcept>
    #include <string>
    // #include <unordered_map>
    // #include <unordered_set>
    #include <utility>
    #include <variant>
    #include <vector>

    #include "expression.h"
using namespace std::literals;

using CValue = std::variant<std::monostate, double, std::string>;

constexpr unsigned SPREADSHEET_CYCLIC_DEPS = 0x01;
constexpr unsigned SPREADSHEET_FUNCTIONS = 0x02;
constexpr unsigned SPREADSHEET_FILE_IO = 0x04;
constexpr unsigned SPREADSHEET_SPEED = 0x08;
constexpr unsigned SPREADSHEET_PARSER = 0x10;
#endif /* __PROGTEST__ */

constexpr CValue UNDEFINED = CValue();

enum class FunctionKind {
    SUM,  // (range)
    COUNT,  // (range)
    MIN,  // (range)
    MAX,  // (range)
    COUNT_VAL,  // (value, range)
    IF,  // (cond, ifTrue, ifFalse)
    // math
    POW,  // x^y
    MUL,  // x * y
    DIV,  // x / y
    ADD,  // x + y
    SUB,  // x - y
    NEG,  // -x
    // comparison
    LT,  // <
    LE,  // <=
    GT,  // >
    GE,  // >>
    NE,  // <> / !=
    EQ,  // = / ==
};

void parse_cell_position(
    std::string_view str,
    size_t& i,
    int& x,
    int& y,
    bool& x_is_absolute,
    bool& y_is_absolute
) {
    if (i < str.size() && str[i] == '$') {
        x_is_absolute = true;
        i++;
    }
    while (i < str.size() && 'A' <= str[i] && str[i] <= 'Z') {
        x *= 26;
        x += str[i] - 'A';
        i++;
    }
    if (i < str.size() && str[i] == '$') {
        y_is_absolute = true;
        i++;
    }
    while (i < str.size() && '0' <= str[i] && str[i] <= '9') {
        y *= 10;
        y += str[i] - '0';
        i++;
    }
}

class CPos {
  public:
    int x = 0;
    int y = 0;

    CPos() {}

    CPos(std::string_view str) {
        size_t i = 0;
        bool x_absolute = false;
        bool y_absolute = false;
        parse_cell_position(str, i, x, y, x_absolute, y_absolute);
        if (x_absolute || y_absolute) {
            throw std::invalid_argument("Absolute axis position in CPos");
        }
        if (i < str.size()) {
            throw std::invalid_argument("mlem");
        }
    }

    CPos(int x, int y) : x(x), y(y) {}

    static std::pair<int, int> make_relative_offset(CPos src, CPos dst) {
        int x = dst.x - src.x;
        int y = dst.y - src.y;
        return {x, y};
    }

    CPos operator+(std::pair<int, int> offset) const {
        return {
            x + offset.first,
            y + offset.second,
        };
    }

    std::strong_ordering operator<=>(const CPos& other) const = default;
};

struct CellReference {
    CPos pos {0, 0};
    bool x_absolute = false;
    bool y_absolute = false;

    CellReference() {}

    static void parse(CellReference& self, std::string_view str, size_t& i) {
        parse_cell_position(
            str,
            i,
            self.pos.x,
            self.pos.y,
            self.x_absolute,
            self.y_absolute
        );
    }

    CellReference(std::string_view str) {
        size_t i = 0;
        CellReference::parse(*this, str, i);
        if (i < str.size()) {
            throw std::invalid_argument("mlem");
        }
    }

    CellReference(CPos pos) : pos(pos) {}

    bool apply_relative_offset(std::pair<int, int> offset) {
        if (!x_absolute) {
            pos.x += offset.first;
        }
        if (!y_absolute) {
            pos.y += offset.second;
        }
        return !x_absolute || !y_absolute;
    }
};

struct CellRange {
    CellReference start {};
    CellReference end {};

    CellRange(CellReference start, CellReference end) :
        start(start),
        end(end) {}

    CellRange(std::string_view str) {
        size_t i = 0;
        CellReference::parse(start, str, i);
        if (i >= str.size() || str[i] != ':') {
            throw std::invalid_argument("Missing ':' in cell range");
        }
        i++;
        CellReference::parse(end, str, i);

        if (i < str.size()) {
            throw std::invalid_argument("mlem");
        }
    }

    template<typename F>
    void for_cells(F fun) const {
        for (int y = start.pos.y; y < end.pos.y; y++) {
            for (int x = start.pos.x; x < end.pos.x; x++) {
                fun(CPos(x, y));
            }
        }
    }
};

struct Function;

using Expression = std::variant<
    std::monostate,
    double,
    std::string,
    CellReference,
    CellRange,
    Function>;

struct Function {
    FunctionKind kind;
    std::unique_ptr<Expression[]> arguments;

    Function(const Function& copy) : kind(copy.kind), arguments(nullptr) {
        size_t count = copy.argument_count();
        arguments = std::unique_ptr<Expression[]>(new Expression[count] {});

        for (size_t i = 0; i < count; i++) {
            // use placement new instead of = assignment, because assignmend is deleted ...
            // this does not run the destructor for the default Expression value
            // this is okay because the default is std::monostate which contains nothing
            arguments[i] = copy.arguments[i];
        }
    }

    Function(FunctionKind kind, std::unique_ptr<Expression[]> arguments) :
        kind(kind),
        arguments(std::move(arguments)) {}

    Function& operator=(const Function& other) {
        Function copy(other);
        kind = copy.kind;
        arguments = std::move(copy.arguments);
        return *this;
    }

    size_t argument_count() const {
        return kind_argument_count(kind);
    }

    static size_t kind_argument_count(FunctionKind kind) {
        switch (kind) {
            case FunctionKind::SUM:
            case FunctionKind::COUNT:
            case FunctionKind::MIN:
            case FunctionKind::MAX:
            case FunctionKind::NEG:
                return 1;
            case FunctionKind::COUNT_VAL:
            case FunctionKind::POW:
            case FunctionKind::MUL:
            case FunctionKind::DIV:
            case FunctionKind::ADD:
            case FunctionKind::SUB:
            case FunctionKind::LT:
            case FunctionKind::LE:
            case FunctionKind::GT:
            case FunctionKind::GE:
            case FunctionKind::NE:
            case FunctionKind::EQ:
                return 2;
            case FunctionKind::IF:
                return 3;
            default:
                break;
        }
        assert(false);
    }

    Expression* begin() {
        return arguments.get();
    }

    Expression* end() {
        return arguments.get() + argument_count();
    }
};

class ExpressionBuilder: public CExprBuilder {
    std::vector<Expression> stack;

  public:
    ExpressionBuilder() {}

    void assert_size(size_t size) {
        if (stack.size() < size) {
            throw std::invalid_argument("assert_size() failed");
        }
    }

    Expression pop() {
        assert_size(1);

        Expression take = std::move(stack.back());
        stack.pop_back();
        return take;
    }

    Expression finish() {
        assert(stack.size() == 1);
        return pop();
    }

    std::unique_ptr<Expression[]> pop_multiple(size_t len) {
        assert_size(len);

        std::unique_ptr<Expression[]> ptr(new Expression[len]);

        auto start = stack.begin() + (ptrdiff_t)(stack.size() - len);
        for (size_t i = 0; i < len; i++) {
            ptr[i] = std::move(start[(ptrdiff_t)i]);
        }

        stack.erase(start, stack.end());
        return ptr;
    }

    void push(Expression e) {
        stack.push_back(std::move(e));
    }

    void push_function(FunctionKind kind) {
        size_t count = Function::kind_argument_count(kind);
        auto args = pop_multiple(count);
        push({Function(kind, std::move(args))});
    }

    virtual void opAdd() {
        push_function(FunctionKind::ADD);
    }

    virtual void opSub() {
        push_function(FunctionKind::SUB);
    }

    virtual void opMul() {
        push_function(FunctionKind::MUL);
    }

    virtual void opDiv() {
        push_function(FunctionKind::DIV);
    }

    virtual void opPow() {
        push_function(FunctionKind::POW);
    }

    virtual void opNeg() {
        push_function(FunctionKind::NEG);
    }

    virtual void opEq() {
        push_function(FunctionKind::EQ);
    }

    virtual void opNe() {
        push_function(FunctionKind::NE);
    }

    virtual void opLt() {
        push_function(FunctionKind::LT);
    }

    virtual void opLe() {
        push_function(FunctionKind::LE);
    }

    virtual void opGt() {
        push_function(FunctionKind::GT);
    }

    virtual void opGe() {
        push_function(FunctionKind::GE);
    }

    virtual void valNumber(double val) {
        push(Expression(val));
    }

    virtual void valString(std::string val) {
        push(Expression(val));
    }

    virtual void valReference(std::string val) {
        push(CellReference(val));
    }

    virtual void valRange(std::string val) {
        push(CellRange(val));
    }

    virtual void funcCall(std::string fnName, int paramCount) {
        FunctionKind kind;
        if (fnName == "sum") {
            kind = FunctionKind::SUM;
        } else if (fnName == "count") {
            kind = FunctionKind::COUNT;
        } else if (fnName == "min") {
            kind = FunctionKind::MIN;
        } else if (fnName == "max") {
            kind = FunctionKind::MAX;
        } else if (fnName == "countval") {
            kind = FunctionKind::COUNT_VAL;
        } else if (fnName == "if") {
            kind = FunctionKind::IF;
        } else {
            throw std::invalid_argument("Unhandled function name");
        }
        assert((size_t)paramCount == Function::kind_argument_count(kind));
        push_function(kind);
    }
};

class Cell {
    Expression expression;
    CValue cached_value;
    bool dirty;

  public:
    Cell() {}

    Cell(const std::string& str) {
        ExpressionBuilder builder {};
        parseExpression(str, builder);
        expression = builder.finish();
    }

    template<typename F>
    void visit_expressions(F fun) {
        Cell::visit_expression(expression, fun);
    }

    template<typename F>
    static void visit_expression(Expression& expr, F fun) {
        fun(expr);
        Cell::on_variant<Function>(expr, [fun](Function& function) {
            for (auto& arg : function) {
                Cell::visit_expression(arg, fun);
            }
        });
    }

    template<typename T, typename F>
    static bool on_variant(Expression& expr, F fun) {
        try {
            T& variant = std::get<T>(expr);
            fun(variant);

            return true;
        } catch (std::bad_variant_access& ex) {
            return false;
        }
    }

    template<typename F>
    void on_cell_references(F fun) {
        this->visit_expressions([fun](Expression& expr) {
            Cell::on_variant<CellReference>(expr, [fun](auto& c) {
                fun(c.pos);
            });
            Cell::on_variant<CellRange>(expr, [fun](auto& c) {
                c.for_cells(fun);
            });
        });
    }

    bool apply_offset(std::pair<int, int> offset) {
        bool any_relative = false;
        this->visit_expressions([&](Expression& expr) {
            Cell::on_variant<CellReference>(expr, [&](auto& c) {
                any_relative |= c.apply_relative_offset(offset);
            });
            Cell::on_variant<CellRange>(expr, [&](auto& c) {
                any_relative |= c.start.apply_relative_offset(offset);
                any_relative |= c.end.apply_relative_offset(offset);
            });
        });
        return any_relative;
    }

    friend class CSpreadsheet;
};

class CSpreadsheet {
    std::map<CPos, Cell> cells;
    std::set<std::pair<CPos, CPos>> edges;
    std::set<CPos> cell_call_stack;

  public:
    static unsigned capabilities() {
        return SPREADSHEET_CYCLIC_DEPS | SPREADSHEET_FUNCTIONS
            | SPREADSHEET_SPEED
            //  | SPREADSHEET_FILE_IO
            ;
    }

    CSpreadsheet() {}

    bool load(std::istream& is) {
        assert(0 && "unsupported");
        return false;
    }

    bool save(std::ostream& os) const {
        assert(0 && "unsupported");
        return false;
    }

    void remove_cell_dependency(CPos from, CPos to) {
        edges.erase(std::make_pair(from, to));
    }

    void add_cell_dependency(CPos from, CPos to) {
        edges.insert(std::make_pair(from, to));
    }

    void mark_dirty(CPos pos) {
        auto entry = cell_call_stack.insert(pos);

        if (!entry.second) {
            return;
        }

        cells[pos].dirty = true;

        auto children =
            edges.lower_bound(std::make_pair(pos, CPos(INT_MIN, INT_MIN)));

        for (; children != edges.end() && children->first == pos; children++) {
            mark_dirty(children->second);
        }

        cell_call_stack.erase(entry.first);
    }

    bool setCell_internal(CPos pos, Cell cell) {
        auto entry = cells.insert({pos, std::move(cell)});
        Cell& cell_entry = entry.first->second;

        if (!entry.second) {
            cell_entry.on_cell_references([pos, this](CPos c) {
                this->remove_cell_dependency(c, pos);
            });

            // only doing this if the first insert failed
            // which should mean that the cell wasn't moved out of
            cell_entry = std::move(cell);
        }

        cell_entry.on_cell_references([pos, this](CPos c) {
            this->add_cell_dependency(c, pos);
        });

        mark_dirty(pos);
        assert(cell_call_stack.empty());

        return true;
    }

    bool setCell(CPos pos, std::string contents) {
        try {
            Cell cell(contents);
            return setCell_internal(pos, cell);
        } catch (...) {
            return false;
        }
    }

    Cell* get_cell(CPos pos) {
        try {
            return &cells.at(pos);
        } catch (const std::out_of_range& e) {
            return nullptr;
        }
    }

    CValue reduce_range(
        CellRange range,
        double initial,
        void (*fun)(double* acc, double input)
    ) {
        bool empty = true;
        double acc = initial;
        range.for_cells([&](CPos pos) {
            const CValue& value = getValue_internal(pos);
            if (std::holds_alternative<double>(value)) {
                empty = false;
                fun(&acc, std::get<double>(value));
            }
        });
        if (empty) {
            return UNDEFINED;
        } else {
            return CValue(acc);
        }
    }

    CValue numeric_binary_operator(
        const Function& function,
        double (*fun)(double a, double b)
    ) {
        CValue a = evaluate_expression(function.arguments[0]);
        CValue b = evaluate_expression(function.arguments[1]);

        if (!std::holds_alternative<double>(a)
            || !std::holds_alternative<double>(b)) {
            return UNDEFINED;
        }

        double a_ = std::get<double>(a);
        double b_ = std::get<double>(b);

        return CValue(fun(a_, b_));
    }

    CValue comparison_binary_operator(
        const Function& function,
        std::partial_ordering comparison,
        bool invert
    ) {
        CValue a = evaluate_expression(function.arguments[0]);
        CValue b = evaluate_expression(function.arguments[1]);

        std::partial_ordering order = std::partial_ordering::equivalent;
        if (std::holds_alternative<double>(a)
            && std::holds_alternative<double>(b)) {
            order = std::get<double>(a) <=> std::get<double>(b);
        } else if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b)) {
            order = std::get<std::string>(a) <=> std::get<std::string>(b);
        } else {
            return UNDEFINED;
        }

        return CValue((double)((order == comparison) ^ invert));
    }

    CValue evaluate_expression(const Expression& expr) {
        // std::monostate
        // double
        // std::string
        // CellReference
        // CellRange
        // Function
        switch (expr.index()) {
            case 0:
                return UNDEFINED;
            case 1:
                return CValue(std::get<double>(expr));
            case 2:
                return CValue(std::get<std::string>(expr));
            case 3: {
                CellReference ref = std::get<CellReference>(expr);
                return getValue_internal(ref.pos);
            }
            case 4:
                throw std::invalid_argument("Unexpected cell range");
            case 5: {
                const Function& fun = std::get<Function>(expr);
                switch (fun.kind) {
                    case FunctionKind::SUM: {
                        CellRange range = std::get<CellRange>(fun.arguments[0]);
                        return reduce_range(
                            range,
                            0.0,
                            [](double* acc, double in) { *acc += in; }
                        );
                    }
                    case FunctionKind::COUNT: {
                        CellRange range = std::get<CellRange>(fun.arguments[0]);
                        int count = 0;
                        range.for_cells([&](CPos pos) {
                            const CValue& value = getValue_internal(pos);
                            if (value != UNDEFINED) {
                                count++;
                            }
                        });
                        return CValue((double)count);
                    }
                    case FunctionKind::MIN: {
                        CellRange range = std::get<CellRange>(fun.arguments[0]);
                        return reduce_range(
                            range,
                            std::numeric_limits<double>::infinity(),
                            [](double* acc, double in) {
                                *acc = std::min(*acc, in);
                            }
                        );
                    }
                    case FunctionKind::MAX: {
                        CellRange range = std::get<CellRange>(fun.arguments[0]);
                        return reduce_range(
                            range,
                            -std::numeric_limits<double>::infinity(),
                            [](double* acc, double in) {
                                *acc = std::max(*acc, in);
                            }
                        );
                    }
                    case FunctionKind::NEG: {
                        CValue val = evaluate_expression(fun.arguments[0]);
                        if (std::holds_alternative<double>(val)) {
                            return CValue(-std::get<double>(val));
                        } else {
                            return UNDEFINED;
                        }
                    }
                    case FunctionKind::COUNT_VAL: {
                        CValue val = evaluate_expression(fun.arguments[0]);
                        CellRange range = std::get<CellRange>(fun.arguments[1]);

                        unsigned count = 0;
                        range.for_cells([&](CPos pos) {
                            const CValue& value = getValue_internal(pos);
                            if (value != val) {
                                count++;
                            }
                        });
                        return CValue((double)count);
                    }
                    case FunctionKind::POW: {
                        return numeric_binary_operator(fun, std::pow);
                    }
                    case FunctionKind::MUL: {
                        return numeric_binary_operator(
                            fun,
                            [](double a, double b) { return a * b; }
                        );
                    }
                    case FunctionKind::DIV: {
                        if (std::holds_alternative<double>(fun.arguments[1])) {
                            if (std::get<double>(fun.arguments[1]) == 0.0) {
                                return UNDEFINED;
                            }
                        }
                        return numeric_binary_operator(
                            fun,
                            [](double a, double b) { return a / b; }
                        );
                    }
                    case FunctionKind::ADD: {
                        const Expression& a = fun.arguments[0];
                        const Expression& b = fun.arguments[1];

                        if (std::holds_alternative<std::string>(a)
                            || std::holds_alternative<std::string>(b)) {
                            std::string buf;

                            if (std::holds_alternative<std::string>(a)) {
                                buf = std::get<std::string>(a);
                            } else if (std::holds_alternative<double>(a)) {
                                buf = std::to_string(std::get<double>(a));
                            } else {
                                throw std::invalid_argument(
                                    "Unexpected type in + operator"
                                );
                            }

                            if (std::holds_alternative<std::string>(b)) {
                                buf += std::get<std::string>(b);
                            } else if (std::holds_alternative<double>(b)) {
                                buf += std::to_string(std::get<double>(b));
                            } else {
                                throw std::invalid_argument(
                                    "Unexpected type in + operator"
                                );
                            }

                            return CValue(buf);
                        }

                        return numeric_binary_operator(
                            fun,
                            [](double a, double b) { return a + b; }
                        );
                    }
                    case FunctionKind::SUB: {
                        return numeric_binary_operator(
                            fun,
                            [](double a, double b) { return a - b; }
                        );
                    }
                    case FunctionKind::LT: {
                        return comparison_binary_operator(
                            fun,
                            std::partial_ordering::less,
                            false
                        );
                    }
                    case FunctionKind::LE: {
                        return comparison_binary_operator(
                            fun,
                            std::partial_ordering::greater,
                            true
                        );
                    }
                    case FunctionKind::GT: {
                        return comparison_binary_operator(
                            fun,
                            std::partial_ordering::greater,
                            false
                        );
                    }
                    case FunctionKind::GE: {
                        return comparison_binary_operator(
                            fun,
                            std::partial_ordering::less,
                            true
                        );
                    }
                    case FunctionKind::NE: {
                        return comparison_binary_operator(
                            fun,
                            std::partial_ordering::equivalent,
                            true
                        );
                    }
                    case FunctionKind::EQ: {
                        return comparison_binary_operator(
                            fun,
                            std::partial_ordering::equivalent,
                            false
                        );
                    }
                    case FunctionKind::IF: {
                        if (!std::holds_alternative<double>(fun.arguments[0])) {
                            return UNDEFINED;
                        }
                        double cond = std::get<double>(fun.arguments[0]);
                        if (cond != 0.0) {
                            return evaluate_expression(fun.arguments[1]);
                        } else {
                            return evaluate_expression(fun.arguments[2]);
                        }
                    }
                    default:
                        break;
                }
            }
            default:
                break;
        }
        assert(0 && "Unhandled variant");
    }

    const CValue& getValue_internal(CPos pos) {
        static const CValue STATIC_UNDEFINED = CValue {};
        Cell* cell = get_cell(pos);
        if (!cell) {
            return STATIC_UNDEFINED;
        }

        if (cell->dirty) {
            auto previous = cell_call_stack.insert(pos);
            if (!previous.second) {
                return STATIC_UNDEFINED;
            }

            try {
                cell->cached_value = evaluate_expression(cell->expression);
            } catch (...) {
                cell->cached_value = UNDEFINED;
            }

            cell->dirty = false;
            cell_call_stack.erase(pos);
        }
        return cell->cached_value;
    }

    CValue getValue(CPos pos) {
        assert(cell_call_stack.empty());
        return getValue_internal(pos);
    }

    void copyCell(CPos src, CPos dst) {
        if (dst == src) {
            return;
        }

        auto entry = cells.find(src);
        if (entry == cells.end()) {
            cells.erase(dst);
        } else {
            Cell copy = entry->second;
            auto offset = CPos::make_relative_offset(src, dst);
            // mark dirty if cell contained any relative cell references
            if (copy.apply_offset(offset)) {
                copy.dirty = true;
            }

            setCell_internal(dst, std::move(copy));
        }
    }

    void copyRect(CPos dst, CPos src, int w = 1, int h = 1) {
        assert(w >= 0);
        assert(h >= 0);

        if (src == dst || w == 0 || h == 0) {
            return;
        }

        int x_start = src.x;
        int x_end = src.x + w - 1;
        int x_d = 1;

        if (dst.x > src.x) {
            std::swap(x_start, x_end);
            x_d = -1;
        }

        int y_start = src.y;
        int y_end = src.y + h - 1;
        int y_d = 1;

        if (dst.y > src.y) {
            std::swap(y_start, y_end);
            y_d = -1;
        }

        auto offset = CPos::make_relative_offset(src, dst);

        for (int y = y_start;; y += y_d) {
            for (int x = x_start;; x += x_d) {
                CPos src_(x, y);
            CPos dst_ = src_ + offset;
            copyCell(src_, dst_);
                if (x == x_end) {
                    break;
                }
            }
            if (y == y_end) {
                break;
            }
        }
    }
};
