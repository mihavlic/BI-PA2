#ifndef __PROGTEST__
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <list>
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
BinarySearch binary_search_by(const T *self, size_t len,
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

struct IndexRange {
  size_t start;
  size_t end;

  bool empty() const { return start >= end; }
  size_t len() const {
    if (empty()) {
      return 0;
    }
    return end - start;
  }
};

template <typename T> struct Slice {
  const T *start;
  const T *end;

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
  BinarySearch find(const T &value) const {
    return binary_search_by<T>(m_data.data(), m_data.size(),
                               [&value, this](const T &element) -> int {
                                 return order(element, value);
                               });
  }
  T *find_ptr(const T &value) {
    BinarySearch entry = find(value);
    if (entry.found) {
      return begin() + entry.index;
    } else {
      return nullptr;
    }
  }
  void insert_entry(BinarySearch position, T value) {
    if (!position.found) {
      m_data.insert(m_data.begin() + position.index, value);
    }
  }
  bool insert(T value) {
    BinarySearch position = find(value);

    insert_entry(position, value);

    if (position.found) {
      return false;
    } else {
      return true;
    }
  }
  bool remove(T value, T *out) {
    BinarySearch position = find(value);

    if (position.found) {
      if (out) {
        *out = std::move(m_data[position.index]);
      }
      m_data.erase(m_data.begin() + position.index);
      return true;
    } else {
      return false;
    }
  }
  template <typename E>
  IndexRange find_slice_by_key(const E &key,
                               const E &(*extract)(const T &)) const {
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
          if (!(element_key > key)) {
            return -1;
          } else {
            return 1;
          }
        });

    return IndexRange{start.index, end.index};
  }
  void debug() const {
    for (const T &element : m_data) {
      std::cout << element << ' ';
    }
    std::cout << std::endl;
  }
  Slice<T> slice(IndexRange range) const {
    return Slice<T>{begin() + range.start, begin() + range.end};
  }
  T &operator[](size_t index) { return m_data[index]; }
  const T &operator[](size_t index) const { return m_data[index]; }
  T *data() { return m_data.data(); }
  const T *data() const { return m_data.data(); }
  size_t size() const { return m_data.size(); }
  T *begin() { return m_data.data(); }
  const T *begin() const { return m_data.data(); }
  T *end() { return m_data.data() + m_data.size(); }
  const T *end() const { return m_data.data() + m_data.size(); }
};

struct CaseInsensitive {
  std::string inner;
  bool operator<(const CaseInsensitive &rhs) const {
    return std::lexicographical_compare(
        std::begin(inner), std::end(inner), std::begin(rhs.inner),
        std::end(rhs.inner), [](const char &char1, const char &char2) {
          return std::tolower(char1) < std::tolower(char2);
        });
  }
  bool operator>(const CaseInsensitive &rhs) const { return rhs < *this; }
};

struct LandEntry {
  CaseInsensitive owner;
  unsigned counter;
  unsigned id;
  std::string city;
  std::string addr;
  std::string region;
};

class CIterator {
  Slice<const LandEntry *> inner;

public:
  CIterator(Slice<const LandEntry *> slice) : inner(slice) {}
  CIterator(LandEntry *const *start, LandEntry *const *end)
      : inner(Slice<const LandEntry *>{start, end}) {}
  bool atEnd() const { return inner.empty(); }
  void next() { inner.next(); }
  std::string city() const { return (*inner.start)->city; }
  std::string addr() const { return (*inner.start)->addr; }
  std::string region() const { return (*inner.start)->region; }
  unsigned id() const { return (*inner.start)->id; }
  std::string owner() const { return (*inner.start)->owner.inner; }
  size_t len() const { return inner.len(); }
};

template <typename A, typename B>
int cmp_two(const A &a1, const B &a2, const A &b1, const B &b2) {
  if (a1 < b1) {
    return -1;
  } else if (a1 > b1) {
    return 1;
  }
  if (a2 < b2) {
    return -1;
  } else if (a2 > b2) {
    return 1;
  }
  return 0;
}

using GoodParsingRules = LandEntry *;

int cmp_entry_region_id(const GoodParsingRules &a, const GoodParsingRules &b) {
  return cmp_two<std::string, unsigned>(a->region, a->id, b->region, b->id);
}

int cmp_entry_city_addr(const GoodParsingRules &a, const GoodParsingRules &b) {
  return cmp_two<std::string, std::string>(a->city, a->addr, b->city, b->addr);
}

