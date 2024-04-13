#include <cstdint>
#ifndef __PROGTEST__
    #include <algorithm>
    #include <array>
    #include <cassert>
    #include <cctype>
    #include <climits>
    #include <compare>
    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <functional>
    #include <iomanip>
    #include <iostream>
    #include <iterator>
    #include <list>
    #include <map>
    #include <memory>
    #include <set>
    #include <string>
    #include <unordered_map>
    #include <unordered_set>
    #include <vector>

class CDate {
  public:
    CDate(int y, int m, int d) : m_Y(y), m_M(m), m_D(d) {}

    std::strong_ordering operator<=>(const CDate& other) const = default;

    friend std::ostream& operator<<(std::ostream& os, const CDate& d) {
        return os << d.m_Y << '-' << d.m_M << '-' << d.m_D;
    }

  private:
    int m_Y;
    int m_M;
    int m_D;
};

enum class ESortKey { NAME, BIRTH_DATE, ENROLL_YEAR };
#endif /* __PROGTEST__ */

struct NameWords {
    std::vector<std::string> words;

    NameWords() {}

    explicit NameWords(const std::string& str) {
        extract_words(str.data(), str.size(), words);
        for (auto& word : words) {
            std::transform(word.begin(), word.end(), word.begin(), [](char c) {
                return std::tolower(c);
            });
        }
        std::sort(words.begin(), words.end());
    }

    static void extract_words(
        const char* str,
        size_t size,
        std::vector<std::string>& words
    ) {
        size_t i = 0;
        while (i < size) {
            while (i < size && str[i] == ' ') {
                i++;
            }

            size_t start = i;
            while (i < size && str[i] != ' ') {
                i++;
            }
            size_t end = i;

            if (start < end) {
                words.emplace_back(std::string(str + start, end - start));
            }
        }
    }

    std::strong_ordering operator<=>(const NameWords& other) const = default;

    bool contains(const NameWords& other) const {
        for (const auto& word : other.words) {
            if (!std::binary_search(words.begin(), words.end(), word)) {
                return false;
            }
        }

        return true;
    }
};

template<typename T>
struct ExtraComparable {
    signed char extra = 0;
    T value;

    ExtraComparable(T value_) : value(value_) {}

    std::strong_ordering operator<=>(const ExtraComparable<T>& other
    ) const = default;

    friend std::ostream&
    operator<<(std::ostream& os, const ExtraComparable<T>& t) {
        return os << t.value;
    }
};

class CStudent {
    ExtraComparable<std::string> name {{}};
    ExtraComparable<CDate> born {{0, 0, 0}};
    ExtraComparable<int> enrolled {0};

    unsigned insert_order {0};
    NameWords words {};

  public:
    CStudent() {}

    CStudent(const std::string& name_, const CDate& born_, int enrolled_) :
        name({name_}),
        born({born_}),
        enrolled(enrolled_),
        insert_order(0),
        words(name_) {}

    friend std::ostream& operator<<(std::ostream& os, const CStudent& t) {
        return os << '(' << t.name << " " << t.born << " " << t.enrolled << " "
                  << t.insert_order << ')';
    }

    std::strong_ordering operator<=>(const CStudent& b) const {
        std::strong_ordering order = born <=> b.born;
        if (order != std::strong_ordering::equal) {
            return order;
        }

        order = enrolled <=> b.enrolled;
        if (order != std::strong_ordering::equal) {
            return order;
        }

        return name <=> b.name;
    }

    bool operator==(const CStudent& b) const {
        return (*this <=> b) == std::strong_ordering::equal;
    }

    bool operator!=(const CStudent& b) const {
        return (*this <=> b) != std::strong_ordering::equal;
    }

    friend class CSort;
    friend class CStudyDept;
};

class CFilter {
    std::set<NameWords> names;
    CDate born_start {0, 0, 0};
    CDate born_end {0, 0, 0};
    int enrolled_start = 0;
    int enrolled_end = 0;
    bool born_start_present = false;
    bool born_end_present = false;
    bool enrolled_start_present = false;
    bool enrolled_end_present = false;

  public:
    CFilter() {}

    CFilter& name(const std::string& name) {
        names.insert(NameWords(name));
        return *this;
    }

    CFilter& bornBefore(const CDate& date) {
        born_end = date;
        born_end_present = true;
        return *this;
    }

    CFilter& bornAfter(const CDate& date) {
        born_start = date;
        born_start_present = true;
        return *this;
    }

    CFilter& enrolledBefore(int year) {
        enrolled_end = year;
        enrolled_end_present = true;
        return *this;
    }

    CFilter& enrolledAfter(int year) {
        enrolled_start = year;
        enrolled_start_present = true;
        return *this;
    }

    friend class CStudyDept;
};

class CSort {
    std::vector<std::pair<ESortKey, bool>> keys;

