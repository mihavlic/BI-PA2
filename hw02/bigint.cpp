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

template <typename T> struct Pair {
  T first;
  T second;
};

template <typename T> struct Slice {
  T *start;
  T *stop;

  Slice(std::vector<T> &vector)
      : start(vector.data()), stop(vector.data() + vector.size()) {}
  Slice(T *ptr) : start(ptr), stop(ptr + 1) {}
  Slice(T *start_, T *end_) : start(start_), stop(end_) {}

  bool empty() const { return start >= stop; }
  size_t size() const {
    if (empty()) {
      return 0;
    }
    return stop - start;
  }
  void next() { start += 1; }
  void next_by(size_t offset) { *this = start_at(offset); }
  T *begin() const { return start; }
  T *end() const { return stop; }
  Pair<Slice<T>> split_at(size_t at) {
    assert(at <= size());

    Slice<T> a{start, start + at};
    Slice<T> b{start + at, stop};

    return Pair<Slice<T>>{a, b};
  }
  Slice<T> clamp_size(size_t clamp) {
    size_t len = std::min(size(), clamp);
    return Slice<T>{start, start + len};
  }
  Slice<T> start_at(size_t offset) {
    size_t offset_ = std::min(size(), offset);
    return Slice<T>{start + offset_, stop};
  }
  T &operator[](size_t index) { return start[index]; }
};

template <typename T> struct ConstSlice {
  const T *start;
  const T *stop;

  ConstSlice(const std::vector<T> &vector)
      : start(vector.data()), stop(vector.data() + vector.size()) {}
  ConstSlice(const T *start_, const T *end_) : start(start_), stop(end_) {}
  ConstSlice(const T *ptr) : start(ptr), stop(ptr + 1) {}
  ConstSlice(Slice<T> range) : start(range.start), stop(range.stop) {}

  bool empty() const { return start >= stop; }
  size_t size() const {
    if (empty()) {
      return 0;
    }
    return stop - start;
  }
  void next() {
    if (!empty()) {
      start += 1;
    }
  }
  void next_by(size_t offset) { *this = start_at(offset); }
  const T *begin() const { return start; }
  const T *end() const { return stop; }
  Pair<ConstSlice<T>> split_at(size_t at) {
    assert(at <= size());

    ConstSlice<T> a{start, start + at};
    ConstSlice<T> b{start + at, stop};

    return Pair<ConstSlice<T>>{a, b};
  }
  ConstSlice<T> clamp_size(size_t clamp) {
    size_t len = std::min(size(), clamp);
    return ConstSlice<T>{start, start + len};
  }
  ConstSlice<T> start_at(size_t offset) {
    size_t offset_ = std::min(size(), offset);
    return ConstSlice<T>{start + offset_, stop};
  }
  const T &operator[](size_t index) { return start[index]; }
};

using Digit = uint32_t;
using DoubleDigit = uint64_t;
constexpr Digit DIGIT_BITS = sizeof(Digit) * 8;

using Digits = Slice<Digit>;
using ConstDigits = ConstSlice<Digit>;
using OwnedDigits = std::vector<Digit>;