int cmp_entry_owner_counter(const GoodParsingRules &a,
                            const GoodParsingRules &b) {
  return cmp_two<CaseInsensitive, unsigned>(a->owner, a->counter, b->owner,
                                            b->counter);
}

class CLandRegister {
  unsigned counter;
  TrashSet<LandEntry *> region_id{cmp_entry_region_id};
  TrashSet<LandEntry *> city_addr{cmp_entry_city_addr};
  TrashSet<LandEntry *> owner_counter{cmp_entry_owner_counter};

public:
  ~CLandRegister() {
    for (LandEntry *i : region_id) {
      delete i;
    }
  }
  bool add(const std::string &city, const std::string &addr,
           const std::string &region, unsigned int id) {
    LandEntry *entry = new LandEntry{
        "", counter, id, city, addr, region,
    };

    BinarySearch a = region_id.find(entry);
    BinarySearch b = city_addr.find(entry);

    if (a.found || b.found) {
      delete entry;
      return false;
    }

    region_id.insert_entry(a, entry);
    city_addr.insert_entry(b, entry);
    owner_counter.insert(entry);

    counter++;

    return true;
  }

  bool del(const std::string &city, const std::string &addr) {
    LandEntry tmp{
        "", 0, 0, city, addr, "",
    };
    LandEntry *out = nullptr;

    if (city_addr.remove(&tmp, &out)) {
      region_id.remove(out, nullptr);
      owner_counter.remove(out, nullptr);

      delete out;
      return true;
    } else {
      return false;
    }
  }

  bool del(const std::string &region, unsigned int id) {
    LandEntry tmp{
        "", 0, id, "", "", region,
    };
    LandEntry *out = nullptr;

    if (region_id.remove(&tmp, &out)) {
      city_addr.remove(out, nullptr);
      owner_counter.remove(out, nullptr);

      delete out;
      return true;
    } else {
      return false;
    }
  }

  bool getOwner(const std::string &city, const std::string &addr,
                std::string &owner) const {
    LandEntry tmp{
        "", 0, 0, city, addr, "",
    };
    BinarySearch entry = city_addr.find(&tmp);

    if (entry.found) {
      owner = city_addr[entry.index]->owner.inner;
      return true;
    } else {
      return false;
    }
  }

  bool getOwner(const std::string &region, unsigned int id,
                std::string &owner) const {
    LandEntry tmp{
        "", 0, id, "", "", region,
    };
    BinarySearch entry = region_id.find(&tmp);

    if (entry.found) {
      owner = region_id[entry.index]->owner.inner;
      return true;
    } else {
      return false;
    }
  }

  bool setOwner(LandEntry *entry, const std::string &owner) {
    if (entry->owner.inner == owner) {
      return false;
    }

    owner_counter.remove(entry, nullptr);
    entry->owner.inner = owner;
    entry->counter = counter++;
    owner_counter.insert(entry);

    return true;
  }

  bool newOwner(const std::string &city, const std::string &addr,
                const std::string &owner) {
    LandEntry tmp{
        "", 0, 0, city, addr, "",
    };
    LandEntry **entry = city_addr.find_ptr(&tmp);
    if (!entry) {
      return false;
    }
    return setOwner(*entry, owner);
  }

  bool newOwner(const std::string &region, unsigned int id,
                const std::string &owner) {
    LandEntry tmp{
        "", 0, id, "", "", region,
    };

    LandEntry **entry = region_id.find_ptr(&tmp);
    if (!entry) {
      return false;
    }
    return setOwner(*entry, owner);
  }

  size_t count(const std::string &owner) const {
    return listByOwner(owner).len();
  }

  CIterator listByAddr() const {
    return CIterator(city_addr.begin(), city_addr.end());
  }

