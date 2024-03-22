#ifndef __PROGTEST__
#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <cmath>
#include <compare>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#endif /* __PROGTEST__ */

// much of this code was initially taken
// from https://github.com/rust-num/num-bigint
// but since then I've made significant changes (some of the code was pretty
// bad)

template <typename T> struct Pair {
  T first;
  T second;
};

template <typename T> struct Slice {
  T *start;
  size_t len;

  Slice(std::vector<T> &vector) : start(vector.data()), len(vector.size()) {}
  Slice(T *ptr) : start(ptr), len(1) {}
  Slice(T *ptr, size_t size) : start(ptr), len(size) {}
  Slice(T *start_, T *end_) : start(start_), len(0) {
    if (start_ < end_) {
      len = end_ - start_;
    }
  }

  bool empty() const { return len == 0; }
  size_t size() const { return len; }
  void next() {
    if (!empty()) {
      start++;
      len--;
    }
  }
  void next_by(size_t offset) { *this = start_at(offset); }
  T *begin() const { return start; }
  T *end() const { return start + len; }
  Pair<Slice<T>> split_at(size_t at) {
    assert(at <= size());

    Slice<T> a{start, at};
    Slice<T> b{start + at, len - at};

    return Pair<Slice<T>>{a, b};
  }
  Slice<T> clamp_size(size_t clamp) {
    size_t len = std::min(size(), clamp);
    return Slice<T>(start, len);
  }
  Slice<T> start_at(size_t offset) {
    size_t offset_ = std::min(size(), offset);
    return Slice<T>(start + offset_, len - offset_);
  }
  T &operator[](size_t index) { return start[index]; }
};

template <typename T> struct ConstSlice {
  const T *start;
  size_t len;

  ConstSlice(const std::vector<T> &vector)
      : start(vector.data()), len(vector.size()) {}
  ConstSlice(const T *ptr) : start(ptr), len(1) {}
  ConstSlice(const T *ptr, size_t size) : start(ptr), len(size) {}
  ConstSlice(const T *start_, const T *end_) : start(start_), len(0) {
    if (start_ < end_) {
      len = end_ - start_;
    }
  }
  ConstSlice(Slice<T> slice) : start(slice.start), len(slice.len) {}

  bool empty() const { return len == 0; }
  size_t size() const { return len; }
  void next() {
    if (!empty()) {
      start++;
      len--;
    }
  }
  void next_by(size_t offset) { *this = start_at(offset); }
  const T *begin() const { return start; }
  const T *end() const { return start + len; }
  Pair<ConstSlice<T>> split_at(size_t at) {
    assert(at <= size());

    ConstSlice<T> a{start, at};
    ConstSlice<T> b{start + at, len - at};

    return Pair<ConstSlice<T>>{a, b};
  }
  ConstSlice<T> clamp_size(size_t clamp) {
    size_t len = std::min(size(), clamp);
    return ConstSlice<T>(start, len);
  }
  ConstSlice<T> start_at(size_t offset) {
    size_t offset_ = std::min(size(), offset);
    return ConstSlice<T>(start + offset_, len - offset_);
  }
  const T &operator[](size_t index) const { return start[index]; }
};

using Digit = uint32_t;
using DoubleDigit = uint64_t;
constexpr Digit DIGIT_BITS = sizeof(Digit) * 8;

using Digits = Slice<Digit>;
using ConstDigits = ConstSlice<Digit>;
using OwnedDigits = std::vector<Digit>;

inline Digit double_high(DoubleDigit digit) {
  return (Digit)(digit >> DIGIT_BITS);
}
inline Digit double_low(DoubleDigit digit) { return (Digit)digit; }
inline DoubleDigit double_pack(Digit high, Digit low) {
  return (DoubleDigit)low | ((DoubleDigit)high << DIGIT_BITS);
}

// return a + b, overflow in carry
inline Digit add_carry_u32(uint32_t a, uint32_t b, uint32_t *carry) {
  uint32_t s;
  uint32_t c1 = __builtin_add_overflow(a, b, &s);
  uint32_t c2 = __builtin_add_overflow(s, *carry, &s);
  *(carry) = c1 | c2;

  return s;
}

