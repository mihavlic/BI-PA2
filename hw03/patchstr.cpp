#ifndef __PROGTEST__
    #include <cassert>
    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <iostream>
    #include <stdexcept>
#endif /* __PROGTEST__ */

class CPatchStr;

class RcString {
    unsigned* count;

  public:
    RcString(const char* str, size_t len) {
        size_t bytes = len + sizeof(unsigned);
        count = (unsigned*)malloc(bytes);

        *count = 1;
        memcpy(count + 1, str, len);
    }

    RcString(const RcString& a) : count(a.count) {
        if (count) {
            (*count)++;
        }
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

    unsigned refcount() const {
        if (count) {
            return *count;
        } else {
            return 0;
        }
    }

    const char* get() const {
        return (const char*)(count + 1);
    }
};

struct Range {
    size_t start;
    size_t end;

    Range() : start(0), end(0) {}

    Range(size_t start_, size_t end_) : start(start_), end(end_) {}

    bool empty() const {
        return end <= start;
    }

    size_t size() const {
        if (empty()) {
            return 0;
        }
        return end - start;
    }

    Range subtract_base(size_t base) {
        return Range {start - base, end - base};
    }

    bool operator==(const Range& other) {
        return start == other.start && end == other.end;
    }
};

class Part {
    size_t offset;
    size_t size;
    RcString data;

  public:
    Part(Part&& move) :
        offset(move.offset),
        size(move.size),
        data(std::move(move.data)) {}

    Part(const Part& copy) :
        offset(copy.offset),
        size(copy.size),
        data(copy.data) {}

    Part(const char* str) :
        offset(0),
        size(strlen(str)),
        data(RcString(str, size)) {}

    void shrink_to(Range range) {
        assert(range.start < size);
        assert(range.end <= size);

        offset += range.start;
        size = range.size();
    }

    Part split(size_t at) {
        assert(at < size);
        assert(at > 0);

        Part right(*this);
        right.offset += at;
        right.size -= at;

        this->size = at;

        return right;
    }

    bool empty() const {
        return size == 0;
    }

    size_t len() const {
        return size;
    }

    const char* str() const {
        return data.get() + offset;
    }
};

template<typename T>
void copy_into_unitialized(T* dst, const T* src, size_t len) {
    if (src == dst || len == 0) {
        return;
    }

    if (dst > src) {
        for (size_t i = len; i--;) {
            new (dst + i) T(src[i]);
        }
    } else {
        for (size_t i = 0; i < len; i++) {
            new (dst + i) T(src[i]);
        }
    }
}

template<typename T>
void move_into_unitialized(T* dst, T* src, size_t len) {
    if (src == dst || len == 0) {
        return;
    }

    if (dst > src) {
        for (size_t i = len; i--;) {
            new (dst + i) T(std::move(src[i]));
        }
    } else {
        for (size_t i = 0; i < len; i++) {
            new (dst + i) T(std::move(src[i]));
        }
    }
}

template<typename T>
class Vector {
    size_t m_size = 0;
    size_t m_capacity = 0;
    T* m_data = nullptr;

  public:
    Vector() {}

    Vector(const Vector& copy) {
        m_size = copy.m_size;
        m_capacity = copy.m_size;
        m_data = (T*)(malloc(m_size * sizeof(T)));

        for (size_t i = 0; i < copy.m_size; i++) {
            new (m_data + i) T(copy[i]);
        }
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

    void operator=(const Vector<T>& other) {
        if (this != &other) {
            clear();
            append(other);
        }
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

            T* new_data = (T*)malloc(new_capacity * sizeof(T));
            move_into_unitialized(new_data, m_data, m_size);

            free(m_data);
            m_data = new_data;
            m_capacity = new_capacity;
        }
    }

    void _insert_internal(
        size_t dst_start,
        size_t dst_len,
        const T* data,
        size_t data_len,
        size_t new_size
    ) {
        for (size_t i = 0; i < dst_len; i++) {
            m_data[dst_start + i].~T();
        }

        auto dst = dst_start + data_len;
        auto src = dst_start + dst_len;
        move_into_unitialized(
            m_data + dst,
            m_data + src,
            m_size - (dst_start + dst_len)
        );

        copy_into_unitialized(m_data + dst_start, data, data_len);

        m_size = new_size;
    }

    void
    insert(size_t dst_start, size_t dst_len, const T* data, size_t data_len) {
        assert(dst_start + dst_len <= m_size);

        size_t new_size = (m_size + data_len) - dst_len;
        ensure_capacity(new_size);

        _insert_internal(dst_start, dst_len, data, data_len, new_size);
    }

    void insert(size_t dst_start, size_t dst_len, const Vector<T>& vec) {
        assert(dst_start + dst_len <= m_size);

        size_t new_size = (m_size + vec.size()) - dst_len;
        ensure_capacity(new_size);

        _insert_internal(dst_start, dst_len, vec.begin(), vec.size(), new_size);
    }

