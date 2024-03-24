#ifndef __PROGTEST__
    #include <cassert>
    #include <cmath>
    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <iomanip>
    #include <iostream>
    #include <memory>
    #include <stdexcept>
#endif /* __PROGTEST__ */

class CPatchStr {
  public:
    CPatchStr();
    CPatchStr(const char* str);
    // copy constructor
    // destructor
    // operator =
    CPatchStr subStr(size_t from, size_t len) const;
    CPatchStr& append(const CPatchStr& src);

    CPatchStr& insert(size_t pos, const CPatchStr& src);
    CPatchStr& remove(size_t from, size_t len);
    char* toStr() const;

  private:
    // todo
};

#ifndef __PROGTEST__
bool stringMatch(char* str, const char* expected) {
    bool res = std::strcmp(str, expected) == 0;
    delete[] str;
    return res;
}

int main() {
    char tmpStr[100];

    CPatchStr a("test");
    assert(stringMatch(a.toStr(), "test"));
    std::strncpy(tmpStr, " da", sizeof(tmpStr) - 1);
    a.append(tmpStr);
    assert(stringMatch(a.toStr(), "test da"));
    std::strncpy(tmpStr, "ta", sizeof(tmpStr) - 1);
    a.append(tmpStr);
    assert(stringMatch(a.toStr(), "test data"));
    std::strncpy(tmpStr, "foo text", sizeof(tmpStr) - 1);
    CPatchStr b(tmpStr);
    assert(stringMatch(b.toStr(), "foo text"));
    CPatchStr c(a);
    assert(stringMatch(c.toStr(), "test data"));
    CPatchStr d(a.subStr(3, 5));
    assert(stringMatch(d.toStr(), "t dat"));
    d.append(b);
    assert(stringMatch(d.toStr(), "t datfoo text"));
    d.append(b.subStr(3, 4));
    assert(stringMatch(d.toStr(), "t datfoo text tex"));
    c.append(d);
    assert(stringMatch(c.toStr(), "test datat datfoo text tex"));
    c.append(c);
    assert(stringMatch(
        c.toStr(),
        "test datat datfoo text textest datat datfoo text tex"
    ));
    d.insert(2, c.subStr(6, 9));
    assert(stringMatch(d.toStr(), "t atat datfdatfoo text tex"));
    b = "abcdefgh";
    assert(stringMatch(b.toStr(), "abcdefgh"));
    assert(stringMatch(d.toStr(), "t atat datfdatfoo text tex"));
    assert(stringMatch(d.subStr(4, 8).toStr(), "at datfd"));
    assert(stringMatch(b.subStr(2, 6).toStr(), "cdefgh"));
    try {
        b.subStr(2, 7).toStr();
        assert("Exception not thrown" == nullptr);
    } catch (const std::out_of_range& e) {
    } catch (...) {
        assert("Invalid exception thrown" == nullptr);
    }
    a.remove(3, 5);
    assert(stringMatch(a.toStr(), "tesa"));
    return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