  public:
    CSort() {}

    CSort& addKey(ESortKey key, bool ascending) {
        keys.push_back(std::make_pair(key, ascending));
        return *this;
    }

    void sort(std::vector<CStudent>& vec) const {
        std::sort(
            vec.begin(),
            vec.end(),
            [this](const CStudent& a, const CStudent& b) -> bool {
                for (auto& key : this->keys) {
                    const CStudent* a_ = &a;
                    const CStudent* b_ = &b;

                    if (!key.second) {
                        std::swap(a_, b_);
                    }

                    std::strong_ordering order = std::strong_ordering::equal;
                    switch (key.first) {
                        case ESortKey::NAME:
                            order = a_->name.value <=> b_->name.value;
                            break;
                        case ESortKey::BIRTH_DATE:
                            order = a_->born.value <=> b_->born.value;
                            break;
                        case ESortKey::ENROLL_YEAR:
                            order = a_->enrolled.value <=> b_->enrolled.value;
                            break;
                        default:
                            break;
                    }
                    if (order != std::strong_ordering::equal) {
                        return order == std::strong_ordering::less;
                    }
                }

                return a.insert_order < b.insert_order;
            }
        );
    }
};

class CStudyDept {
    std::set<CStudent> set;
    unsigned counter;

  public:
    CStudyDept() {}

    bool addStudent(const CStudent& x) {
        auto pair = set.insert(x);

        if (!pair.second) {
            return false;
        }

        // yeees
        const_cast<CStudent*>(&*pair.first)->insert_order = counter++;

        return true;
    }

    bool delStudent(const CStudent& x) {
        auto erased_count = set.erase(x);
        return erased_count > 0;
    }

    std::list<CStudent> search(const CFilter& flt, const CSort& sortOpt) const {
        CStudent start_dummy {};
        CStudent end_dummy {};

        if (flt.born_start_present) {
            start_dummy.born = flt.born_start;
        } else {
            start_dummy.born.extra = -1;
        }
        if (flt.born_end_present) {
            end_dummy.born = flt.born_end;
        } else {
            end_dummy.born.extra = 1;
        }

        if (flt.enrolled_start_present) {
            start_dummy.enrolled = flt.enrolled_start;
        } else {
            start_dummy.enrolled.extra = -1;
        }
        if (flt.enrolled_end_present) {
            end_dummy.enrolled = flt.enrolled_end;
        } else {
            end_dummy.enrolled.extra = 1;
        }

        start_dummy.name.extra = -1;
        end_dummy.name.extra = 1;

        if (start_dummy >= end_dummy) {
            return {};
        }

        auto start = set.upper_bound(start_dummy);
        auto end = set.lower_bound(end_dummy);

        std::vector<CStudent> vec {};

        if (flt.names.size() > 0) {
            auto names_match = [&flt](const CStudent& student) -> bool {
                return flt.names.contains(student.words);
            };

            std::copy_if(start, end, std::back_inserter(vec), names_match);
        } else {
            vec = std::vector<CStudent> {start, end};
        }

        sortOpt.sort(vec);

        return std::list<CStudent> {vec.begin(), vec.end()};
    }

    std::set<std::string> suggest(const std::string& name) const {
        NameWords search(name);
        std::set<std::string> out;

        for (const CStudent& student : set) {
            if (student.words.contains(search)) {
                out.insert(student.name.value);
            }
        }

        return out;
    }
};

#ifndef __PROGTEST__
bool compare_list(
    const std::list<CStudent>& got,
    const std::list<CStudent>& expected
) {
    bool equal = got == expected;
    if (!equal) {
        std::cout << "expected:\n";
        for (const auto& student : expected) {
            std::cout << " " << student << std::endl;
        }
        std::cout << "got:\n";
        for (const auto& student : got) {
            std::cout << " " << student << std::endl;
        }
    }
    return equal;
}

bool compare_set(
    const std::set<std::string>& got,
    const std::set<std::string>& expected
) {
    bool equal = got == expected;
    if (!equal) {
        std::cout << "expected:\n";
        for (const auto& student : expected) {
            std::cout << " " << student << std::endl;
        }
        std::cout << "got:\n";
        for (const auto& student : got) {
            std::cout << " " << student << std::endl;
        }
    }
    return equal;
}

