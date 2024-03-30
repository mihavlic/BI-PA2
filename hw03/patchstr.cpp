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

class CPatchStr;

class RcString {
    unsigned* count;

  public:
    RcString(const char* str, size_t len) {
        size_t bytes = len + sizeof(unsigned)
            + 1;  // including refcount and terminating \0
        char* data = (char*)malloc(bytes);

        *(unsigned*)data = 1;
        memcpy(data + 4, str, len);
    }

    RcString(const RcString& a) : count(a.count) {
        (*a.count)++;
    }

    RcString(RcString&& a) : count(a.count) {
        a.count = nullptr;
    }

    ~RcString() {
        if (count) {
            if (--(*count) == 0) {
                free(count);
            }
        }
    }

    const RcString& operator=(RcString other) {
        *this = other;
        return *this;
    }

    unsigned refcount() const {
        return *count;
    }

    const char* get() const {
        return (const char*)(count + 4);
    }
};

class Part {
    unsigned offset;
    unsigned size;
    RcString data;

  public:
    Part(const char* str) :
        offset(0),
        size(strlen(str)),
        data(RcString(str, size)) {}

    void remove_start(unsigned count) {
        assert(count <= size);
        offset += count;
        size -= count;
    }

    void remove_end(unsigned count) {
        assert(count <= size);
        size -= count;
    }

    bool empty() const {
        return size == 0;
    }
};

struct ListElement {
    Part self;
    ListElement* next = nullptr;
};

class List {
    ListElement* head;

  public:
    List() {}

    List(List&& a) : head(a.head) {
        a.head = nullptr;
    }

    List(List& a) {
        ListElement** next = &head;
        ListElement* cursor = a.head;
        while (cursor) {
            ListElement* copy = new ListElement(*cursor);
            *next = copy;

            next = &copy->next;
            cursor = cursor->next;
        }
    }

    List(Part value) : head(new ListElement {value, nullptr}) {}

    ~List() {
        ListElement* cursor = head;
        while (cursor) {
            ListElement* tmp = cursor;
            cursor = tmp->next;
            free(tmp);
        }
    }
};

class CPatchStr {
    List parts;

  public:
    CPatchStr();

    CPatchStr(CPatchStr& a) : parts(a.parts) {}

    CPatchStr(CPatchStr&& a) : parts(std::move(a.parts)) {}

    CPatchStr(const char* str) : parts(List(Part(str))) {}

    void operator=(CPatchStr other) {
        *this = other;
    }

    void operator=(const char* str) {
        *this = CPatchStr(str);
    }

    // copy constructor
    // destructor
    // operator =
    CPatchStr subStr(size_t from, size_t len) const {
        return "";
    }

    CPatchStr& append(const CPatchStr& src);

    CPatchStr& insert(size_t pos, const CPatchStr& src);
    CPatchStr& remove(size_t from, size_t len);
    char* toStr() const;
};

#ifndef __PROGTEST__
bool stringMatch(char* str, const char* expected) {
    bool res = std::strcmp(str, expected) == 0;
    if (!res) {
        std::cout << "expected: " << str << "\ngot:      " << expected
                  << std::endl;
    }
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
