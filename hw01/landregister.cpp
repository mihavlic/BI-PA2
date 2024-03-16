#ifndef __PROGTEST__
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <vector>
#endif /* __PROGTEST__ */

struct BinarySearch {
  size_t index;
  bool found;
};

// essentially std::lower_bound, stolen from
// https://doc.rust-lang.org/src/core/slice/mod.rs.html#2825-2827
template <typename T>
BinarySearch binary_search_by(T *self, size_t len,
                              std::function<int(const T &)> order) {
  size_t size = len;
  size_t left = 0;
  size_t right = size;

  while (left < right) {
    size_t mid = left + size / 2;
    int cmp = order(self[mid]);

    if (cmp < 0) {
      left = mid + 1;
    } else if (cmp > 0) {
      right = mid;
    } else {
      return BinarySearch{mid, true};
    }

    size = right - left;
  }

  return BinarySearch{left, false};
}

template <typename T> struct Slice {
  T *start;
  T *end;

  bool empty() const { return start >= end; }
  size_t len() const {
    if (empty()) {
      return 0;
    }
    return end - start;
  }
  void next() { start++; }
  void debug() {
    while (!empty()) {
      std::cout << *start << ' ';
      next();
    }
    std::cout << std::endl;
  }
};

template <typename T> using EntryOrdering = int (*)(const T &, const T &);

template <typename T> class TrashSet {
  std::vector<T> m_data;
  EntryOrdering<T> order;

public:
  TrashSet(EntryOrdering<T> order_) : order(order_) {}
  TrashSet()
      : order([](const T &a, const T &b) {
          if (a < b) {
            return -1;
          } else if (a > b) {
            return 1;
          } else {
            return 0;
          }
        }) {}
  BinarySearch find(const T &value) {
    return binary_search_by<T>(m_data.data(), m_data.size(),
                               [&value, this](const T &element) -> int {
                                 return order(element, value);
                               });
  }
  bool insert(T value) {
    BinarySearch position = find(value);

    if (position.found) {
      m_data[position.index] = value;
      return false;
    } else {
      m_data.insert(m_data.begin() + position.index, value);
      return true;
    }
  }
  bool remove(T value) {
    BinarySearch position = find(value, order);

    if (position.found) {
      m_data.erase(m_data.begin() + position.index);
      return true;
    } else {
      return false;
    }
  }
  template <typename E>
  Slice<T> find_slice_by_key(const E &key, const E &(*extract)(const T &)) {
    BinarySearch start = binary_search_by<T>(
        m_data.data(), m_data.size(), [extract, &key](const T &element) -> int {
          const E &element_key = extract(element);
          if (element_key < key) {
            return -1;
          } else {
            return 1;
          }
        });
    BinarySearch end = binary_search_by<T>(
        m_data.data(), m_data.size(), [extract, &key](const T &element) -> int {
          const E &element_key = extract(element);
          if (element_key <= key) {
            return -1;
          } else {
            return 1;
          }
        });

    T *ptr = m_data.data();
    return Slice<T>{ptr + start.index, ptr + end.index};
  }
  void print() const {
    for (const T &element : m_data) {
      std::cout << element << ' ';
    }
    std::cout << std::endl;
  }
};

struct LandEntry {
  std::string owner;
  std::string city;
  std::string addr;
  std::string region;
  unsigned int id;
};

class CIterator {
  Slice<LandEntry> inner;

public:
  CIterator(LandEntry *start, LandEntry *end)
      : inner(Slice<LandEntry>{start, end}) {}
  bool atEnd() const { return inner.empty(); }
  void next() { inner.next(); }
  std::string city() const { return inner.start->city; }
  std::string addr() const { return inner.start->addr; }
  std::string region() const { return inner.start->region; }
  unsigned id() const { return inner.start->id; }
  std::string owner() const { return inner.start->owner; }

private:
  // todo
};

class CLandRegister {
  std::vector<LandEntry> lands;

public:
  bool add(const std::string &city, const std::string &addr,
           const std::string &region, unsigned int id) {
    return 1;
  }

  bool del(const std::string &city, const std::string &addr) { return 1; }