    void insert(size_t dst_pos, const T& value) {
        insert(dst_pos, 0, &value, 1);
    }

    void append(const T* data, size_t data_len) {
        insert(m_size, 0, data, data_len);
    }

    void append(const Vector<T>& vec) {
        insert(m_size, 0, vec);
    }

    void push(T value) {
        size_t new_size = m_size + 1;
        ensure_capacity(new_size);
        new (m_data + m_size) T(std::move(value));

        m_size = new_size;
    }

    void clear() {
        for (T& part : *this) {
            part.~T();
        }
        m_size = 0;
    }

    void remove(size_t pos) {
        assert(pos < m_size);
        m_data[pos].~T();

        move_into_unitialized(
            m_data + pos,
            m_data + pos + 1,
            (m_size - pos) - 1
        );

        m_size -= 1;
    }

    T& operator[](size_t pos) {
        assert(pos < m_size);
        return m_data[pos];
    }

    const T& operator[](size_t pos) const {
        assert(pos < m_size);
        return m_data[pos];
    }

    T* begin() {
        return m_data;
    }

    T* end() {
        return m_data + m_size;
    }

    const T* begin() const {
        return m_data;
    }

    const T* end() const {
        return m_data + m_size;
    }
};

size_t subtract_ranges(const Range& a, const Range& b, Range out[2]) {
    if (a.empty()) {
        return 0;
    }
    if (b.empty() || (a.start < b.start && a.end <= b.start)
        || (b.start < a.start && b.end <= a.start)) {
        // doesn't overlap
        out[0] = a;
        return 1;
    }
    if ((b.start <= a.start && a.end <= b.end)) {
        // overlaps completely
        return 0;
    }
    if (a.start <= b.start && b.start < a.end && a.end <= b.end) {
        // overlaps the end
        out[0] = Range {a.start, b.start};
        return 1;
    }
    if (b.start <= a.start && a.start < b.end && b.end <= a.end) {
        // overlaps the start
        out[0] = Range {b.end, a.end};
        return 1;
    }
    {
        // somewhere in the middle
        out[0] = Range {a.start, b.start};
        out[1] = Range {b.end, a.end};
        return 2;
    }
}

Range intersect_ranges(const Range& a, const Range& b) {
    return Range(std::max(a.start, b.start), std::min(a.end, b.end));
}

void test_subtract_ranges() {
    Range empty1(0, 0);
    Range empty2(1, 1);
    Range empty3(2, 1);

    Range out[2];
    assert(subtract_ranges(empty1, empty1, out) == 0);
    assert(subtract_ranges(empty2, empty1, out) == 0);
    assert(subtract_ranges(empty1, empty2, out) == 0);
    assert(subtract_ranges(empty1, empty3, out) == 0);
    assert(subtract_ranges(empty3, empty3, out) == 0);

    Range a(2, 5);
    assert(subtract_ranges(a, a, out) == 0);

    assert(subtract_ranges(a, empty3, out) == 1);
    assert(out[0] == a);

    Range b(5, 6);
    assert(subtract_ranges(a, b, out) == 1);
    assert(out[0] == a);

    assert(subtract_ranges(b, a, out) == 1);
    assert(out[0] == b);

    Range c(4, 6);
    assert(subtract_ranges(a, c, out) == 1);
    assert(out[0] == Range(2, 4));

    assert(subtract_ranges(c, a, out) == 1);
    assert(out[0] == Range(5, 6));

    Range d(3, 4);
    assert(subtract_ranges(a, d, out) == 2);
    assert(out[0] == Range(2, 3));
    assert(out[1] == Range(4, 5));
}

class CPatchStr {
    Vector<Part> m_parts;

  public:
    CPatchStr() {}

    CPatchStr(const CPatchStr& copy) : m_parts(copy.m_parts) {}

    CPatchStr(const char* str) : m_parts(Part(str)) {}

    void operator=(const CPatchStr& other) {
        this->m_parts = other.m_parts;
    }

    void operator=(const char* str) {
        m_parts.clear();
        m_parts.push(Part(str));
    }

    CPatchStr subStr(size_t from, size_t len) const {
        Range range {from, from + len};

        CPatchStr substr {};

        size_t offset = 0;

        for (const Part& part : m_parts) {
            Range part_range {offset, offset + part.len()};

            Range out = intersect_ranges(part_range, range);

            if (!out.empty()) {
                Part copy {part};
                copy.shrink_to(out.subtract_base(offset));
                substr.m_parts.push(copy);
            }

            offset = part_range.end;
        }

        if (range.start > offset || range.end > offset) {
            throw std::out_of_range("Out of range");
        }

        return substr;
    }

    // CPatchStr& append(const char* src) {
    //     Part tmp(src);
    //     m_parts.append(tmp);
    //     return *this;
    // }

    CPatchStr& append(const CPatchStr& src) {
        m_parts.append(src.m_parts);
        return *this;
    }