int main(void) {
    CStudyDept x0;
    assert(
        CStudent("James Bond", CDate(1980, 4, 11), 2010)
        == CStudent("James Bond", CDate(1980, 4, 11), 2010)
    );
    assert(
        !(CStudent("James Bond", CDate(1980, 4, 11), 2010)
          != CStudent("James Bond", CDate(1980, 4, 11), 2010))
    );
    assert(
        CStudent("James Bond", CDate(1980, 4, 11), 2010)
        != CStudent("Peter Peterson", CDate(1980, 4, 11), 2010)
    );
    assert(
        !(CStudent("James Bond", CDate(1980, 4, 11), 2010)
          == CStudent("Peter Peterson", CDate(1980, 4, 11), 2010))
    );
    assert(
        CStudent("James Bond", CDate(1980, 4, 11), 2010)
        != CStudent("James Bond", CDate(1997, 6, 17), 2010)
    );
    assert(
        !(CStudent("James Bond", CDate(1980, 4, 11), 2010)
          == CStudent("James Bond", CDate(1997, 6, 17), 2010))
    );
    assert(
        CStudent("James Bond", CDate(1980, 4, 11), 2010)
        != CStudent("James Bond", CDate(1980, 4, 11), 2016)
    );
    assert(
        !(CStudent("James Bond", CDate(1980, 4, 11), 2010)
          == CStudent("James Bond", CDate(1980, 4, 11), 2016))
    );
    assert(
        CStudent("James Bond", CDate(1980, 4, 11), 2010)
        != CStudent("Peter Peterson", CDate(1980, 4, 11), 2016)
    );
    assert(
        !(CStudent("James Bond", CDate(1980, 4, 11), 2010)
          == CStudent("Peter Peterson", CDate(1980, 4, 11), 2016))
    );
    assert(
        CStudent("James Bond", CDate(1980, 4, 11), 2010)
        != CStudent("Peter Peterson", CDate(1997, 6, 17), 2010)
    );
    assert(
        !(CStudent("James Bond", CDate(1980, 4, 11), 2010)
          == CStudent("Peter Peterson", CDate(1997, 6, 17), 2010))
    );
    assert(
        CStudent("James Bond", CDate(1980, 4, 11), 2010)
        != CStudent("James Bond", CDate(1997, 6, 17), 2016)
    );
    assert(
        !(CStudent("James Bond", CDate(1980, 4, 11), 2010)
          == CStudent("James Bond", CDate(1997, 6, 17), 2016))
    );
    assert(
        CStudent("James Bond", CDate(1980, 4, 11), 2010)
        != CStudent("Peter Peterson", CDate(1997, 6, 17), 2016)
    );
    assert(
        !(CStudent("James Bond", CDate(1980, 4, 11), 2010)
          == CStudent("Peter Peterson", CDate(1997, 6, 17), 2016))
    );
    assert(x0.addStudent(CStudent("John Peter Taylor", CDate(1983, 7, 13), 2014)
    ));
    assert(x0.addStudent(CStudent("John Taylor", CDate(1981, 6, 30), 2012)));
    assert(x0.addStudent(CStudent("Peter Taylor", CDate(1982, 2, 23), 2011)));
    assert(x0.addStudent(CStudent("Peter John Taylor", CDate(1984, 1, 17), 2017)
    ));
    assert(x0.addStudent(CStudent("James Bond", CDate(1981, 7, 16), 2013)));
    assert(x0.addStudent(CStudent("James Bond", CDate(1982, 7, 16), 2013)));
    assert(x0.addStudent(CStudent("James Bond", CDate(1981, 8, 16), 2013)));
    assert(x0.addStudent(CStudent("James Bond", CDate(1981, 7, 17), 2013)));
    assert(x0.addStudent(CStudent("James Bond", CDate(1981, 7, 16), 2012)));
    assert(x0.addStudent(CStudent("Bond James", CDate(1981, 7, 16), 2013)));
    assert(compare_list(
        x0.search(
            CFilter()
                .bornAfter(CDate(1980, 4, 11))
                .bornBefore(CDate(1983, 7, 13))
                .name("John Taylor")
                .name("james BOND"),
            CSort()
                .addKey(ESortKey::ENROLL_YEAR, false)
                .addKey(ESortKey::BIRTH_DATE, false)
                .addKey(ESortKey::NAME, true)
        ),
        (std::list<CStudent> {
            CStudent("James Bond", CDate(1982, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 8, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 17), 2013),
            CStudent("Bond James", CDate(1981, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 16), 2012),
            CStudent("John Taylor", CDate(1981, 6, 30), 2012)
        })
    ));
    assert(compare_list(
        x0.search(
            CFilter().name("james bond"),
            CSort()
                .addKey(ESortKey::ENROLL_YEAR, false)
                .addKey(ESortKey::BIRTH_DATE, false)
                .addKey(ESortKey::NAME, true)
        ),
        (std::list<CStudent> {
            CStudent("James Bond", CDate(1982, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 8, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 17), 2013),
            CStudent("Bond James", CDate(1981, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 16), 2012)
        })
    ));
    assert(compare_list(
        x0.search(CFilter(), CSort()),
        (std::list<CStudent> {
            CStudent("John Peter Taylor", CDate(1983, 7, 13), 2014),
            CStudent("John Taylor", CDate(1981, 6, 30), 2012),
            CStudent("Peter Taylor", CDate(1982, 2, 23), 2011),
            CStudent("Peter John Taylor", CDate(1984, 1, 17), 2017),
            CStudent("James Bond", CDate(1981, 7, 16), 2013),
            CStudent("James Bond", CDate(1982, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 8, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 17), 2013),
            CStudent("James Bond", CDate(1981, 7, 16), 2012),
            CStudent("Bond James", CDate(1981, 7, 16), 2013)
        })
    ));
    assert(compare_list(
        x0.search(CFilter(), CSort().addKey(ESortKey::NAME, true)),
        (std::list<CStudent> {
            CStudent("Bond James", CDate(1981, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 16), 2013),
            CStudent("James Bond", CDate(1982, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 8, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 17), 2013),
            CStudent("James Bond", CDate(1981, 7, 16), 2012),
            CStudent("John Peter Taylor", CDate(1983, 7, 13), 2014),
            CStudent("John Taylor", CDate(1981, 6, 30), 2012),
            CStudent("Peter John Taylor", CDate(1984, 1, 17), 2017),
            CStudent("Peter Taylor", CDate(1982, 2, 23), 2011)
        })
    ));
    assert(compare_list(
        x0.search(CFilter(), CSort().addKey(ESortKey::NAME, false)),
        (std::list<CStudent> {
            CStudent("Peter Taylor", CDate(1982, 2, 23), 2011),
            CStudent("Peter John Taylor", CDate(1984, 1, 17), 2017),
            CStudent("John Taylor", CDate(1981, 6, 30), 2012),
            CStudent("John Peter Taylor", CDate(1983, 7, 13), 2014),
            CStudent("James Bond", CDate(1981, 7, 16), 2013),
            CStudent("James Bond", CDate(1982, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 8, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 17), 2013),
            CStudent("James Bond", CDate(1981, 7, 16), 2012),
            CStudent("Bond James", CDate(1981, 7, 16), 2013)
        })
    ));
    assert(compare_list(
        x0.search(
            CFilter(),
            CSort()
                .addKey(ESortKey::ENROLL_YEAR, false)
                .addKey(ESortKey::BIRTH_DATE, false)
                .addKey(ESortKey::NAME, true)
        ),
        (std::list<CStudent> {
            CStudent("Peter John Taylor", CDate(1984, 1, 17), 2017),
            CStudent("John Peter Taylor", CDate(1983, 7, 13), 2014),
            CStudent("James Bond", CDate(1982, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 8, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 17), 2013),
            CStudent("Bond James", CDate(1981, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 16), 2012),
            CStudent("John Taylor", CDate(1981, 6, 30), 2012),
            CStudent("Peter Taylor", CDate(1982, 2, 23), 2011)
        })
    ));
    assert(compare_list(
        x0.search(
            CFilter().name("james"),
            CSort().addKey(ESortKey::NAME, true)
        ),
        (std::list<CStudent> {})
    ));
    assert(compare_set(
        x0.suggest("peter"),
        (std::set<std::string> {
            "John Peter Taylor",
            "Peter John Taylor",
            "Peter Taylor"
        })
    ));
    assert(compare_set(
        x0.suggest("bond"),
        (std::set<std::string> {"Bond James", "James Bond"})
    ));
    assert(compare_set(
        x0.suggest("peter joHn"),
        (std::set<std::string> {"John Peter Taylor", "Peter John Taylor"})
    ));
    assert(
        compare_set(x0.suggest("peter joHn bond"), (std::set<std::string> {}))
    );
    assert(compare_set(x0.suggest("pete"), (std::set<std::string> {})));
    assert(compare_set(
        x0.suggest("peter joHn PETER"),
        (std::set<std::string> {"John Peter Taylor", "Peter John Taylor"})
    ));
    assert(!x0.addStudent(CStudent("James Bond", CDate(1981, 7, 16), 2013)));
    assert(x0.delStudent(CStudent("James Bond", CDate(1981, 7, 16), 2013)));
    assert(
        x0.search(
            CFilter()
                .bornAfter(CDate(1980, 4, 11))
                .bornBefore(CDate(1983, 7, 13))
                .name("John Taylor")
                .name("james BOND"),
            CSort()
                .addKey(ESortKey::ENROLL_YEAR, false)
                .addKey(ESortKey::BIRTH_DATE, false)
                .addKey(ESortKey::NAME, true)
        )
        == (std::list<CStudent> {
            CStudent("James Bond", CDate(1982, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 8, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 17), 2013),
            CStudent("Bond James", CDate(1981, 7, 16), 2013),
            CStudent("James Bond", CDate(1981, 7, 16), 2012),
            CStudent("John Taylor", CDate(1981, 6, 30), 2012)
        })
    );
    assert(!x0.delStudent(CStudent("James Bond", CDate(1981, 7, 16), 2013)));
    return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
