#ifndef __PROGTEST__
    #include <algorithm>
    #include <cassert>
    #include <cmath>
    #include <compare>
    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <iostream>
    #include <map>
    #include <memory>
    #include <set>
    #include <sstream>
    #include <stdexcept>
    #include <string>
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
    NE,  // !=
    EQ,  // ==
};

bool parse_cell_position(
    // input string
    std::string_view str,
    // offset into str
    size_t& i,

    // output variables
    int& x,
    int& y,
    bool& x_is_absolute,
    bool& y_is_absolute
) {
    bool x_empty = true;
    bool y_empty = true;

    if (i < str.size() && str[i] == '$') {
        x_is_absolute = true;
        i++;
    }
    while (i < str.size()
           && (('A' <= str[i] && str[i] <= 'Z')
               || ('a' <= str[i] && str[i] <= 'z'))) {
        x_empty = false;
        x *= 26;
        // A in excel numbering is actually a 1
        // otherwise AAAAB == 0000B == B
        x += std::tolower(str[i]) - 'a' + 1;
        i++;
    }

    if (i < str.size() && str[i] == '$') {
        y_is_absolute = true;
        i++;
    }
    while (i < str.size() && '0' <= str[i] && str[i] <= '9') {
        y_empty = false;
        y *= 10;
        y += str[i] - '0';
        i++;
    }
    return !x_empty && !y_empty;
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
        bool success =
            parse_cell_position(str, i, x, y, x_absolute, y_absolute);

        if (!success || x_absolute || y_absolute || i < str.size()) {
            throw std::invalid_argument("Cpos parsing failed");
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

    static bool parse(CellReference& self, std::string_view str, size_t& i) {
        return parse_cell_position(
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
        bool success = CellReference::parse(*this, str, i);
        if (!success || i < str.size()) {
            throw std::invalid_argument("CellReference parsing failed");
        }
    }

    CellReference(CPos pos) : pos(pos) {}

    void apply_relative_offset(std::pair<int, int> offset) {
        if (!x_absolute) {
            pos.x += offset.first;
        }
        if (!y_absolute) {
            pos.y += offset.second;
        }
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
        bool success = true;
        success &= CellReference::parse(start, str, i);
        if (i >= str.size() || str[i] != ':') {
            throw std::invalid_argument("Missing ':' in cell range");
        }
        i++;
        success &= CellReference::parse(end, str, i);

        if (!success || i < str.size()) {
            throw std::invalid_argument("CellRange parsing failed");
        }
    }

    // call closure for all cells in range
    template<typename F>
    void for_cells(F fun) const {
        for (int y = start.pos.y; y <= end.pos.y; y++) {
            for (int x = start.pos.x; x <= end.pos.x; x++) {
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
        return static_argument_count(kind);
    }

    static size_t static_argument_count(FunctionKind kind) {
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
        assert(0 && "Missing variant");
    }

    Expression* begin() {
        return arguments.get();
    }

    Expression* end() {
        return arguments.get() + argument_count();
    }

    const Expression* begin() const {
        return arguments.get();
    }

    const Expression* end() const {
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

    // pops multiple values from the stack and returns them
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

    // pops number of arguments depending on FunctionKind and pushes a Function
    void push_function(FunctionKind kind) {
        size_t count = Function::static_argument_count(kind);
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

// compiler bug?
// https://gcc.gnu.org/bugzilla/show_bug.cgi?format=multiple&id=107138
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

    void valUndefined() {
        push(Expression());
    }

    virtual void valNumber(double val) {
        push(Expression(val));
    }

    virtual void valString(std::string val) {
        push(std::move(val));
    }

    virtual void valReference(std::string val) {
        push(CellReference(val));
    }

    void rawValReference(CellReference val) {
        push(val);
    }

    void rawValRange(CellRange val) {
        push(val);
    }

    virtual void valRange(std::string val) {
        push(CellRange(val));
    }

#pragma GCC diagnostic pop

    void rawFuncCall(FunctionKind kind) {
        push_function(kind);
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
        assert((size_t)paramCount == Function::static_argument_count(kind));
        push_function(kind);
    }
};

class Cell {
    Expression expression {};
    CValue cached_value = UNDEFINED;
    bool dirty = true;

  public:
    Cell() = delete;

    Cell(Expression expr) : expression(expr) {}

    Cell(const std::string& str) {
        ExpressionBuilder builder {};
        parseExpression(str, builder);
        expression = builder.finish();
    }

    template<typename F>
    static void visit_expression(Expression& expr, F fun) {
        on_variant<Function>(expr, [fun](Function& function) {
            for (auto& arg : function) {
                Cell::visit_expression(arg, fun);
            }
        });
        fun(expr);
    }

    template<typename F>
    static void visit_expression(const Expression& expr, F fun) {
        on_variant<Function>(expr, [fun](const Function& function) {
            for (const auto& arg : function) {
                Cell::visit_expression(arg, fun);
            }
        });
        fun(expr);
    }

    // std::visit doesn't work and I don't know why
    template<typename T, typename F>
    static bool on_variant(Expression& expr, F fun) {
        if (std::holds_alternative<T>(expr)) {
            T& variant = std::get<T>(expr);
            fun(variant);
            return true;
        } else {
            return false;
        }
    }

    template<typename T, typename F>
    static bool on_variant(const Expression& expr, F fun) {
        if (std::holds_alternative<T>(expr)) {
            const T& variant = std::get<T>(expr);
            fun(variant);
            return true;
        } else {
            return false;
        }
    }

    // call closure for all cell references in expression
    template<typename F>
    void on_cell_references(F fun) {
        Cell::visit_expression(expression, [fun](Expression& expr) {
            Cell::on_variant<CellReference>(expr, [fun](auto& c) {
                fun(c.pos);
            });
            Cell::on_variant<CellRange>(expr, [fun](auto& c) {
                c.for_cells(fun);
            });
        });
    }

    void apply_offset(std::pair<int, int> offset) {
        Cell::visit_expression(expression, [&](Expression& expr) {
            Cell::on_variant<CellReference>(expr, [&](auto& c) {
                c.apply_relative_offset(offset);
            });
            Cell::on_variant<CellRange>(expr, [&](auto& c) {
                c.start.apply_relative_offset(offset);
                c.end.apply_relative_offset(offset);
            });
        });
    }

    friend class CSpreadsheet;
};

class StreamWriter {
    std::ostream& os;

  public:
    StreamWriter(std::ostream& os) : os(os) {}

    std::ostream& get_inner() {
        return os;
    }

    void write_i8(int8_t value) {
        os.write(reinterpret_cast<const char*>(&value), sizeof(int8_t));
    }

    void write_i32(int32_t value) {
        os.write(reinterpret_cast<const char*>(&value), sizeof(int32_t));
    }

    void write_i64(int64_t value) {
        os.write(reinterpret_cast<const char*>(&value), sizeof(int64_t));
    }

    void write_double(double value) {
        os.write(reinterpret_cast<const char*>(&value), sizeof(double));
    }

    void write_string(std::string_view str) {
        os.write(str.data(), (std::streamsize)str.size());
        os.put(0);
    }

    void write_cell_pos(CPos pos) {
        write_i32(pos.x);
        write_i32(pos.y);
    }

    void write_cell_ref(const CellReference& cell) {
        write_cell_pos(cell.pos);
        write_i8((int8_t)cell.x_absolute);
        write_i8((int8_t)cell.y_absolute);
    }

    void _inner_write_expression(const Expression& expr) {
        write_i8((int8_t)expr.index());
        // std::monostate
        // double
        // std::string
        // CellReference
        // CellRange
        // Function
        switch (expr.index()) {
            case 0:
                break;
            case 1:
                write_double(std::get<double>(expr));
                break;
            case 2:
                write_string(std::get<std::string>(expr));
                break;
            case 3: {
                CellReference ref = std::get<CellReference>(expr);
                write_cell_ref(ref);
                break;
            }
            case 4: {
                CellRange range = std::get<CellRange>(expr);
                write_cell_ref(range.start);
                write_cell_ref(range.end);
                break;
            }
            case 5: {
                Function function = std::get<Function>(expr);
                write_i8((int8_t)function.kind);
                break;
            };
            default:
                assert(false);
        }
    }

    void write_expression(const Expression& expr) {
        Cell::visit_expression(expr, [&](const Expression& expr) {
            _inner_write_expression(expr);
        });
        write_i8(-1);
    }
};

class StreamReader {
    std::vector<char> buffer;
    std::istream& os;

  public:
    StreamReader(std::istream& os) : os(os) {}

    int8_t read_i8() {
        int8_t value = 0;
        os.read(reinterpret_cast<char*>(&value), sizeof(int8_t));
        return value;
    }

    int32_t read_i32() {
        int32_t value = 0;
        os.read(reinterpret_cast<char*>(&value), sizeof(int32_t));
        return value;
    }

    int64_t read_i64() {
        int64_t value = 0;
        os.read(reinterpret_cast<char*>(&value), sizeof(int64_t));
        return value;
    }

    double read_double() {
        double value = 0;
        os.read(reinterpret_cast<char*>(&value), sizeof(double));
        return value;
    }

    std::string_view read_string() {
        buffer.clear();
        if (os.bad()) {
            return "";
        }

        while (true) {
            int c = os.get();
            if (c == EOF || c == 0) {
                break;
            }
            buffer.push_back((char)c);
        }

        return std::string_view(buffer.data(), buffer.size());
    }

    CPos read_cell_pos() {
        CPos pos {};
        pos.x = read_i32();
        pos.y = read_i32();
        return pos;
    }

    CellReference read_cell_ref() {
        CellReference cell {};
        cell.pos = read_cell_pos();
        cell.x_absolute = (bool)read_i8();
        cell.y_absolute = (bool)read_i8();
        return cell;
    }

    bool _inner_read_expression(ExpressionBuilder& builder) {
        int8_t kind = read_i8();
        // std::monostate
        // double
        // std::string
        // CellReference
        // CellRange
        // Function
        switch (kind) {
            case 0:
                builder.valUndefined();
                break;
            case 1: {
                auto val = read_double();
                builder.valNumber(val);
                break;
            }
            case 2: {
                auto val = read_string();
                builder.valString(std::string(val));
                break;
            }
            case 3: {
                CellReference val = read_cell_ref();
                builder.rawValReference(val);
                break;
            }
            case 4: {
                CellReference start = read_cell_ref();
                CellReference end = read_cell_ref();
                builder.rawValRange(CellRange {start, end});
                break;
            }
            case 5: {
                FunctionKind function = (FunctionKind)read_i8();
                builder.rawFuncCall(function);
                break;
            };
            default:
                return false;
        }
        return true;
    }

    void read_expression(ExpressionBuilder& builder) {
        while (_inner_read_expression(builder)) {}
    }
};

// fnv hashing implementation from
// https://gist.github.com/hwei/1950649d523afd03285c
class FnvHasher {
    unsigned int state;

    static const unsigned int OFFSET_BASIS = 2166136261u;
    static const unsigned int FNV_PRIME = 16777619u;

  public:
    FnvHasher() : state(OFFSET_BASIS) {}

    inline void hash_char(char c) {
        state ^= c;
        state *= FNV_PRIME;
    }

    void hash_istream(std::istream& is) {
        if (is.fail()) {
            return;
        }

        auto position = is.tellg();

        while (true) {
            int c = is.get();
            if (c == EOF) {
                break;
            }
            hash_char((char)c);
        }

        is.clear();
        is.seekg(position);
    }

    unsigned int finish() {
        return state;
    }
};

class CSpreadsheet {
    std::map<CPos, Cell> cells;
    std::set<std::pair<CPos, CPos>> edges;
    std::set<CPos> call_stack;

  public:
    CSpreadsheet() {}

    CSpreadsheet(const CSpreadsheet& other) = default;
    CSpreadsheet(CSpreadsheet&& other) = default;
    CSpreadsheet& operator=(const CSpreadsheet& other) = default;

    static unsigned capabilities() {
        return SPREADSHEET_CYCLIC_DEPS | SPREADSHEET_FUNCTIONS
            | SPREADSHEET_SPEED | SPREADSHEET_FILE_IO;
    }

    bool load(std::istream& is) {
        StreamReader w(is);

        int64_t saved_hash = w.read_i64();

        FnvHasher hasher;
        hasher.hash_istream(is);
        unsigned int hash = hasher.finish();

        if (hash != saved_hash) {
            return false;
        }

        cells.clear();
        edges.clear();

        ExpressionBuilder builder {};

        int64_t cells_len = w.read_i64();
        for (int64_t i = 0; i < cells_len; i++) {
            CPos pos = w.read_cell_pos();
            w.read_expression(builder);
            Cell cell(builder.finish());

            cells.insert({pos, cell});
        }

        int64_t edges_len = w.read_i64();
        for (int64_t i = 0; i < edges_len; i++) {
            CPos a = w.read_cell_pos();
            CPos b = w.read_cell_pos();

            edges.insert({a, b});
        }

        return !is.fail();
    }

    bool save(std::ostream& os) const {
        std::stringstream ss;

        StreamWriter w(ss);

        // leave space for hash
        w.write_i64(0);

        w.write_i64((int64_t)cells.size());
        for (auto& pair : cells) {
            w.write_cell_pos(pair.first);
            w.write_expression(pair.second.expression);
        }

        w.write_i64((int64_t)edges.size());
        for (auto& pair : edges) {
            w.write_cell_pos(pair.first);
            w.write_cell_pos(pair.second);
        }

        {
            FnvHasher hasher;
            ss.seekp(8);
            ss.seekg(8);
            hasher.hash_istream(ss);
            unsigned int hash = hasher.finish();

            w.get_inner().seekp(0);
            w.write_i64(hash);
        }

        ss.seekg(0);
        os.write(ss.view().data(), (long)ss.view().size());

        return !os.fail();
    }

    void remove_cell_dependency(CPos from, CPos to) {
        edges.erase(std::make_pair(from, to));
    }

    void add_cell_dependency(CPos from, CPos to) {
        edges.insert(std::make_pair(from, to));
    }

    // mark the pos and all cells that depend on it as dirty
    void mark_dirty(CPos pos) {
        auto entry = call_stack.insert(pos);

        if (!entry.second) {
            return;
        }

        {
            auto entry = cells.find(pos);
            if (entry == cells.end()) {
                return;
            }

            Cell& cell_entry = entry->second;
            cell_entry.dirty = true;

            auto children =
                edges.lower_bound(std::make_pair(pos, CPos(INT_MIN, INT_MIN)));

            for (; children != edges.end() && children->first == pos;
                 children++) {
                mark_dirty(children->second);
            }
        }

        call_stack.erase(entry.first);
    }

    bool setCell_internal(CPos pos, Cell cell) {
        auto entry = cells.find(pos);
        if (entry != cells.end()) {
            entry->second.on_cell_references([&](CPos c) {
                this->remove_cell_dependency(c, pos);
            });

            entry->second = std::move(cell);
        } else {
            entry = cells.insert({pos, std::move(cell)}).first;
        }

        entry->second.on_cell_references([&](CPos c) {
            this->add_cell_dependency(c, pos);
        });

        mark_dirty(pos);

        return true;
    }

    bool setCell(CPos pos, std::string contents) {
        try {
            return setCell_internal(pos, Cell(contents));
        } catch (std::invalid_argument& e) {
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

    // fold a lambda over a cell range, undefined
    CValue fold_range(
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

    // call lambda on two number arguments, otherwise return undefined
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
        bool (*number_fun)(double a, double b),
        bool (*string_fun)(std::string& a, std::string& b)
    ) {
        CValue a = evaluate_expression(function.arguments[0]);
        CValue b = evaluate_expression(function.arguments[1]);

        bool compare = false;
        if (std::holds_alternative<double>(a)
            && std::holds_alternative<double>(b)) {
            compare = number_fun(std::get<double>(a), std::get<double>(b));
        } else if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b)) {
            compare =
                string_fun(std::get<std::string>(a), std::get<std::string>(b));
        } else {
            return UNDEFINED;
        }

        return CValue((double)compare);
    }

    CValue evaluate_expression_internal(const Expression& expr) {
        // 0 std::monostate
        // 1 double
        // 2 std::string
        // 3 CellReference
        // 4 CellRange
        // 5 Function
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
                        return fold_range(
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
                    case FunctionKind::COUNT_VAL: {
                        CValue val = evaluate_expression(fun.arguments[0]);
                        CellRange range = std::get<CellRange>(fun.arguments[1]);

                        unsigned count = 0;
                        range.for_cells([&](CPos pos) {
                            const CValue& value = getValue_internal(pos);
                            if (val == value) {
                                count++;
                            }
                        });
                        return CValue((double)count);
                    }
                    case FunctionKind::IF: {
                        CValue cond = evaluate_expression(fun.arguments[0]);
                        if (std::get<double>(cond) != 0.0) {
                            return evaluate_expression(fun.arguments[1]);
                        } else {
                            return evaluate_expression(fun.arguments[2]);
                        }
                    }
                    case FunctionKind::MIN: {
                        CellRange range = std::get<CellRange>(fun.arguments[0]);
                        return fold_range(
                            range,
                            std::numeric_limits<double>::infinity(),
                            [](double* acc, double in) {
                                *acc = std::min(*acc, in);
                            }
                        );
                    }
                    case FunctionKind::MAX: {
                        CellRange range = std::get<CellRange>(fun.arguments[0]);
                        return fold_range(
                            range,
                            -std::numeric_limits<double>::infinity(),
                            [](double* acc, double in) {
                                *acc = std::max(*acc, in);
                            }
                        );
                    }
                    case FunctionKind::NEG: {
                        CValue val = evaluate_expression(fun.arguments[0]);
                        return CValue(-std::get<double>(val));
                    }
                    case FunctionKind::POW: {
                        return numeric_binary_operator(
                            fun,
                            [](double a, double b) { return std::pow(a, b); }
                        );
                    }
                    case FunctionKind::MUL: {
                        return numeric_binary_operator(
                            fun,
                            [](double a, double b) { return a * b; }
                        );
                    }
                    case FunctionKind::DIV: {
                        return numeric_binary_operator(
                            fun,
                            [](double a, double b) {
                                if (fabs(b) == 0.0) {
                                    throw std::invalid_argument("Divide by zero"
                                    );
                                }
                                return a / b;
                            }
                        );
                    }
                    case FunctionKind::ADD: {
                        CValue a = evaluate_expression(fun.arguments[0]);
                        CValue b = evaluate_expression(fun.arguments[1]);

                        if (std::holds_alternative<std::string>(a)
                            || std::holds_alternative<std::string>(b)) {
                            std::string buf;

                            if (std::holds_alternative<std::string>(a)) {
                                buf = std::get<std::string>(std::move(a));
                            } else if (std::holds_alternative<double>(a)) {
                                buf = std::to_string(std::get<double>(a));
                            } else {
                                return UNDEFINED;
                            }

                            if (std::holds_alternative<std::string>(b)) {
                                buf += std::get<std::string>(b);
                            } else if (std::holds_alternative<double>(b)) {
                                buf += std::to_string(std::get<double>(b));
                            } else {
                                return UNDEFINED;
                            }

                            return CValue(buf);
                        }

                        double a_ = std::get<double>(a);
                        double b_ = std::get<double>(b);

                        return CValue(a_ + b_);
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
                            [](double a, double b) { return a < b; },
                            [](std::string& a, std::string& b) { return a < b; }
                        );
                    }
                    case FunctionKind::LE: {
                        return comparison_binary_operator(
                            fun,
                            [](double a, double b) { return a <= b; },
                            [](std::string& a, std::string& b) {
                                return a <= b;
                            }
                        );
                    }
                    case FunctionKind::GT: {
                        return comparison_binary_operator(
                            fun,
                            [](double a, double b) { return a > b; },
                            [](std::string& a, std::string& b) { return a > b; }
                        );
                    }
                    case FunctionKind::GE: {
                        return comparison_binary_operator(
                            fun,
                            [](double a, double b) { return a >= b; },
                            [](std::string& a, std::string& b) {
                                return a >= b;
                            }
                        );
                    }
                    case FunctionKind::NE: {
                        return comparison_binary_operator(
                            fun,
                            [](double a, double b) { return a != b; },
                            [](std::string& a, std::string& b) {
                                return a != b;
                            }
                        );
                    }
                    case FunctionKind::EQ: {
                        return comparison_binary_operator(
                            fun,
                            [](double a, double b) { return a == b; },
                            [](std::string& a, std::string& b) {
                                return a == b;
                            }
                        );
                    }
                    default:
                        break;
                }
            }
            default:
                break;
        }
        assert(0 && "Unhandled variant");
        return UNDEFINED;
    }

    CValue evaluate_expression(const Expression& expr) {
        try {
            return evaluate_expression_internal(expr);
        } catch (...) {
            return UNDEFINED;
        }
    }

    // getValue but returns a reference
    const CValue& getValue_internal(CPos pos) {
        static const CValue STATIC_UNDEFINED = CValue {};

        Cell* cell = get_cell(pos);
        if (!cell) {
            return STATIC_UNDEFINED;
        }

        if (cell->dirty) {
            auto previous = call_stack.insert(pos);

            if (!previous.second) {
                return STATIC_UNDEFINED;
            }

            cell->cached_value = evaluate_expression(cell->expression);
            cell->dirty = false;

            call_stack.erase(pos);
        }

        return cell->cached_value;
    }

    CValue getValue(CPos pos) {
        assert(call_stack.empty());
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
            copy.apply_offset(offset);

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
        int x_end = src.x + w - 1;  // end inclusive
        int x_d = 1;

        // the copy ranges may overlap so we need to reverse the copy direction
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
