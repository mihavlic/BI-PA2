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

    Part split(unsigned at) {
        assert(at < size);

        Part right(*this);
        right.offset += at;
        right.size -= at;

        this->size = at;

        return right;
    }

    void remove_left(unsigned count) {
        assert(count < size);
        offset += count;
        size -= count;
    }

    bool empty() const {
        return size == 0;
    }

    unsigned len() const {
        return size;
    }
};

template<typename T>
class Vector {
    size_t m_size = 0;
    size_t m_capacity = 0;
    T* m_data = nullptr;

  public:
    Vector() {}

    Vector(Vector& copy) {
        m_size = copy.m_size;
        m_capacity = copy.m_size;
        m_data = (T*)(malloc(m_size * sizeof(T)));

        memcpy(m_data, copy.m_data, m_size * sizeof(T));
    }

    Vector(Vector&& move) {
        m_size = move.m_size;
        m_capacity = move.m_size;
        m_data = move.m_data;

        move.m_size = 0;
        move.m_capacity = 0;
        move.m_data = nullptr;
    }

    Vector(T value) : Vector() {
        this->push(value);
    }

    ~Vector() {
        for (size_t i = 0; i < m_size; i++) {
            m_data[i].~T();
        }
        free(m_data);
    }

    size_t size() const {
        return m_size;
    }

    void ensure_capacity(size_t new_size) {
        if (new_size > m_capacity) {
            size_t new_capacity = m_capacity * 2;
            if (new_size > new_capacity) {
                new_capacity = new_size;
            }

            T* new_data = (T*)malloc(new_size * sizeof(T));
            std::memcpy(new_data, m_data, m_size * sizeof(T));

            free(m_data);
            m_data = new_data;
            m_capacity = new_capacity;
        }
    }

    void push(T value) {
        size_t new_size = m_size + 1;
        ensure_capacity(new_size);
        m_data[m_size] = value;

        m_size = new_size;
    }

    void
    insert(size_t dst_start, size_t dst_len, const T* data, size_t data_len) {
        if (data_len > dst_len) {
            ensure_capacity(m_size + data_len - dst_len);
        }

        for (int i = 0; i < dst_len; i++) {
            m_data[dst_start + i]->~T();
        }

        if (dst_len > 0) {
            memmove(
                m_data + dst_start + data_len,
                m_data + dst_start + dst_len,
                (m_size - (dst_start + dst_len)) * sizeof(T)
            );
        }

        for (int i = 0; i < data_len; i++) {
            new (m_data[dst_start + i]) T(data[i]);
        }
    }

    void clear() {
        m_size = 0;
    }

    T& operator[](size_t pos) {
        assert(pos < m_size);
        return m_data[pos];
    }

    const T& operator[](size_t pos) const {
        assert(pos < m_size);
        return m_data[pos];
    }

    template<typename F>
    void retain(F lambda) {
        size_t out = 0;
        for (size_t i = 0; i < m_size; i++) {
            T& el = m_data[i];
            if (lambda(el)) {
                memmove(m_data + out, m_data + i, sizeof(T));
                out++;
            } else {
                el.~T();
            }
        }
        m_size = out;
    }
};

class CPatchStr {
    Vector<Part> parts;

  public:
    CPatchStr();

    CPatchStr(const char* str) : parts(Part(str)) {}

    void operator=(CPatchStr other) {
        *this = other;
    }

    void operator=(const char* str) {
        parts.clear();
        parts.push(Part(str));
    }

    // copy constructor
    // destructor
    // operator =
    CPatchStr subStr(size_t from, size_t len) const {
        return "";
    }

    CPatchStr& append(const CPatchStr& src) {}

    CPatchStr& insert(size_t pos, const CPatchStr& src) {
        unsigned offset = 0;
        ListElement* prev = nullptr;
        ListElement* current = head;

        while (current && pos < offset) {
            unsigned part_end = offset + current->data.len();

            // pos is in the middle of a part, need to split it
            if (pos < part_end) {
                Part right_part = current->data.split(pos - offset);
                ListElement* right =
                    new ListElement {.data = right_part, .next = current->next};

                current->next = nullptr;
                prev = current;
                current = right;

                break;
            }

            offset = part_end;
            prev = current;
            current = current->next;
        }

        if (pos > offset) {
            throw std::out_of_range("bungus");
        }

        ListElement** pdst = &prev->next;
        ListElement* psrc = src.head;

        while (psrc) {
            ListElement* copy = new ListElement(*psrc);
            *pdst = copy;

            pdst = &copy->next;
            psrc = psrc->next;
        }

        *pdst = current;

        return *this;
    }

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