Digit double_high(DoubleDigit digit) { return (Digit)(digit >> DIGIT_BITS); }
Digit double_low(DoubleDigit digit) { return (Digit)digit; }
DoubleDigit double_pack(Digit high, Digit low) {
  return (DoubleDigit)low + ((DoubleDigit)high << DIGIT_BITS);
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
// return low_bits(x), carry = high_bits(carry) >> low_bits_size
inline Digit mac_carry_u32(Digit a, Digit b, Digit c, Digit *acc) {
  DoubleDigit d = (DoubleDigit)*acc;
  d += (DoubleDigit)a;
  d += (DoubleDigit)b * (DoubleDigit)c;

  Digit hi = double_high(d);
  Digit lo = double_low(d);

  *acc = hi;
  return lo;
}

// x = carry + a * b
// return low_bits(x), carry = high_bits(carry) >> low_bits_size
inline Digit mul_carry_u32(Digit a, Digit b, Digit *acc) {
  DoubleDigit d = (DoubleDigit)*acc;
  d += (DoubleDigit)a * (DoubleDigit)b;

  Digit hi = double_high(d);
  Digit lo = double_low(d);

  *acc = hi;
  return lo;
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
void add2_grow(OwnedDigits &a, ConstDigits b) {
  size_t a_len = a.size();

  Digit carry = 0;
  if (a_len < b.size()) {
    //      232
    // + 165847  second number is longer than first
    //
    //      232
    // +    847  add the common length
    //    1 079
    //    ^ carry out
    //
    //           copy the higher digits to first number and add carry from
    //           previous part
    //  165 ___  (lower digits masked by slice offset)
    // +  1  carry in
    //  166
    //
    //  166 079  final result
    Digit lo_carry = add2(a, b.clamp_size(a_len));
    a.insert(a.end(), b.begin() + a_len, b.end());

    auto dst = Slice(a).start_at(a_len);
    auto src = Slice(&lo_carry);
    carry = add2(dst, src);
  } else {
    carry = add2(a, b);
  };

  if (carry != 0) {
    a.push_back(carry);
  }
}

/// Three argument multiply accumulate:
/// a += b * c
void mac_digit(Digits a, ConstDigits b, Digit c) {
  if (c == 0) {
    return;
  }

  size_t b_len = b.size();

  Digit carry = 0;
  for (size_t i = 0; i < b_len; i++) {
    a[i] = mac_carry_u32(a[i], b[i], c, &carry);
  }

  // carry high bits are always zero after mac_with_carry
  Digit carry_lo = double_low(carry);
  ConstDigits slice(&carry_lo);

  Digit final_carry = add2(a.start_at(b_len), slice);

  assert(final_carry == 0);
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
void sub_digit(Digits a, Digit b) {
  size_t len = a.size();

  Digit carry = b;
  for (size_t i = 0; i < len && carry != 0; i++) {
    a[i] = sub_carry_u32(a[i], 0, &carry);
  }

  assert(carry == 0);
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

Digits normalize_slice(Digits digits) {
  if (digits.empty()) {
    return digits;
  }

  size_t i = digits.size();
  while (i-- && digits[i] == 0) {
  }
  return Digits(digits.begin(), digits.begin() + i + 1);
}

void normalize(OwnedDigits &digits) {
  Digits normalized = normalize_slice(digits);
  digits.erase(digits.begin() + normalized.size(), digits.end());
}

void mac3(OwnedDigits &acc, ConstDigits b, ConstDigits c);

class CBigInt {
  bool is_negative;
  OwnedDigits digits;

public:
  CBigInt() {}
  CBigInt(Digit digit) : digits({digit}) {}
  CBigInt(ConstDigits slice) { from_slice(slice); }
  CBigInt(int digit)
      : is_negative(digit < 0), digits({(Digit)std::abs(digit)}) {}
  CBigInt(long digit)
      : is_negative(digit < 0), digits({(Digit)std::abs(digit)}) {}
  CBigInt(const char *cursor, const char *end) {
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
  }
  CBigInt(const std::string &str)
      : CBigInt(str.data(), str.data() + str.size()) {}
  CBigInt(const char *cstr) : CBigInt(cstr, cstr + strlen(cstr)) {}
  void from_slice(ConstDigits slice) {
    digits.clear();
    digits.insert(digits.begin(), slice.begin(), slice.end());
  }
  void negate() { is_negative = !is_negative; }
  Digits slice() { return Digits(digits); }
  ConstDigits const_slice() const { return ConstDigits(digits); }
  OwnedDigits &vector() { return digits; }
  size_t size() const { return digits.size(); }
  bool is_empty() const { return digits.empty(); }
  void clear() {
    is_negative = false;
    digits.clear();
  }
  void make_zeros(size_t size) {
    digits.clear();
    digits.resize(size, 0);
  }
  void normalize() { ::normalize(digits); }
  int cmp(const CBigInt &other) const {
    if (is_empty() && other.is_empty()) {
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
    if (this->is_empty()) {
      *this = other;
      return;
    }
    if (other.is_empty()) {
      return;
    }

    if (this->is_negative == other.is_negative) {
      add2_grow(this->digits, other.digits);
    } else {
      if (this->size() < other.size()) {
        this->digits.resize(other.size(), 0);
      }
      Digit carry = sub2(this->digits, other.digits);
      if (carry != 0) {
        // we've subtracted in the wrong order and underflowed
        twos_complement(this->digits);
        this->is_negative = !this->is_negative;
      }

      normalize();
    }
  }
  void operator+=(signed int other) {
    if (other >= 0) {
      *this += (Digit)other;
    } else {
      CBigInt aaa(other);
      *this += aaa;
    }
  }
  void operator+=(Digit other) {
    Digit carry = add_digit(digits, other);
    if (carry != 0) {
      digits.push_back(carry);
    }
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
  std::string to_decimal_le() const {
    std::string decoded{};
    CBigInt copy = *this;
    copy.normalize();

    while (copy.size() > 1) {
      Digit r = copy.div_rem(1000'000'000);
      for (int i = 0; i < 9; i++) {
        decoded.push_back('0' + (r % 10));
        r /= 10;
      }
    }

    if (copy.size() > 0) {
      Digit r = copy.digits[0];
      // do not emit tailing zeros
      while (r > 0) {
        decoded.push_back('0' + (r % 10));
        r /= 10;
      }
    }
    if (decoded.empty()) {
      decoded.push_back('0');
    } else if (copy.is_negative) {
      decoded.push_back('-');
    }

    // expected: -4761556948575111126880 6273660670 73182286
    // got:      -4761556948575111126880 627366067  73182286

    return decoded;
  }
  friend std::ostream &operator<<(std::ostream &stream, const CBigInt &big) {
    std::string decoded = big.to_decimal_le();
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
  bool operator<(const CBigInt &other) { return cmp(other) < 0; }
  bool operator>(const CBigInt &other) { return cmp(other) > 0; }
  bool operator<=(const CBigInt &other) { return cmp(other) <= 0; }
  bool operator>=(const CBigInt &other) { return cmp(other) >= 0; }
  bool operator==(const CBigInt &other) { return cmp(other) == 0; }
  bool operator!=(const CBigInt &other) { return cmp(other) != 0; }
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
//
// yoink
// https://github.com/rust-num/num-bigint/blob/e9b204cf5abd91dda241a921444cce4abcc6f885/src/biguint/multiplication.rs#L108
void mac3(OwnedDigits &acc, ConstDigits x, ConstDigits y) {
  if (x.size() > y.size()) {
    std::swap(x, y);
  }

  Slice out(acc);

  // We use three algorithms for different input sizes.
  //
  // - For small inputs, long multiplication is fastest.
  // - Next we use Karatsuba multiplication (Toom-2), which we have optimized
  //   to avoid unnecessary allocations for intermediate values.
  // - For the largest inputs we use Toom-3, which better optimizes the
  //   number of operations, but uses more temporary allocations.
  //
  // The thresholds are somewhat arbitrary, chosen by evaluating the results
  // of `cargo bench --bench bigint multiply`.

  if (out.size() <= 32) {
    size_t x_size = x.size();
    for (size_t i = 0; i < x_size; i++) {
      mac_digit(out.start_at(i), y, x[i]);
    }
  } else {
    size_t half = x.size() / 2;
    auto x_split = x.split_at(half);
    auto x0 = x_split.first;
    auto x1 = x_split.second;

    auto y_split = y.split_at(half);
    auto y0 = y_split.first;
    auto y1 = y_split.second;

    // when multiplying two numbers, we need at most 2n + 1 bits
    // log2(a * b) = log2(a) + log2(b)
    // 1 is added to deal with rounding the fractional number up
    //
    // bits(x1 * y1) >= bits(x0 * y0)
    // because the size of x0 and y0 is either equal or one smaller
    size_t len = x1.size() + y1.size();
    CBigInt p{};

    // stolen karatsuba
    //
    // x * y = ... ->
    //
    // p0 = x0 * y0
    // p1 = (x1 - x0) * (y1 - y0)
    // p2 = x1 * y1
    //
    // x * y = p2 * b^2 + p2 * b
    //       + p0 * b + p0
    //       - p1 * b

    {
      p.make_zeros(len);

      // p2 = x1 * y1
      mac3(p.vector(), x1, y1);
      p.normalize();

      // acc += p2 * b^2 + p2 * b
      add2(out.start_at(half), p.slice());
      add2(out.start_at(half * 2), p.slice());
    }

    {
      p.make_zeros(len);

      // p0 = x0 * y0
      mac3(p.vector(), x0, y0);
      p.normalize();

      // acc += p0 * b + p0
      add2(out, p.slice());
      add2(out.start_at(half), p.slice());
    }

    {
      // q = (x1 - x0)
      // r = (y1 - y0)
      CBigInt q(x1);
      p.from_slice(x0);
      p.negate();
      q += p;

      CBigInt r(y1);
      p.from_slice(y0);
      p.negate();
      q += p;

      // p1 = q * r
      q *= r;

      // -= p1 * b
      sub2(out.start_at(half), p.slice());
    }
  }
}

#ifndef __PROGTEST__
static bool equal(const CBigInt &x, const char val[]) {
  std::ostringstream oss;
  oss << x;
  if (oss.str() != val) {
    std::cout << "expected: " << val << "\ngot:      " << oss.str()
              << std::endl;
    return false;
  }
  return true;
}
static bool equalHex(const CBigInt &x, const char val[]) {
  return true; // hex output is needed for bonus tests only
  // std::ostringstream oss;
  // oss << std::hex << x;
  // return oss . str () == val;
}
int main() {
  CBigInt a, b;

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