    CPatchStr& _insert_internal(size_t pos, const CPatchStr& src) {
        size_t offset = 0;

        size_t i = 0;
        for (; i < m_parts.size(); i++) {
            Part& part = m_parts[i];
            size_t part_start = offset;
            size_t part_end = part_start + part.len();

            if (part_start == pos) {
                break;
            }

            // pos is in the middle of a part, need to split it
            if (pos < part_end) {
                Part right = part.split(pos - part_start);

                offset += part.len();
                i++;

                m_parts.insert(i, right);
                break;
            }

            offset = part_end;
        }

        if (pos > offset) {
            throw std::out_of_range("Out of range");
        }

        m_parts.insert(i, 0, src.m_parts);

        return *this;
    }

    CPatchStr& insert(size_t pos, const CPatchStr& src) {
        if (this == &src) {
            CPatchStr copy(src);
            return _insert_internal(pos, copy);
        } else {
            return _insert_internal(pos, src);
        }
    }

    CPatchStr& remove(size_t from, size_t len) {
        Range range {from, from + len};

        size_t offset = 0;

        for (size_t i = 0; i < m_parts.size() && offset < range.end;) {
            Part* part = &m_parts[i];
            Range part_range {offset, offset + part->len()};

            Range out[2];
            size_t count = subtract_ranges(part_range, range, out);

            if (count == 0) {
                m_parts.remove(i);
            } else {
                if (count == 2) {
                    Part right {*part};
                    right.shrink_to(out[1].subtract_base(offset));
                    m_parts.insert(i + 1, right);
                    part = &m_parts[i];
                    i++;
                }
                part->shrink_to(out[0].subtract_base(offset));
                i++;
            }

            offset = part_range.end;
        }

        if (range.end > offset) {
            throw std::out_of_range("Out of range");
        }

        return *this;
    }

    char* toStr() const {
        size_t size = 0;
        for (const Part& part : m_parts) {
            size += part.len();
        }

        char* str = new char[size + 1];

        size_t offset = 0;
        for (const Part& part : m_parts) {
            memcpy(str + offset, part.str(), part.len());
            offset += part.len();
        }
        str[offset] = 0;

        return str;
    }
};

#ifndef __PROGTEST__

bool stringMatch(const CPatchStr& pstr, const char* expected) {
    char* str = pstr.toStr();
    bool res = std::strcmp(str, expected) == 0;
    if (!res) {
        std::cout << "expected: " << expected << "\ngot:      " << str
                  << std::endl;
    }

    delete[] str;
    return res;
}

// #define FUZZ

    #ifdef FUZZ
        #include "../out/fuzz.cpp"
    #endif

int main() {
    test_subtract_ranges();

    #ifdef FUZZ
    fuzz();
    // return 0;
    #endif

    char tmpStr[100];

    CPatchStr g("aaaa");
    g = g;
    g = "aaaa";
    g.remove(0, 0);
    g.remove(4, 0);
    assert(stringMatch(g, "aaaa"));

    g.insert(4, "bbb");
    assert(stringMatch(g, "aaaabbb"));

    CPatchStr a("test");
    assert(stringMatch(a, "test"));
    std::strncpy(tmpStr, " da", sizeof(tmpStr) - 1);
    a.append(tmpStr);
    assert(stringMatch(a, "test da"));
    std::strncpy(tmpStr, "ta", sizeof(tmpStr) - 1);
    a.append(tmpStr);
    assert(stringMatch(a, "test data"));
    std::strncpy(tmpStr, "foo text", sizeof(tmpStr) - 1);
    CPatchStr b(tmpStr);
    assert(stringMatch(b, "foo text"));
    CPatchStr c(a);
    assert(stringMatch(c, "test data"));

    assert(stringMatch(a, "test data"));
    CPatchStr d(a.subStr(3, 5));
    assert(stringMatch(d, "t dat"));
    d.append(b);
    assert(stringMatch(d, "t datfoo text"));
    d.append(b.subStr(3, 4));
    assert(stringMatch(d, "t datfoo text tex"));
    c.append(d);
    assert(stringMatch(c, "test datat datfoo text tex"));
    c.append(c);
    assert(stringMatch(
        c,
        "test datat datfoo text textest datat datfoo text tex"
        //            atat datf
    ));
    assert(stringMatch(c.subStr(6, 9), "atat datf"));
    d.insert(2, c.subStr(6, 9));
    assert(stringMatch(d, "t atat datfdatfoo text tex"));
    b = "abcdefgh";
    assert(stringMatch(b, "abcdefgh"));
    assert(stringMatch(d, "t atat datfdatfoo text tex"));
    assert(stringMatch(d.subStr(4, 8), "at datfd"));
    assert(stringMatch(b.subStr(2, 6), "cdefgh"));
    try {
        b.subStr(2, 7);
        assert("Exception not thrown" == nullptr);
    } catch (const std::out_of_range& e) {
    } catch (...) {
        assert("Invalid exception thrown" == nullptr);
    }
    a.remove(3, 5);
    assert(stringMatch(a, "tesa"));
    return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
