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
    size_t cmp = order(self[mid]);

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

template <typename T>
BinarySearch binary_search_by(std::vector<T> &vector,
                              std::function<int(const T &)> order) {
  return binary_search_by(vector.data(), vector.size(), order);
}

struct IndexRange {
  size_t start;
  size_t end;
};

template <typename T> class TrashSet {
  std::vector<T> m_data;
  int (*m_order)(const T &, const T &);

public:
  TrashSet()
      : m_order([](T &a, T &b) {
          if (a < b) {
            return -1;
          } else if (a > b) {
            return 1;
          } else {
            return 0;
          }
        }) {}
  TrashSet(int (*order)(const T &, const T &)) : m_order(order) {}
  BinarySearch find(T &value) {
    return binary_search_by(m_data, [&value, this](const T &element) -> int {
      return m_order(element, value);
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
    BinarySearch position = find(value);

    if (position.found) {
      m_data.erase(position.index);
      return true;
    } else {
      return false;
    }
  }
  template <typename E>
  IndexRange find_slice_by_key(std::function<E &(T &)> extract, E &key) {
    BinarySearch start = binary_search_by(m_data, [extract, key](T &element) {
      E &key_ = extract(element);
      if (key_ < key) {
        return -1;
      } else {
        return 1;
      }
    });
    BinarySearch end = binary_search_by(m_data, [extract, key](T &element) {
      E &key_ = extract(element);
      if (key_ < key) {
        return 1;
      } else {
        return -1;
      }
    });
    return IndexRange{start.index, end.index};
  }
  bool iter(T value) {
    BinarySearch position = find(value);

    if (position.found) {
      m_data.erase(position.index);
      return true;
    } else {
      return false;
    }
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

public:
  bool atEnd() const;
  void next();
  std::string city() const;
  std::string addr() const;
  std::string region() const;
  unsigned id() const;
  std::string owner() const;

private:
  // todo
};

class CLandRegister {
  std::vector<LandEntry> lands;

public:
  bool add(const std::string &city, const std::string &addr,
           const std::string &region, unsigned int id);

  bool del(const std::string &city, const std::string &addr);

  bool del(const std::string &region, unsigned int id);

  bool getOwner(const std::string &city, const std::string &addr,
                std::string &owner) const;

  bool getOwner(const std::string &region, unsigned int id,
                std::string &owner) const;

  bool newOwner(const std::string &city, const std::string &addr,
                const std::string &owner);

  bool newOwner(const std::string &region, unsigned int id,
                const std::string &owner);

  size_t count(const std::string &owner) const;

  CIterator listByAddr() const;

  CIterator listByOwner(const std::string &owner) const;
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
};

int main(void) {
  TrashSet<Tuple> set([](Tuple &a, Tuple &b) {
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
  });
  set.insert(Tuple{1, 2});
  set.insert(Tuple{1, 3});
  set.insert(Tuple{5, 3});
  set.insert(Tuple{-1, 3});
  set.insert(Tuple{1, -5});

  int a = 1;
  set.find_slice_by_key<int>([](Tuple &e) -> int & { return e.x; }, a);

  test0();
  test1();
  return EXIT_SUCCESS;
}

#endif /* __PROGTEST__ */
