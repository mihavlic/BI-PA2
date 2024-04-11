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
        std::sort(words.begin(), words.end());
        for (auto& word : words) {
            std::transform(
                word.begin(),
                word.end(),
                word.begin(),
                [](unsigned char c) { return std::tolower(c); }
            );
        }
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

    bool matches(const NameWords& other) const {
        // both words are small case and sorted, we can just compare
        return words == other.words;
    }

    bool contains(const NameWords& other) const {
        for (const auto& word : other.words) {
            if (!std::binary_search(words.begin(), words.end(), word)) {
                return false;
            }
        }

        return true;
    }
};

class CStudent {
    std::string name;
    CDate born;
    int enrolled;
    unsigned insert_order;
    NameWords words;

  public:
    CStudent() : born(0, 0, 0) {}

    CStudent(const std::string& name_, const CDate& born_, int enrolled_) :
        name(name_),
        born(born_),
        enrolled(enrolled_),
        insert_order(0),
        words(name) {}

    bool operator==(const CStudent& other) const {
        return name == other.name && born == other.born
            && enrolled == other.enrolled;
    }

    bool operator!=(const CStudent& other) const {
        return !(*this == other);
    }

    friend std::ostream& operator<<(std::ostream& os, const CStudent& t) {
        return os << '(' << t.name << " " << t.born << " " << t.enrolled << " "
                  << t.insert_order << ')';
    }

    void set_insert_order(unsigned insert_order_) {
        insert_order = insert_order_;
    }

    static bool lt_by_all(const CStudent* a, const CStudent* b) {
        if (a->name < b->name) {
            return true;
        }
        if (a->name > b->name) {
            return false;
        }

        if (a->born < b->born) {
            return true;
        }
        if (a->born > b->born) {
            return false;
        }

        if (a->enrolled < b->enrolled) {
            return true;
        }
        return false;
    }

    static bool lt_by_name(const CStudent* a, const CStudent* b) {
        if (a->name == b->name) {
            return a->insert_order < b->insert_order;
        }
        return (a->name < b->name);
    }

    static bool lt_by_born(const CStudent* a, const CStudent* b) {
        if (a->born == b->born) {
            return a->insert_order < b->insert_order;
        }
        return (a->born < b->born);
    }

    static bool lt_by_enrolled(const CStudent* a, const CStudent* b) {
        if (a->enrolled == b->enrolled) {
            return a->insert_order < b->insert_order;
        }
        return (a->enrolled < b->enrolled);
    }

    friend class CSort;
    friend class CStudyDept;
};

class CFilter {
    std::vector<NameWords> names;
    CDate born_start {0, 0, 0};
    CDate born_end {0, 0, 0};
    int enrolled_start = 0;
    int enrolled_end = 0;

  public:
    CFilter() {}

    CFilter& name(const std::string& name) {
        names.emplace_back(NameWords(name));
        return *this;
    }

    CFilter& bornBefore(const CDate& date) {
        born_end = date;
        return *this;
    }

    CFilter& bornAfter(const CDate& date) {
        born_start = date;
        return *this;
    }

    CFilter& enrolledBefore(int year) {
        enrolled_end = year;
        return *this;
    }

    CFilter& enrolledAfter(int year) {
        enrolled_start = year;
        return *this;
    }

    friend class CStudyDept;
};

class CSort {
    std::vector<std::pair<ESortKey, bool>> keys;

  public:
    CSort() {}

    CSort& addKey(ESortKey key, bool ascending) {
        for (auto& el : keys) {
            if (el.first == key) {
                return *this;
            }
        }
        keys.push_back(std::make_pair(key, ascending));

        return *this;
    }

    void sort(std::vector<CStudent*>& vec) const {
        std::sort(
            vec.begin(),
            vec.end(),
            [this](CStudent* const& a, CStudent* const& b) -> bool {
                for (auto& key : this->keys) {
                    const CStudent* a_ = a;
                    const CStudent* b_ = b;

                    if (!key.second) {
                        std::swap(a_, b_);
                    }

                    switch (key.first) {
                        case ESortKey::NAME:
                            if (a_->name == b_->name) {
                                break;
                            }
                            return a_->name < b_->name;
                        case ESortKey::BIRTH_DATE:
                            if (a_->born == b_->born) {
                                break;
                            }
                            return a_->born < b_->born;
                        case ESortKey::ENROLL_YEAR:
                            if (a_->enrolled == b_->enrolled) {
                                break;
                            }
                            return a_->enrolled < b_->enrolled;
                        default:
                            break;
                    }
                }

                return a->insert_order < b->insert_order;
            }
        );
    }
};