// return a - b, overflow in carry
inline Digit sub_carry_u32(uint32_t a, uint32_t b, uint32_t *carry) {
  uint32_t s;
  uint32_t c1 = __builtin_sub_overflow(a, b, &s);
  uint32_t c2 = __builtin_sub_overflow(s, *carry, &s);

  *(carry) = c1 | c2;
  return s;
}

// x = carry + a + b * c
// return low_bits(x), carry = high_bits(carry) >> DIGIT_BITS
inline Digit mac_carry_u32(Digit a, Digit b, Digit c, Digit *acc) {
  DoubleDigit d = (DoubleDigit)*acc;
  d += (DoubleDigit)a;
  d += (DoubleDigit)b * (DoubleDigit)c;

  *acc = double_high(d);
  return double_low(d);
}

// x = carry + a * b
// return low_bits(x), carry = high_bits(carry) >> DIGIT_BITS
inline Digit mul_carry_u32(Digit a, Digit b, Digit *acc) {
  DoubleDigit d = (DoubleDigit)*acc;
  d += (DoubleDigit)a * (DoubleDigit)b;

  *acc = double_high(d);
  return double_low(d);
}

// a += b, return carry
Digit add2(Digits a, ConstDigits b) {
  size_t a_len = a.size();
  size_t b_len = b.size();
  assert(a_len >= b_len);

  Digit carry = 0;
  size_t i = 0;

  for (; i < b_len; i++) {
    a[i] = add_carry_u32(a[i], b[i], &carry);
  }

  for (; i < a_len && carry != 0; i++) {
    a[i] = add_carry_u32(a[i], 0, &carry);
  }

  return carry;
}

// a -= b, return carry
Digit sub2(Digits a, ConstDigits b) {
  size_t a_len = a.size();
  size_t len = std::min(a_len, b.size());

  Digit carry = 0;
  size_t i = 0;

  for (; i < len; i++) {
    a[i] = sub_carry_u32(a[i], b[i], &carry);
  }

  for (; i < a_len && carry != 0; i++) {
    a[i] = sub_carry_u32(a[i], 0, &carry);
  }

  return carry;
}

// invert digit bits, then add 1
void twos_complement(Digits a) {
  size_t len = a.size();

  Digit carry = 1;
  size_t i = 0;

  for (; i < len; i++) {
    a[i] = ~a[i];
    a[i] = add_carry_u32(a[i], 0, &carry);
  }
}

// a += b
Digit add_digit(Digits a, Digit b) {
  size_t len = a.size();

  Digit carry = b;
  for (size_t i = 0; i < len && carry != 0; i++) {
    a[i] = add_carry_u32(a[i], 0, &carry);
  }

  return carry;
}

// a *= b
Digit sub_digit(Digits a, Digit b) {
  size_t len = a.size();

  Digit carry = b;
  for (size_t i = 0; i < len && carry != 0; i++) {
    a[i] = sub_carry_u32(a[i], 0, &carry);
  }

  return carry;
}

// a *= b
Digit mul_digit(Digits a, Digit b) {
  size_t len = a.size();

  Digit carry = 0;
  for (size_t i = 0; i < len; i++) {
    a[i] = mul_carry_u32(a[i], b, &carry);
  }

  return carry;
}

// a += b * c
Digit mac_digit(Digits a, ConstDigits b, Digit c) {
  if (c == 0) {
    return 0;
  }

  assert(a.size() >= b.size());
  size_t b_len = b.size();

  Digit carry = 0;
  for (size_t i = 0; i < b_len; i++) {
    a[i] = mac_carry_u32(a[i], b[i], c, &carry);
  }

  if (carry != 0) {
    carry = add_digit(a.start_at(b_len), carry);
  }

  return carry;
}

// return a / b, *rem = a % b
inline Digit div_wide(Digit a, Digit b, Digit *rem) {
  DoubleDigit lhs = double_pack(*rem, a);
  DoubleDigit rhs = (DoubleDigit)b;

  auto q_r = std::ldiv(lhs, rhs);

  *rem = (Digit)q_r.rem;
  return (Digit)q_r.quot;
}

// divides a, returns rem
Digit div_rem_digit(Digits a, Digit divisor) {
  if (a.empty()) {
    return 0;
  }

  Digit rem = 0;
  for (size_t i = a.size(); i--;) {
    a[i] = div_wide(a[i], divisor, &rem);
  }

  return rem;
}