  bool del(const std::string &region, unsigned int id) { return 1; }

  bool getOwner(const std::string &city, const std::string &addr,
                std::string &owner) const {
    return 1;
  }

  bool getOwner(const std::string &region, unsigned int id,
                std::string &owner) const {
    return 1;
  }

  bool newOwner(const std::string &city, const std::string &addr,
                const std::string &owner) {
    return 1;
  }

  bool newOwner(const std::string &region, unsigned int id,
                const std::string &owner) {
    return 1;
  }

  size_t count(const std::string &owner) const { return 1; }

  CIterator listByAddr() const { return CIterator{nullptr, nullptr}; }

  CIterator listByOwner(const std::string &owner) const {
    return CIterator{nullptr, nullptr};
  }
};
#ifndef __PROGTEST__
static void test0() {
  CLandRegister x;
  std::string owner;

  assert(x.add("Prague", "Thakurova", "Dejvice", 12345));
  assert(x.add("Prague", "Evropska", "Vokovice", 12345));
  assert(x.add("Prague", "Technicka", "Dejvice", 9873));
  assert(x.add("Plzen", "Evropska", "Plzen mesto", 78901));
  assert(x.add("Liberec", "Evropska", "Librec", 4552));
  CIterator i0 = x.listByAddr();
  assert(!i0.atEnd() && i0.city() == "Liberec" && i0.addr() == "Evropska" &&
         i0.region() == "Librec" && i0.id() == 4552 && i0.owner() == "");
  i0.next();
  assert(!i0.atEnd() && i0.city() == "Plzen" && i0.addr() == "Evropska" &&
         i0.region() == "Plzen mesto" && i0.id() == 78901 && i0.owner() == "");
  i0.next();
  assert(!i0.atEnd() && i0.city() == "Prague" && i0.addr() == "Evropska" &&
         i0.region() == "Vokovice" && i0.id() == 12345 && i0.owner() == "");
  i0.next();
  assert(!i0.atEnd() && i0.city() == "Prague" && i0.addr() == "Technicka" &&
         i0.region() == "Dejvice" && i0.id() == 9873 && i0.owner() == "");
  i0.next();
  assert(!i0.atEnd() && i0.city() == "Prague" && i0.addr() == "Thakurova" &&
         i0.region() == "Dejvice" && i0.id() == 12345 && i0.owner() == "");
  i0.next();
  assert(i0.atEnd());

  assert(x.count("") == 5);
  CIterator i1 = x.listByOwner("");
  assert(!i1.atEnd() && i1.city() == "Prague" && i1.addr() == "Thakurova" &&
         i1.region() == "Dejvice" && i1.id() == 12345 && i1.owner() == "");
  i1.next();
  assert(!i1.atEnd() && i1.city() == "Prague" && i1.addr() == "Evropska" &&
         i1.region() == "Vokovice" && i1.id() == 12345 && i1.owner() == "");
  i1.next();
  assert(!i1.atEnd() && i1.city() == "Prague" && i1.addr() == "Technicka" &&
         i1.region() == "Dejvice" && i1.id() == 9873 && i1.owner() == "");
  i1.next();
  assert(!i1.atEnd() && i1.city() == "Plzen" && i1.addr() == "Evropska" &&
         i1.region() == "Plzen mesto" && i1.id() == 78901 && i1.owner() == "");
  i1.next();
  assert(!i1.atEnd() && i1.city() == "Liberec" && i1.addr() == "Evropska" &&
         i1.region() == "Librec" && i1.id() == 4552 && i1.owner() == "");
  i1.next();
  assert(i1.atEnd());

  assert(x.count("CVUT") == 0);
  CIterator i2 = x.listByOwner("CVUT");
  assert(i2.atEnd());

  assert(x.newOwner("Prague", "Thakurova", "CVUT"));
  assert(x.newOwner("Dejvice", 9873, "CVUT"));
  assert(x.newOwner("Plzen", "Evropska", "Anton Hrabis"));
  assert(x.newOwner("Librec", 4552, "Cvut"));
  assert(x.getOwner("Prague", "Thakurova", owner) && owner == "CVUT");
  assert(x.getOwner("Dejvice", 12345, owner) && owner == "CVUT");
  assert(x.getOwner("Prague", "Evropska", owner) && owner == "");
  assert(x.getOwner("Vokovice", 12345, owner) && owner == "");
  assert(x.getOwner("Prague", "Technicka", owner) && owner == "CVUT");
  assert(x.getOwner("Dejvice", 9873, owner) && owner == "CVUT");
  assert(x.getOwner("Plzen", "Evropska", owner) && owner == "Anton Hrabis");
  assert(x.getOwner("Plzen mesto", 78901, owner) && owner == "Anton Hrabis");
  assert(x.getOwner("Liberec", "Evropska", owner) && owner == "Cvut");
  assert(x.getOwner("Librec", 4552, owner) && owner == "Cvut");
  CIterator i3 = x.listByAddr();
  assert(!i3.atEnd() && i3.city() == "Liberec" && i3.addr() == "Evropska" &&
         i3.region() == "Librec" && i3.id() == 4552 && i3.owner() == "Cvut");
  i3.next();
  assert(!i3.atEnd() && i3.city() == "Plzen" && i3.addr() == "Evropska" &&
         i3.region() == "Plzen mesto" && i3.id() == 78901 &&
         i3.owner() == "Anton Hrabis");
  i3.next();
  assert(!i3.atEnd() && i3.city() == "Prague" && i3.addr() == "Evropska" &&
         i3.region() == "Vokovice" && i3.id() == 12345 && i3.owner() == "");
  i3.next();
  assert(!i3.atEnd() && i3.city() == "Prague" && i3.addr() == "Technicka" &&
         i3.region() == "Dejvice" && i3.id() == 9873 && i3.owner() == "CVUT");
  i3.next();
  assert(!i3.atEnd() && i3.city() == "Prague" && i3.addr() == "Thakurova" &&
         i3.region() == "Dejvice" && i3.id() == 12345 && i3.owner() == "CVUT");
  i3.next();
  assert(i3.atEnd());

  assert(x.count("cvut") == 3);
  CIterator i4 = x.listByOwner("cVuT");
  assert(!i4.atEnd() && i4.city() == "Prague" && i4.addr() == "Thakurova" &&
         i4.region() == "Dejvice" && i4.id() == 12345 && i4.owner() == "CVUT");
  i4.next();
  assert(!i4.atEnd() && i4.city() == "Prague" && i4.addr() == "Technicka" &&
         i4.region() == "Dejvice" && i4.id() == 9873 && i4.owner() == "CVUT");
  i4.next();
  assert(!i4.atEnd() && i4.city() == "Liberec" && i4.addr() == "Evropska" &&
         i4.region() == "Librec" && i4.id() == 4552 && i4.owner() == "Cvut");
  i4.next();
  assert(i4.atEnd());

  assert(x.newOwner("Plzen mesto", 78901, "CVut"));
  assert(x.count("CVUT") == 4);
  CIterator i5 = x.listByOwner("CVUT");
  assert(!i5.atEnd() && i5.city() == "Prague" && i5.addr() == "Thakurova" &&
         i5.region() == "Dejvice" && i5.id() == 12345 && i5.owner() == "CVUT");
  i5.next();
  assert(!i5.atEnd() && i5.city() == "Prague" && i5.addr() == "Technicka" &&
         i5.region() == "Dejvice" && i5.id() == 9873 && i5.owner() == "CVUT");
  i5.next();
  assert(!i5.atEnd() && i5.city() == "Liberec" && i5.addr() == "Evropska" &&
         i5.region() == "Librec" && i5.id() == 4552 && i5.owner() == "Cvut");
  i5.next();
  assert(!i5.atEnd() && i5.city() == "Plzen" && i5.addr() == "Evropska" &&
         i5.region() == "Plzen mesto" && i5.id() == 78901 &&
         i5.owner() == "CVut");
  i5.next();
  assert(i5.atEnd());

  assert(x.del("Liberec", "Evropska"));
  assert(x.del("Plzen mesto", 78901));
  assert(x.count("cvut") == 2);
  CIterator i6 = x.listByOwner("cVuT");
  assert(!i6.atEnd() && i6.city() == "Prague" && i6.addr() == "Thakurova" &&
         i6.region() == "Dejvice" && i6.id() == 12345 && i6.owner() == "CVUT");
  i6.next();
  assert(!i6.atEnd() && i6.city() == "Prague" && i6.addr() == "Technicka" &&
         i6.region() == "Dejvice" && i6.id() == 9873 && i6.owner() == "CVUT");
  i6.next();
  assert(i6.atEnd());

  assert(x.add("Liberec", "Evropska", "Librec", 4552));
}

