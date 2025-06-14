#include <cassert>
#include <cfloat>
#include <sstream>

#include "velka.cpp"

bool valueMatch_internal(const CValue& r, const CValue& s) {
    if (r.index() != s.index()) {
        return false;
    }
    if (r.index() == 0) {
        return true;
    }
    if (r.index() == 2) {
        return std::get<std::string>(r) == std::get<std::string>(s);
    }
    if (std::isnan(std::get<double>(r)) && std::isnan(std::get<double>(s))) {
        return true;
    }
    if (std::isinf(std::get<double>(r)) && std::isinf(std::get<double>(s))) {
        return (std::get<double>(r) < 0 && std::get<double>(s) < 0)
            || (std::get<double>(r) > 0 && std::get<double>(s) > 0);
    }
    return fabs(std::get<double>(r) - std::get<double>(s))
        <= 1e8 * DBL_EPSILON * fabs(std::get<double>(r));
}

void print_value(const CValue& a) {
    //  std::variant<std::monostate, double, std::string>;
    switch (a.index()) {
        case 0:
            std::cout << "Undefined";
            break;
        case 1:
            std::cout << std::get<double>(a);
            break;
        case 2:
            std::cout << std::get<std::string>(a);
            break;
        default:
            assert(false);
    }
}

bool valueMatch(const CValue& r, const CValue& s) {
    bool result = valueMatch_internal(r, s);
    if (!result) {
        std::cout << "Expected: ";
        print_value(s);
        std::cout << std::endl;

        std::cout << "Got:      ";
        print_value(r);
        std::cout << std::endl;
    }
    return result;
}

bool exprMatch(CSpreadsheet& s, std::string expr, CValue value) {
    CPos pos("A9999999");
    s.setCell(pos, expr);
    bool res = valueMatch(s.getValue(pos), value);
    assert(res);
    return res;
}