Digit parse_digit_decimal(const char *str, size_t len) {
  Digit digit = 0;
  for (size_t i = 0; i < len; i++) {
    digit *= 10;
    digit += str[i] - '0';
  }

  return digit;
}

ConstDigits normalize_slice(ConstDigits digits) {
  if (digits.empty()) {
    return digits;
  }

  size_t i = digits.size();
  while (i-- && digits[i] == 0) {
  }
  return ConstDigits(digits.begin(), digits.begin() + i + 1);
}

void mac3(Digits acc, ConstDigits b, ConstDigits c);

class CBigInt {
  bool is_negative = false;
  OwnedDigits digits;

public:
  CBigInt() {}
  CBigInt(CBigInt &&a)
      : is_negative(a.is_negative), digits(std::move(a.digits)) {}
  CBigInt(const CBigInt &a) : is_negative(a.is_negative), digits(a.digits) {}
  explicit CBigInt(ConstDigits digits_)
      : digits(digits_.begin(), digits_.end()) {
    normalize();
  }
  explicit CBigInt(Digit digit) : digits({digit}) { normalize(); }
  explicit CBigInt(int digit)
      : is_negative(digit < 0), digits({(Digit)std::abs((signed long)digit)}) {
    normalize();
  }
  explicit CBigInt(long digit)
      : is_negative(digit < 0), digits({(Digit)std::abs(digit)}) {
    normalize();
  }
  explicit CBigInt(const char *cursor, const char *end) {
    ConstSlice<char> view(cursor, end);

    if (view.empty()) {
      throw std::invalid_argument("Empty");
    }

    if (view[0] == '-') {
      this->is_negative = true;
      view.next();
    } else if (view[0] == '+') {
      view.next();
    }

    if (view.empty()) {
      throw std::invalid_argument("Empty");
    }

    // check that all characters are 0-9
    ConstSlice<char> copy = view;
    for (char c : copy) {
      if (!('0' <= c && c <= '9')) {
        throw std::invalid_argument("Not 0-9");
      }
    }

    // estimate number of bits needed to store the number
    // assume the worst case that all digits are 9
    double bits = std::log2(10.0) * view.size();
    size_t big_digits = std::ceil(bits / DIGIT_BITS);
    this->digits.reserve(big_digits);

    Digit chunk_size = 9;             // 10^9 fits in 2^32, 10^10 does not
    Digit chunk_base = 1'000'000'000; // 10^power

    size_t head = view.size() % chunk_size;
    Digit digit = parse_digit_decimal(view.begin(), head);
    view.next_by(head);

    this->digits.push_back(digit);

    while (!view.empty()) {
      Digit digit = parse_digit_decimal(view.begin(), chunk_size);

      *this *= chunk_base;
      *this += digit;

      view.next_by(chunk_size);
    }

    normalize();
  }
  explicit CBigInt(const std::string &str)
      : CBigInt(str.data(), str.data() + str.size()) {}
  explicit CBigInt(const char *cstr) : CBigInt(cstr, cstr + strlen(cstr)) {}
  void from_slice(ConstDigits slice) {
    digits.clear();
    digits.insert(digits.begin(), slice.begin(), slice.end());
    normalize();
  }
  void negate() { is_negative = !is_negative; }
  Digits slice() { return Digits(digits); }
  ConstDigits const_slice() const { return ConstDigits(digits); }
  OwnedDigits &vector() { return digits; }
  size_t size() const { return digits.size(); }
  bool empty() const { return digits.empty(); }
  void clear() {
    is_negative = false;
    digits.clear();
  }
  void make_zeros(size_t size) {
    digits.clear();
    digits.resize(size, 0);
  }
  void normalize() {
    ConstDigits normalized = normalize_slice(digits);
    digits.erase(digits.begin() + normalized.size(), digits.end());
    if (empty()) {
      is_negative = false;
    }
  }
  int cmp(const CBigInt &other) const {
    if (empty() && other.empty()) {
      return 0;
    }

    if (is_negative && !other.is_negative) {
      return -1;
    }
    if (!is_negative && other.is_negative) {
      return 1;
    }

    int cmp = cmp_absolute(other);
    if (is_negative) {
      return -cmp;
    } else {
      return cmp;
    }
  }
  int cmp_absolute(const CBigInt &other) const {
    if (size() < other.size()) {
      return -1;
    }
    if (size() > other.size()) {
      return 1;
    }

    for (int i = size() - 1; i >= 0; i--) {
      Digit a = this->digits[i];
      Digit b = other.digits[i];
      if (a < b) {
        return -1;
      }
      if (a > b) {
        return 1;
      }
    }

    return 0;
  }
  Digit div_rem(Digit divisor) {
    Digit rem = div_rem_digit(digits, divisor);
    normalize();
    return rem;
  }
  void operator+=(const CBigInt &other) {
    OwnedDigits &a = this->digits;
    const OwnedDigits &b = other.digits;

    if (this->is_negative == other.is_negative) {
      size_t b_clamp_len = b.size();

      if (a.size() < b.size()) {
        b_clamp_len = a.size();
        a.insert(a.end(), b.begin() + a.size(), b.end());
      }

      Digit carry = add2(a, ConstDigits(b).clamp_size(b_clamp_len));

      if (carry != 0) {
        a.push_back(carry);
      }
    } else {
      if (a.size() < b.size()) {
        a.resize(b.size(), 0);
      }

      Digit carry = sub2(a, b);

      if (carry != 0) {
        // we've subtracted in the wrong order and underflowed
        twos_complement(a);
        this->is_negative = !this->is_negative;
      }

      normalize();
    }
  }
  void operator+=(signed int other) {
    if (other >= 0) {
      *this += (Digit)other;
    } else {
      Digit abs = (Digit)-other;
      if (is_negative) {
        *this += abs;
      } else {
        *this -= abs;
      }
    }
  }
  void operator+=(Digit other) {
    Digit carry = add_digit(digits, other);
    if (carry != 0) {
      digits.push_back(carry);
    }
  }
  void operator-=(Digit other) {
    Digit carry = sub_digit(digits, other);
    if (carry != 0) {
      twos_complement(digits);
      this->is_negative = !this->is_negative;
    }
  }
  void operator-=(const CBigInt &other) {
    this->is_negative = !this->is_negative;
    *this += other;
    this->is_negative = !this->is_negative;
  }
  void operator-=(signed int other) {
    this->is_negative = !this->is_negative;
    *this += other;
    this->is_negative = !this->is_negative;
  }
  void operator*=(const CBigInt &other) {
    OwnedDigits dst(this->size() + other.size(), 0);
    mac3(dst, this->const_slice(), other.const_slice());

    this->digits = dst;
    this->is_negative = (is_negative == !other.is_negative);
    normalize();
  }
  void operator*=(Digit other) {
    Digit carry = mul_digit(digits, other);
    if (carry != 0) {
      digits.push_back(carry);
    }
  }
  void operator*=(int other) {
    Digit carry = mul_digit(digits, (Digit)std::abs(other));
    if (carry != 0) {
      digits.push_back(carry);
    }
    if (other < 0) {
      this->is_negative = !this->is_negative;
    }
  }
  CBigInt operator*(const CBigInt &other) const {
    CBigInt copy = *this;
    copy *= other;
    return copy;
  }
  CBigInt operator+(const CBigInt &other) const {
    CBigInt copy = *this;
    copy += other;
    return copy;
  }
  CBigInt operator-(const CBigInt &other) const {
    CBigInt copy = *this;
    copy.negate();
    copy += other;
    copy.negate();
    return copy;
  }
  std::string decode_le(Digit chunk_size, Digit digit_base,
                        const char alphabet[]) const {
    std::string decoded{};
    CBigInt copy = *this;
    copy.normalize();

    while (copy.size() > 1) {
      Digit r = copy.div_rem(chunk_size);
      for (Digit i = 1; i < digit_base; i++) {
        Digit quot = r / digit_base;
        Digit rem = r % digit_base;
        decoded.push_back(alphabet[rem]);
        r = quot;
      }
    }

    if (copy.size() > 0) {
      Digit r = copy.digits[0];
      // do not emit tailing zeros
      while (r > 0) {
        Digit quot = r / digit_base;
        Digit rem = r % digit_base;
        decoded.push_back(alphabet[rem]);
        r = quot;
      }
    }
    if (decoded.empty()) {
      decoded.push_back(alphabet[0]);
    } else if (is_negative) {
      decoded.push_back('-');
    }

    return decoded;
  }
  std::string decode_hex_le(const char alphabet[]) const {
    std::string decoded{};
    ConstDigits slice = normalize_slice(digits);

    int i = 0;
    int end = (int)size() - 1;
    for (; i < end; i++) {
      Digit r = slice[i];
      for (int a = 0; a < 8; a++) {
        decoded.push_back(alphabet[r % 16]);
        r /= 16;
      }
    }

    if (i < (int)size()) {
      Digit r = slice[i];
      while (r > 0) {
        decoded.push_back(alphabet[r % 16]);
        r /= 16;
      }
    }

    if (decoded.empty()) {
      decoded.push_back(alphabet[0]);
    } else if (is_negative) {
      decoded.push_back('-');
    }

    return decoded;
  }
  friend std::ostream &operator<<(std::ostream &stream, const CBigInt &big) {
    std::string decoded;
    if (stream.flags() & std::ios::hex) {
      decoded = big.decode_hex_le("0123456789abcdef");
    } else {
      decoded = big.decode_le(1'000'000'000, 10, "0123456789");
    }
    std::reverse(decoded.begin(), decoded.end());

    return stream << decoded;
  }
  friend std::istream &operator>>(std::istream &stream, CBigInt &big) {
    std::string string{};

    while (stream.peek() == ' ') {
      stream.get();
    }

    char c = stream.peek();
    if (c == '+' || c == '-') {
      string.push_back(c);
      stream.get();
    }

    while (true) {
      char c = stream.peek();
      if ('0' <= c && c <= '9') {
        string.push_back(c);
        stream.get();
        continue;
      }
      break;
    }

    try {
      big = CBigInt(string);
    } catch (const std::invalid_argument &e) {
      stream.setstate(std::ios::failbit);
    }

    return stream;
  }
  // clang-format off

  friend CBigInt operator+(const char *a, const CBigInt &b) { return CBigInt(a) + b; }
  friend CBigInt operator+(const CBigInt &a, const char *b) { return a + CBigInt(b); }
  friend CBigInt operator+(int a, const CBigInt &b) { return CBigInt(a) + b; }
  friend CBigInt operator+(const CBigInt &a, int b) { return a + CBigInt(b); }
  friend CBigInt operator+(std::string &a, const CBigInt &b) { return CBigInt(a) + b; }
  friend CBigInt operator+(const CBigInt &a, std::string &b) { return a + CBigInt(b); }

  friend CBigInt operator*(const char *a, const CBigInt &b) { return CBigInt(a) * b; }
  friend CBigInt operator*(const CBigInt &a, const char *b) { return a * CBigInt(b); }
  friend CBigInt operator*(int a, const CBigInt &b) { return CBigInt(a) * b; }
  friend CBigInt operator*(const CBigInt &a, int b) { return a * CBigInt(b); }
  friend CBigInt operator*(std::string &a, const CBigInt &b) { return CBigInt(a) * b; }
  friend CBigInt operator*(const CBigInt &a, std::string &b) { return a * CBigInt(b); }

  CBigInt &operator=(const char* a) { return *this = CBigInt(a); }
  CBigInt &operator=(const std::string& a) { return *this = CBigInt(a); }
  CBigInt &operator=(int a) { return *this = CBigInt(a); }
  CBigInt &operator=(const CBigInt &a) { 
    this->digits = a.digits;
    this->is_negative = a.is_negative;
    return *this;
  }

  void operator+=(const char *a) { return *this += CBigInt(a); }
  void operator+=(const std::string &a) { return *this += CBigInt(a); }

  void operator-=(const char *a) { return *this += CBigInt(a); }
  void operator-=(const std::string &a) { return *this += CBigInt(a); }

  void operator*=(const char *a) { return *this *= CBigInt(a); }
  void operator*=(const std::string &a) { return *this *= CBigInt(a); }

  bool operator<(const CBigInt &b) { return this->cmp(b) < 0; }
  bool operator>(const CBigInt &b) { return this->cmp(b) > 0; }
  bool operator<=(const CBigInt &b) { return this->cmp(b) <= 0; }
  bool operator>=(const CBigInt &b) { return this->cmp(b) >= 0; }
  bool operator==(const CBigInt &b) { return this->cmp(b) == 0; }
  bool operator!=(const CBigInt &b) { return this->cmp(b) != 0; }

  friend bool operator<(const char *a, const CBigInt &b) { return CBigInt(a).cmp(b) < 0; }
  friend bool operator<(const CBigInt &a, const char *b) { return a.cmp(CBigInt(b)) < 0; }
  friend bool operator<(int a, const CBigInt &b) { return CBigInt(a).cmp(b) < 0; }
  friend bool operator<(const CBigInt &a, int b) { return a.cmp(CBigInt(b)) < 0; }
  friend bool operator<(const std::string &a, const CBigInt &b) { return CBigInt(a).cmp(b) < 0; }
  friend bool operator<(const CBigInt &a, std::string &b) { return a.cmp(CBigInt(b)) < 0; }
  friend bool operator<(const CBigInt &a, const CBigInt &b) { return a.cmp(b) < 0; }

  friend bool operator>(const char *a, const CBigInt &b) { return CBigInt(a).cmp(b) > 0;  }
  friend bool operator>(const CBigInt &a, const char *b) { return a.cmp(CBigInt(b)) > 0;  }
  friend bool operator>(int a, const CBigInt &b) { return CBigInt(a).cmp(b) > 0;  }
  friend bool operator>(const CBigInt &a, int b) { return a.cmp(CBigInt(b)) > 0;  }
  friend bool operator>(const std::string &a, const CBigInt &b) { return CBigInt(a).cmp(b) > 0;  }
  friend bool operator>(const CBigInt &a, std::string &b) { return a.cmp(CBigInt(b)) > 0;  }
  friend bool operator>(const CBigInt &a, const CBigInt &b) { return a.cmp(b) > 0; }

  friend bool operator<=(const char *a, const CBigInt &b) { return CBigInt(a).cmp(b) <= 0; }
  friend bool operator<=(const CBigInt &a, const char *b) { return a.cmp(CBigInt(b)) <= 0; }
  friend bool operator<=(int a, const CBigInt &b) { return CBigInt(a).cmp(b) <= 0; }
  friend bool operator<=(const CBigInt &a, int b) { return a.cmp(CBigInt(b)) <= 0; }
  friend bool operator<=(const std::string &a, const CBigInt &b) { return CBigInt(a).cmp(b) <= 0; }
  friend bool operator<=(const CBigInt &a, std::string &b) { return a.cmp(CBigInt(b)) <= 0; }
  friend bool operator<=(const CBigInt &a, const CBigInt &b) { return a.cmp(b) <= 0; }

  friend bool operator>=(const char *a, const CBigInt &b) { return CBigInt(a).cmp(b) >= 0; }
  friend bool operator>=(const CBigInt &a, const char *b) { return a.cmp(CBigInt(b)) >= 0; }
  friend bool operator>=(int a, const CBigInt &b) { return CBigInt(a).cmp(b) >= 0; }
  friend bool operator>=(const CBigInt &a, int b) { return a.cmp(CBigInt(b)) >= 0; }
  friend bool operator>=(const std::string &a, const CBigInt &b) { return CBigInt(a).cmp(b) >= 0; }
  friend bool operator>=(const CBigInt &a, std::string &b) { return a.cmp(CBigInt(b)) >= 0; }
  friend bool operator>=(const CBigInt &a, const CBigInt &b) { return a.cmp(b) >= 0; }

  friend bool operator==(const char *a, const CBigInt &b) { return CBigInt(a).cmp(b) == 0; }
  friend bool operator==(const CBigInt &a, const char *b) { return a.cmp(CBigInt(b)) == 0; }
  friend bool operator==(int a, const CBigInt &b) { return CBigInt(a).cmp(b) == 0; }
  friend bool operator==(const CBigInt &a, int b) { return a.cmp(CBigInt(b)) == 0; }
  friend bool operator==(const std::string &a, const CBigInt &b) { return CBigInt(a).cmp(b) == 0; }
  friend bool operator==(const CBigInt &a, std::string &b) { return a.cmp(CBigInt(b)) == 0; }
  friend bool operator==(const CBigInt &a, const CBigInt &b) { return a.cmp(b) == 0; }

  friend bool operator!=(const char *a, const CBigInt &b) { return CBigInt(a).cmp(b) != 0; }
  friend bool operator!=(const CBigInt &a, const char *b) { return a.cmp(CBigInt(b)) != 0; }
  friend bool operator!=(int a, const CBigInt &b) { return CBigInt(a).cmp(b) != 0; }
  friend bool operator!=(const CBigInt &a, int b) { return a.cmp(CBigInt(b)) != 0; }
  friend bool operator!=(const std::string &a, const CBigInt &b) { return CBigInt(a).cmp(b) != 0; }
  friend bool operator!=(const CBigInt &a, std::string &b) { return a.cmp(CBigInt(b)) != 0; }
  friend bool operator!=(const CBigInt &a, const CBigInt &b) { return a.cmp(b) != 0; }

  // clang-format on

  // default constructor
  // copying/assignment/destruction
  // int constructor
  // string constructor
  // operator +, any combination {CBigInt/int/string} + {CBigInt/int/string}
  // operator *, any combination {CBigInt/int/string} * {CBigInt/int/string}
  // operator +=, any of {CBigInt/int/string}
  // operator *=, any of {CBigInt/int/string}
  // comparison operators, any combination {CBigInt/int/string}
  // {<,<=,>,>=,==,!=} {CBigInt/int/string} output operator << input operator >>
private:
  // todo
};

// acc += b * c
void mac3(Digits out, ConstDigits x, ConstDigits y) {
  x = normalize_slice(x);
  y = normalize_slice(y);
  out = out.clamp_size(x.size() + y.size());

  if (x.size() > y.size()) {
    std::swap(x, y);
  }

  if (x.empty()) {
    return;
  }

  if (x.size() <= 32) {
    size_t x_size = x.size();
    for (size_t i = 0; i < x_size; i++) {
      Digit overflow = mac_digit(out.start_at(i), y, x[i]);
      assert(overflow == 0);
    }
  } else {
    // Karatsuba multiplication
    // https://en.wikipedia.org/wiki/Karatsuba_algorithm#Basic_step

    size_t b = x.size() / 2;

    auto x_split = x.split_at(b);
    ConstDigits x0 = x_split.first;
    ConstDigits x1 = x_split.second;

    auto y_split = y.split_at(b);
    ConstDigits y0 = y_split.first;
    ConstDigits y1 = y_split.second;

    CBigInt z3{}, z2{}, z0{};

    // z3 = (x1 + x0) * (y1 + y0)
    {
      z0.from_slice(x1);
      z2.from_slice(x0);
      z0 += z2;

      z2.from_slice(y1);
      z3.from_slice(y0);
      z2 += z3;

      z3.make_zeros(z0.size() + z2.size());
      mac3(z3.slice(), z0.slice(), z2.slice());
      z3.normalize();
    }

    // z2 = x1 * y1
    z2.make_zeros(x1.size() + y1.size());
    mac3(z2.slice(), x1, y1);

    // z0 = x0 * y0
    z0.make_zeros(x0.size() + y0.size());
    mac3(z0.slice(), x0, y0);

    // x*y = z0
    //     + (z2 << 2b)
    //     + (z1 << b)

    Digit carry = 0;

    carry |= add2(out, z0.slice());
    carry |= add2(out.start_at(2 * b), z2.slice());
    // z1 = z3 - z2 - z0
    carry |= add2(out.start_at(b), z3.slice());
    carry |= sub2(out.start_at(b), z2.slice());
    carry |= sub2(out.start_at(b), z0.slice());

    assert(carry == 0);
  }
}

#ifndef __PROGTEST__
static bool equal(const CBigInt &x, const char val[]) {
  std::ostringstream oss;
  oss << x;
  if (oss.str() != val) {
    std::cout << "expected: " << val << "\ngot:      " << oss.str() << std::endl
              << std::hex;
    return false;
  }
  return true;
}
static bool equalHex(const CBigInt &x, const char val[]) {
  std::ostringstream oss;
  oss << std::hex << x;
  if (oss.str() != val) {
    std::cout << "expected: " << val << "\ngot:      " << oss.str() << std::endl
              << std::hex;
    return false;
  }
  return true;
}

// #include "fuzz.cpp"

int main() {
  // fuzz();

  CBigInt a, b;
  a = "115";
  assert(equalHex(a, "73"));

  a = "15818899999165008224";
  b = "9203627109237";
  assert(equal(a * b, "145591256870624226354416451365088"));

  a = "8589934592";
  b = "8589934595";
  assert(equal(a * b, "73786976320608010240"));

  a = "8589934592";
  b = "8589935246";
  assert(equal(a * b, "73786981912655429632"));

  a = "18446744073709551616";
  b = "18446744073709551616";
  assert(equal(a * b, "340282366920938463463374607431768211456"));

  a = "10329099484295688410";
  b = "242523800600752046";
  assert(equal(a * b, "2505052463714658325337337681285986860"));

  a = "354108774791";
  b = "9720934714462407409";
  assert(equal(a * b, "3442268281561582515829270826519"));

  a = "4682204987";
  b = "3820358949226912862";
  assert(equal(a * b, "17887703724200331197070842794"));

  a = 10;
  b = -20;
  a += b;
  assert(equal(a, "-10"));

  std::istringstream is;
  a = 10;
  a += 20;
  assert(equal(a, "30"));
  a *= 5;
  assert(equal(a, "150"));
  b = a + 3;
  assert(equal(b, "153"));
  b = a * 7;
  assert(equal(b, "1050"));
  assert(equal(a, "150"));
  assert(equalHex(a, "96"));

  a = "4294967295";
  b = "4294967295";
  assert(equal(a * b, "18446744065119617025"));

  a = "4294967296";
  b = "1";
  assert(equal(a * b, "4294967296"));

  a = "4294967296";
  b = "4294967295";
  assert(equal(a * b, "18446744069414584320"));

  a = "4294967296";
  b = "4294967296";
  assert(equal(a * b, "18446744073709551616"));

  a = 10;
  a += -20;
  assert(equal(a, "-10"));
  a *= 5;
  assert(equal(a, "-50"));
  b = a + 73;
  assert(equal(b, "23"));
  b = a * -7;
  assert(equal(b, "350"));
  assert(equal(a, "-50"));
  assert(equalHex(a, "-32"));

  a = "-99999999999999999999";
  assert(equal(a, "-99999999999999999999"));

  a = "12345678901234567890";
  assert(equal(a, "12345678901234567890"));

  a += "-99999999999999999999";
  assert(equal(a, "-87654321098765432109"));
  a *= "54321987654321987654";
  assert(equal(a, "-4761556948575111126880627366067073182286"));
  a *= 0;
  assert(equal(a, "0"));
  a = 10;
  b = a + "400";
  assert(equal(b, "410"));
  b = a * "15";
  assert(equal(b, "150"));
  assert(equal(a, "10"));
  assert(equalHex(a, "a"));

  is.clear();
  is.str(" 1234");
  assert(is >> b);
  assert(equal(b, "1234"));
  is.clear();
  is.str(" 12 34");
  assert(is >> b);
  assert(equal(b, "12"));
  is.clear();
  is.str("999z");
  assert(is >> b);
  assert(equal(b, "999"));
  is.clear();
  is.str("abcd");
  assert(!(is >> b));
  is.clear();
  is.str("- 758");
  assert(!(is >> b));
  a = 42;
  try {
    a = "-xyz";
    assert("missing an exception" == nullptr);
  } catch (const std::invalid_argument &e) {
    assert(equal(a, "42"));
  }

  a = "73786976294838206464";
  assert(equal(a, "73786976294838206464"));
  assert(equalHex(a, "40000000000000000"));
  assert(a < "1361129467683753853853498429727072845824");
  assert(a <= "1361129467683753853853498429727072845824");
  assert(!(a > "1361129467683753853853498429727072845824"));
  assert(!(a >= "1361129467683753853853498429727072845824"));
  assert(!(a == "1361129467683753853853498429727072845824"));
  assert(a != "1361129467683753853853498429727072845824");
  assert(!(a < "73786976294838206464"));
  assert(a <= "73786976294838206464");
  assert(!(a > "73786976294838206464"));
  assert(a >= "73786976294838206464");
  assert(a == "73786976294838206464");
  assert(!(a != "73786976294838206464"));
  assert(a < "73786976294838206465");
  assert(a <= "73786976294838206465");
  assert(!(a > "73786976294838206465"));
  assert(!(a >= "73786976294838206465"));
  assert(!(a == "73786976294838206465"));
  assert(a != "73786976294838206465");
  a = "2147483648";
  assert(!(a < -2147483648));
  assert(!(a <= -2147483648));
  assert(a > -2147483648);
  assert(a >= -2147483648);
  assert(!(a == -2147483648));
  assert(a != -2147483648);
  a = "-12345678";
  assert(!(a < -87654321));
  assert(!(a <= -87654321));
  assert(a > -87654321);
  assert(a >= -87654321);
  assert(!(a == -87654321));
  assert(a != -87654321);

  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