class CStudyDept {
    std::set<CStudent*, decltype(CStudent::lt_by_all)*> by_all {
        CStudent::lt_by_all
    };
    std::set<CStudent*, decltype(CStudent::lt_by_name)*> by_name {
        CStudent::lt_by_name
    };
    std::set<CStudent*, decltype(CStudent::lt_by_born)*> by_born {
        CStudent::lt_by_born
    };
    std::set<CStudent*, decltype(CStudent::lt_by_enrolled)*> by_enrolled {
        CStudent::lt_by_enrolled
    };
    unsigned counter;

  public:
    CStudyDept() {}

    ~CStudyDept() {
        for (CStudent* student : by_all) {
            delete student;
        }
    }

    bool addStudent(const CStudent& x) {
        CStudent* a = new CStudent(x);
        auto ret = by_all.insert(a);

        if (!ret.second) {
            delete a;
            return false;
        }

        a->set_insert_order(counter++);

        by_name.insert(a);
        by_born.insert(a);
        by_enrolled.insert(a);

        return true;
    }

    bool delStudent(const CStudent& x) {
        CStudent* yes = const_cast<CStudent*>(&x);

        auto it = by_all.find(yes);
        if (it == by_all.end()) {
            return false;
        }

        by_name.erase(*it);
        by_born.erase(*it);
        by_enrolled.erase(*it);

        delete *it;
        by_all.erase(it);

        return true;
    }

    static void
    retain_union(std::vector<CStudent*>& vec, std::set<CStudent*>& set) {
        (void)std::remove_if(
            vec.begin(),
            vec.end(),
            [set](CStudent* student) -> bool { return !set.contains(student); }
        );

        set.clear();
        std::copy(vec.begin(), vec.end(), std::inserter(set, set.end()));
        vec.clear();
    }

    std::list<CStudent> search(const CFilter& flt, const CSort& sortOpt) const {
        std::vector<CStudent*> vec;

        CStudent dummy {};

        bool first = true;

        if (flt.born_start < flt.born_end) {
            if (first) {
                first = false;

                dummy.born = flt.born_start;
                auto start = by_born.upper_bound(&dummy);

                dummy.born = flt.born_end;
                auto end = by_born.lower_bound(&dummy);

                std::copy(start, end, std::back_inserter(vec));
            } else {
                //
            }
        }

        if (flt.enrolled_start < flt.enrolled_end) {
            if (first) {
                first = false;

                dummy.enrolled = flt.enrolled_start;
                auto start = by_enrolled.upper_bound(&dummy);

                dummy.enrolled = flt.enrolled_end;
                auto end = by_enrolled.lower_bound(&dummy);

                std::copy(start, end, std::back_inserter(vec));
            } else {
                vec.erase(
                    std::remove_if(
                        vec.begin(),
                        vec.end(),
                        [&flt](CStudent* student) -> bool {
                            return !(
                                flt.enrolled_start < student->enrolled
                                && student->enrolled < flt.enrolled_end
                            );
                        }
                    ),
                    vec.end()
                );
            }
        }

        if (flt.names.size() > 0) {
            auto names_match = [&flt](const CStudent* student) -> bool {
                for (const NameWords& words : flt.names) {
                    if (student->words.matches(words)) {
                        return true;
                    }
                }
                return false;
            };

            if (first) {
                first = false;

                std::copy_if(
                    by_all.begin(),
                    by_all.end(),
                    std::back_inserter(vec),
                    names_match
                );
            } else {
                vec.erase(
                    std::remove_if(
                        vec.begin(),
                        vec.end(),
                        [&names_match](const CStudent* student) -> bool {
                            return !names_match(student);
                        }
                    ),
                    vec.end()
                );
            }
        }

        if (first) {
            first = false;
            std::copy(by_all.begin(), by_all.end(), std::back_inserter(vec));
        }

        sortOpt.sort(vec);

        std::list<CStudent> out;
        for (CStudent* student : vec) {
            out.emplace_back(*student);
        }

        return out;
    }

    std::set<std::string> suggest(const std::string& name) const {
        NameWords search(name);
        std::set<std::string> out;

        for (const CStudent* student : by_name) {
            if (student->words.contains(search)) {
                out.insert(student->name);
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