int main() {
    CSpreadsheet x0, x1;
    std::ostringstream oss;
    std::istringstream iss;
    std::string data;

    assert(x0.setCell(CPos("A1"), "10"));
    assert(x0.setCell(CPos("A2"), "10.0"));
    assert(x0.setCell(CPos("A3"), "11.0"));
    exprMatch(x0, "=A1 < A2", 0.0);
    exprMatch(x0, "=A1 <= A2", 1.0);
    exprMatch(x0, "=A1 > A2", 0.0);
    exprMatch(x0, "=A1 >= A2", 1.0);
    exprMatch(x0, "=A1 = A2", 1.0);
    exprMatch(x0, "=A1 <> A2", 0.0);

    exprMatch(x0, "=A1 < A3", 1.0);
    exprMatch(x0, "=A1 <= A3", 1.0);
    exprMatch(x0, "=A1 > A3", 0.0);
    exprMatch(x0, "=A1 >= A3", 0.0);
    exprMatch(x0, "=A1 = A3", 0.0);
    exprMatch(x0, "=A1 <> A3", 1.0);

    assert(!x0.setCell(CPos("A1"), "=A ** 6b"));

    assert(x0.setCell(CPos("A1"), "10"));
    assert(x0.setCell(CPos("A2"), "20.5"));
    assert(x0.setCell(CPos("A3"), "3e1"));
    assert(x0.setCell(CPos("A4"), "=40"));
    assert(x0.setCell(CPos("A5"), "=5e+1"));
    assert(x0.setCell(
        CPos("A6"),
        "raw text with any characters, including a quote \" or a newline\n"
    ));
    assert(x0.setCell(
        CPos("A7"),
        "=\"quoted string, quotes must be doubled: \"\". Moreover, backslashes are needed for C++.\""
    ));
    assert(valueMatch(x0.getValue(CPos("A1")), CValue(10.0)));
    assert(valueMatch(x0.getValue(CPos("A2")), CValue(20.5)));
    assert(valueMatch(x0.getValue(CPos("A3")), CValue(30.0)));
    assert(valueMatch(x0.getValue(CPos("A4")), CValue(40.0)));
    assert(valueMatch(x0.getValue(CPos("A5")), CValue(50.0)));
    assert(valueMatch(
        x0.getValue(CPos("A6")),
        CValue(
            "raw text with any characters, including a quote \" or a newline\n"
        )
    ));
    assert(valueMatch(
        x0.getValue(CPos("A7")),
        CValue(
            "quoted string, quotes must be doubled: \". Moreover, backslashes are needed for C++."
        )
    ));
    assert(valueMatch(x0.getValue(CPos("A8")), CValue()));
    assert(valueMatch(x0.getValue(CPos("AAAA9999")), CValue()));
    assert(x0.setCell(CPos("B1"), "=A1+A2*A3"));
    assert(x0.setCell(CPos("B2"), "= -A1 ^ 2 - A2 / 2   "));
    assert(x0.setCell(CPos("B3"), "= 2 ^ $A$1"));
    assert(x0.setCell(CPos("B4"), "=($A1+A$2)^2"));
    assert(x0.setCell(CPos("B5"), "=B1+B2+B3+B4"));
    assert(x0.setCell(CPos("B6"), "=B1+B2+B3+B4+B5"));
    assert(valueMatch(x0.getValue(CPos("B1")), CValue(625.0)));
    assert(valueMatch(x0.getValue(CPos("B2")), CValue(-110.25)));
    assert(valueMatch(x0.getValue(CPos("B3")), CValue(1024.0)));
    assert(valueMatch(x0.getValue(CPos("B4")), CValue(930.25)));
    assert(valueMatch(x0.getValue(CPos("B5")), CValue(2469.0)));
    assert(valueMatch(x0.getValue(CPos("B6")), CValue(4938.0)));
    assert(x0.setCell(CPos("A1"), "12"));
    assert(valueMatch(x0.getValue(CPos("B1")), CValue(627.0)));
    assert(valueMatch(x0.getValue(CPos("B2")), CValue(-154.25)));
    assert(valueMatch(x0.getValue(CPos("B3")), CValue(4096.0)));
    assert(valueMatch(x0.getValue(CPos("B4")), CValue(1056.25)));
    assert(valueMatch(x0.getValue(CPos("B5")), CValue(5625.0)));
    assert(valueMatch(x0.getValue(CPos("B6")), CValue(11250.0)));
    x1 = x0;
    assert(x0.setCell(CPos("A2"), "100"));
    assert(x1.setCell(CPos("A2"), "=A3+A5+A4"));
    assert(valueMatch(x0.getValue(CPos("B1")), CValue(3012.0)));
    assert(valueMatch(x0.getValue(CPos("B2")), CValue(-194.0)));
    assert(valueMatch(x0.getValue(CPos("B3")), CValue(4096.0)));
    assert(valueMatch(x0.getValue(CPos("B4")), CValue(12544.0)));
    assert(valueMatch(x0.getValue(CPos("B5")), CValue(19458.0)));
    assert(valueMatch(x0.getValue(CPos("B6")), CValue(38916.0)));
    assert(valueMatch(x1.getValue(CPos("B1")), CValue(3612.0)));
    assert(valueMatch(x1.getValue(CPos("B2")), CValue(-204.0)));
    assert(valueMatch(x1.getValue(CPos("B3")), CValue(4096.0)));
    assert(valueMatch(x1.getValue(CPos("B4")), CValue(17424.0)));
    assert(valueMatch(x1.getValue(CPos("B5")), CValue(24928.0)));
    assert(valueMatch(x1.getValue(CPos("B6")), CValue(49856.0)));
    oss.clear();
    oss.str("");
    assert(x0.save(oss));
    data = oss.str();
    iss.clear();
    iss.str(data);
    assert(x1.load(iss));
    assert(valueMatch(x1.getValue(CPos("B1")), CValue(3012.0)));
    assert(valueMatch(x1.getValue(CPos("B2")), CValue(-194.0)));
    assert(valueMatch(x1.getValue(CPos("B3")), CValue(4096.0)));
    assert(valueMatch(x1.getValue(CPos("B4")), CValue(12544.0)));
    assert(valueMatch(x1.getValue(CPos("B5")), CValue(19458.0)));
    assert(valueMatch(x1.getValue(CPos("B6")), CValue(38916.0)));
    assert(x0.setCell(CPos("A3"), "4e1"));
    assert(valueMatch(x1.getValue(CPos("B1")), CValue(3012.0)));
    assert(valueMatch(x1.getValue(CPos("B2")), CValue(-194.0)));
    assert(valueMatch(x1.getValue(CPos("B3")), CValue(4096.0)));
    assert(valueMatch(x1.getValue(CPos("B4")), CValue(12544.0)));
    assert(valueMatch(x1.getValue(CPos("B5")), CValue(19458.0)));
    assert(valueMatch(x1.getValue(CPos("B6")), CValue(38916.0)));
    oss.clear();
    oss.str("");
    assert(x0.save(oss));
    data = oss.str();
    for (size_t i = 0; i < std::min<size_t>(data.length(), 10); i++) {
        data[i] ^= 0x5a;
    }
    iss.clear();
    iss.str(data);
    assert(!x1.load(iss));
    assert(x0.setCell(CPos("D0"), "10"));
    assert(x0.setCell(CPos("D1"), "20"));
    assert(x0.setCell(CPos("D2"), "30"));
    assert(x0.setCell(CPos("D3"), "40"));
    assert(x0.setCell(CPos("D4"), "50"));
    assert(x0.setCell(CPos("E0"), "60"));
    assert(x0.setCell(CPos("E1"), "70"));
    assert(x0.setCell(CPos("E2"), "80"));
    assert(x0.setCell(CPos("E3"), "90"));
    assert(x0.setCell(CPos("E4"), "100"));
    assert(x0.setCell(CPos("F10"), "=D0+5"));
    assert(x0.setCell(CPos("F11"), "=$D0+5"));
    assert(x0.setCell(CPos("F12"), "=D$0+5"));
    assert(x0.setCell(CPos("F13"), "=$D$0+5"));
    x0.copyRect(CPos("G11"), CPos("F10"), 1, 4);
    assert(valueMatch(x0.getValue(CPos("F10")), CValue(15.0)));
    assert(valueMatch(x0.getValue(CPos("F11")), CValue(15.0)));
    assert(valueMatch(x0.getValue(CPos("F12")), CValue(15.0)));
    assert(valueMatch(x0.getValue(CPos("F13")), CValue(15.0)));
    assert(valueMatch(x0.getValue(CPos("F14")), CValue()));
    assert(valueMatch(x0.getValue(CPos("G10")), CValue()));
    assert(valueMatch(x0.getValue(CPos("G11")), CValue(75.0)));
    assert(valueMatch(x0.getValue(CPos("G12")), CValue(25.0)));
    assert(valueMatch(x0.getValue(CPos("G13")), CValue(65.0)));
    assert(valueMatch(x0.getValue(CPos("G14")), CValue(15.0)));
    x0.copyRect(CPos("G11"), CPos("F10"), 2, 4);
    assert(valueMatch(x0.getValue(CPos("F10")), CValue(15.0)));
    assert(valueMatch(x0.getValue(CPos("F11")), CValue(15.0)));
    assert(valueMatch(x0.getValue(CPos("F12")), CValue(15.0)));
    assert(valueMatch(x0.getValue(CPos("F13")), CValue(15.0)));
    assert(valueMatch(x0.getValue(CPos("F14")), CValue()));
    assert(valueMatch(x0.getValue(CPos("G10")), CValue()));
    assert(valueMatch(x0.getValue(CPos("G11")), CValue(75.0)));
    assert(valueMatch(x0.getValue(CPos("G12")), CValue(25.0)));
    assert(valueMatch(x0.getValue(CPos("G13")), CValue(65.0)));
    assert(valueMatch(x0.getValue(CPos("G14")), CValue(15.0)));
    assert(valueMatch(x0.getValue(CPos("H10")), CValue()));
    assert(valueMatch(x0.getValue(CPos("H11")), CValue()));
    assert(valueMatch(x0.getValue(CPos("H12")), CValue()));
    assert(valueMatch(x0.getValue(CPos("H13")), CValue(35.0)));
    assert(valueMatch(x0.getValue(CPos("H14")), CValue()));
    assert(x0.setCell(CPos("F0"), "-27"));
    assert(valueMatch(x0.getValue(CPos("H14")), CValue(-22.0)));
    x0.copyRect(CPos("H12"), CPos("H13"), 1, 2);
    assert(valueMatch(x0.getValue(CPos("H12")), CValue(25.0)));
    assert(valueMatch(x0.getValue(CPos("H13")), CValue(-22.0)));
    assert(valueMatch(x0.getValue(CPos("H14")), CValue(-22.0)));
    return EXIT_SUCCESS;
}