  CIterator listByOwner(const std::string &owner) const {
    CaseInsensitive a{owner};
    IndexRange range = owner_counter.find_slice_by_key<CaseInsensitive>(
        a, [](LandEntry *const &t) -> const CaseInsensitive & {
          return t->owner;
        });

    return CIterator(owner_counter.begin() + range.start,
                     owner_counter.begin() + range.end);
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
  assert(!i0.atEnd());
  assert(i0.city() == "Liberec");
  assert(i0.addr() == "Evropska");
  assert(i0.region() == "Librec");
  assert(i0.id() == 4552);
  assert(i0.owner() == "");
  i0.next();
  assert(!i0.atEnd());
  assert(i0.city() == "Plzen");
  assert(i0.addr() == "Evropska");
  assert(i0.region() == "Plzen mesto");
  assert(i0.id() == 78901);
  assert(i0.owner() == "");
  i0.next();
  assert(!i0.atEnd());
  assert(i0.city() == "Prague");
  assert(i0.addr() == "Evropska");
  assert(i0.region() == "Vokovice");
  assert(i0.id() == 12345);
  assert(i0.owner() == "");
  i0.next();
  assert(!i0.atEnd());
  assert(i0.city() == "Prague");
  assert(i0.addr() == "Technicka");
  assert(i0.region() == "Dejvice");
  assert(i0.id() == 9873);
  assert(i0.owner() == "");
  i0.next();
  assert(!i0.atEnd());
  assert(i0.city() == "Prague");
  assert(i0.addr() == "Thakurova");
  assert(i0.region() == "Dejvice");
  assert(i0.id() == 12345);
  assert(i0.owner() == "");
  i0.next();
  assert(i0.atEnd());

  assert(x.count("") == 5);
  CIterator i1 = x.listByOwner("");
  assert(!i1.atEnd());
  assert(i1.city() == "Prague");
  assert(i1.addr() == "Thakurova");
  assert(i1.region() == "Dejvice");
  assert(i1.id() == 12345);
  assert(i1.owner() == "");
  i1.next();
  assert(!i1.atEnd());
  assert(i1.city() == "Prague");
  assert(i1.addr() == "Evropska");
  assert(i1.region() == "Vokovice");
  assert(i1.id() == 12345);
  assert(i1.owner() == "");
  i1.next();
  assert(!i1.atEnd());
  assert(i1.city() == "Prague");
  assert(i1.addr() == "Technicka");
  assert(i1.region() == "Dejvice");
  assert(i1.id() == 9873);
  assert(i1.owner() == "");
  i1.next();
  assert(!i1.atEnd());
  assert(i1.city() == "Plzen");
  assert(i1.addr() == "Evropska");
  assert(i1.region() == "Plzen mesto");
  assert(i1.id() == 78901);
  assert(i1.owner() == "");
  i1.next();
  assert(!i1.atEnd());
  assert(i1.city() == "Liberec");
  assert(i1.addr() == "Evropska");
  assert(i1.region() == "Librec");
  assert(i1.id() == 4552);
  assert(i1.owner() == "");
  i1.next();
  assert(i1.atEnd());

  assert(x.count("CVUT") == 0);
  CIterator i2 = x.listByOwner("CVUT");
  assert(i2.atEnd());

  assert(x.newOwner("Prague", "Thakurova", "CVUT"));
  assert(x.newOwner("Dejvice", 9873, "CVUT"));
  assert(x.newOwner("Plzen", "Evropska", "Anton Hrabis"));
  assert(x.newOwner("Librec", 4552, "Cvut"));
  assert(x.getOwner("Prague", "Thakurova", owner));
  assert(owner == "CVUT");
  assert(x.getOwner("Dejvice", 12345, owner));
  assert(owner == "CVUT");
  assert(x.getOwner("Prague", "Evropska", owner));
  assert(owner == "");
  assert(x.getOwner("Vokovice", 12345, owner));
  assert(owner == "");
  assert(x.getOwner("Prague", "Technicka", owner));
  assert(owner == "CVUT");
  assert(x.getOwner("Dejvice", 9873, owner));
  assert(owner == "CVUT");
  assert(x.getOwner("Plzen", "Evropska", owner));
  assert(owner == "Anton Hrabis");
  assert(x.getOwner("Plzen mesto", 78901, owner));
  assert(owner == "Anton Hrabis");
  assert(x.getOwner("Liberec", "Evropska", owner));
  assert(owner == "Cvut");
  assert(x.getOwner("Librec", 4552, owner));
  assert(owner == "Cvut");
  CIterator i3 = x.listByAddr();
  assert(!i3.atEnd());
  assert(i3.city() == "Liberec");
  assert(i3.addr() == "Evropska");
  assert(i3.region() == "Librec");
  assert(i3.id() == 4552);
  assert(i3.owner() == "Cvut");
  i3.next();
  assert(!i3.atEnd());
  assert(i3.city() == "Plzen");
  assert(i3.addr() == "Evropska");
  assert(i3.region() == "Plzen mesto");
  assert(i3.id() == 78901);
  assert(i3.owner() == "Anton Hrabis");
  i3.next();
  assert(!i3.atEnd());
  assert(i3.city() == "Prague");
  assert(i3.addr() == "Evropska");
  assert(i3.region() == "Vokovice");
  assert(i3.id() == 12345);
  assert(i3.owner() == "");
  i3.next();
  assert(!i3.atEnd());
  assert(i3.city() == "Prague");
  assert(i3.addr() == "Technicka");
  assert(i3.region() == "Dejvice");
  assert(i3.id() == 9873);
  assert(i3.owner() == "CVUT");
  i3.next();
  assert(!i3.atEnd());
  assert(i3.city() == "Prague");
  assert(i3.addr() == "Thakurova");
  assert(i3.region() == "Dejvice");
  assert(i3.id() == 12345);
  assert(i3.owner() == "CVUT");
  i3.next();
  assert(i3.atEnd());

  assert(x.count("cvut") == 3);
  CIterator i4 = x.listByOwner("cVuT");
  assert(!i4.atEnd());
  assert(i4.city() == "Prague");
  assert(i4.addr() == "Thakurova");
  assert(i4.region() == "Dejvice");
  assert(i4.id() == 12345);
  assert(i4.owner() == "CVUT");
  i4.next();
  assert(!i4.atEnd());
  assert(i4.city() == "Prague");
  assert(i4.addr() == "Technicka");
  assert(i4.region() == "Dejvice");
  assert(i4.id() == 9873);
  assert(i4.owner() == "CVUT");
  i4.next();
  assert(!i4.atEnd());
  assert(i4.city() == "Liberec");
  assert(i4.addr() == "Evropska");
  assert(i4.region() == "Librec");
  assert(i4.id() == 4552 && i4.owner() == "Cvut");
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
  assert(!i0.atEnd());
  assert(i0.city() == "Prague");
  assert(i0.addr() == "Evropska");
  assert(i0.region() == "Vokovice");
  assert(i0.id() == 12345);
  assert(i0.owner() == "");
  i0.next();
  assert(!i0.atEnd());
  assert(i0.city() == "Prague");
  assert(i0.addr() == "Technicka");
  assert(i0.region() == "Dejvice");
  assert(i0.id() == 9873);
  assert(i0.owner() == "");
  i0.next();
  assert(!i0.atEnd());
  assert(i0.city() == "Prague");
  assert(i0.addr() == "Thakurova");
  assert(i0.region() == "Dejvice");
  assert(i0.id() == 12345);
  assert(i0.owner() == "");
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
  assert(!i1.atEnd());
  assert(i1.city() == "Prague");
  assert(i1.addr() == "Thakurova");
  assert(i1.region() == "Dejvice");
  assert(i1.id() == 12345 && i1.owner() == "CVUT");
  i1.next();
  assert(i1.atEnd());

  assert(!x.del("Brno", "Technicka"));
  assert(!x.del("Karlin", 9873));
  assert(x.del("Prague", "Technicka"));
  assert(!x.del("Prague", "Technicka"));
  assert(!x.del("Dejvice", 9873));
}

template <typename A, typename B> struct Tuple {
  A x;
  B y;

  friend std::ostream &operator<<(std::ostream &os, const Tuple &t) {
    return os << '(' << t.x << ' ' << t.y << ')';
  }
  static int cmp(const Tuple<A, B> &a, const Tuple<A, B> &b) {
    return cmp_two<A, B>(a.x, a.y, b.x, b.y);
  }
};

using IntTuple = Tuple<int, int>;

int main(void) {
  TrashSet<IntTuple> set(IntTuple::cmp);

  set.insert(IntTuple{1, 2});
  set.insert(IntTuple{5, 3});
  set.insert(IntTuple{-9, 5});
  set.insert(IntTuple{8, 1});
  set.insert(IntTuple{5, 1});
  set.insert(IntTuple{1, -5});
  set.insert(IntTuple{-1, 3});
  set.insert(IntTuple{1, 3});

  set.debug();

  int a = 5;
  IndexRange range = set.find_slice_by_key<int>(
      a, [](const IntTuple &t) -> const int & { return t.x; });
  set.slice(range).debug();

  test0();
  test1();
  return 0;
}

#endif /* __PROGTEST__ */