static void test1() {
  CLandRegister x;
  std::string owner;

  assert(x.add("Prague", "Thakurova", "Dejvice", 12345));
  assert(x.add("Prague", "Evropska", "Vokovice", 12345));
  assert(x.add("Prague", "Technicka", "Dejvice", 9873));
  assert(!x.add("Prague", "Technicka", "Hradcany", 7344));
  assert(!x.add("Brno", "Bozetechova", "Dejvice", 9873));
  assert(!x.getOwner("Prague", "THAKUROVA", owner));
  assert(!x.getOwner("Hradcany", 7343, owner));
  CIterator i0 = x.listByAddr();
  assert(!i0.atEnd() && i0.city() == "Prague" && i0.addr() == "Evropska" &&
         i0.region() == "Vokovice" && i0.id() == 12345 && i0.owner() == "");
  i0.next();
  assert(!i0.atEnd() && i0.city() == "Prague" && i0.addr() == "Technicka" &&
         i0.region() == "Dejvice" && i0.id() == 9873 && i0.owner() == "");
  i0.next();
  assert(!i0.atEnd() && i0.city() == "Prague" && i0.addr() == "Thakurova" &&
         i0.region() == "Dejvice" && i0.id() == 12345 && i0.owner() == "");
  i0.next();
  assert(i0.atEnd());

  assert(x.newOwner("Prague", "Thakurova", "CVUT"));
  assert(!x.newOwner("Prague", "technicka", "CVUT"));
  assert(!x.newOwner("prague", "Technicka", "CVUT"));
  assert(!x.newOwner("dejvice", 9873, "CVUT"));
  assert(!x.newOwner("Dejvice", 9973, "CVUT"));
  assert(!x.newOwner("Dejvice", 12345, "CVUT"));
  assert(x.count("CVUT") == 1);
  CIterator i1 = x.listByOwner("CVUT");
  assert(!i1.atEnd() && i1.city() == "Prague" && i1.addr() == "Thakurova" &&
         i1.region() == "Dejvice" && i1.id() == 12345 && i1.owner() == "CVUT");
  i1.next();
  assert(i1.atEnd());

  assert(!x.del("Brno", "Technicka"));
  assert(!x.del("Karlin", 9873));
  assert(x.del("Prague", "Technicka"));
  assert(!x.del("Prague", "Technicka"));
  assert(!x.del("Dejvice", 9873));
}

struct Tuple {
  int x;
  int y;

  friend std::ostream &operator<<(std::ostream &os, const Tuple &t) {
    return os << '(' << t.x << ' ' << t.y << ')';
  }
};

int tuple_order(const Tuple &a, const Tuple &b) {
  if (a.x < b.x) {
    return -1;
  } else if (a.x > b.x) {
    return 1;
  }
  if (a.y < b.y) {
    return -1;
  } else if (a.y > b.y) {
    return 1;
  }
  return 0;
}

const int &extract_x(const Tuple &t) { return t.x; }

int main(void) {
  TrashSet<Tuple> set(tuple_order);

  set.insert(Tuple{1, 2});
  set.insert(Tuple{5, 3});
  set.insert(Tuple{-9, 5});
  set.insert(Tuple{8, 1});
  set.insert(Tuple{5, 1});
  set.insert(Tuple{1, -5});
  set.insert(Tuple{-1, 3});
  set.insert(Tuple{1, 3});

  set.print();

  int a = 5;
  IndexRange r = set.find_slice_by_key(a, extract_x);
  printf("%ld..%ld\n", r.start, r.end);
  set.iter(r).debug();

  // test0();
  // test1();
  return 0;
}

#endif /* __PROGTEST__ */
