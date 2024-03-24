#include <cassert>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

class Date {
    int m_day = 0;
    int m_month = 0;
    int m_year = 0;

  public:
    Date(int day, int month, int year) {
        m_day = day;
        m_month = month;
        m_year = year;
    }

    Date(const std::string& date) {
        std::stringstream stream(date);
        // sscanf(date.c_str(), "%d-%d-%d", &m_day, &m_month, &m_year);
        char c = 0;
        stream >> m_day >> c >> m_month >> c >> m_year;
    }

    int compare(const Date& other) const {
        if (m_year < other.m_year) {
            return -1;
        } else if (m_year > other.m_year) {
            return 1;
        }

        if (m_month < other.m_month) {
            return -1;
        } else if (m_month > other.m_month) {
            return 1;
        }

        if (m_day < other.m_day) {
            return -1;
        } else if (m_day > other.m_day) {
            return 1;
        }

        return 0;
    }

    bool lessThan(const Date& other) const {
        return compare(other) < 0;
    }

    void print(std::ostream& os) const {
        os << m_day << '.' << m_month << '.' << m_year;
    }
};

void test_compare() {
    Date a("15.1.2000");
    Date b("16.1.2000");  // < 0
    assert(a.compare(b) < 0);

    Date c("1.1.2000");
    assert(c.compare(c) == 0);

    Date d("9.4.2010");
    Date e("9.3.2010");  // > 0
    assert(d.compare(e) > 0);
}

enum class Gender { Male, Female };

void print_gender(Gender self, std::ostream& os) {
    switch (self) {
        case Gender::Male: {
            os << "male";
            break;
        };
        case Gender::Female: {
            os << "female";
            break;
        };
        default: {
            os << "dinosaur";
            break;
        };
    }
}

void normalize_words(std::string& str) {
    bool prev_word_boundary = true;
    for (int i = 0; i < str.length(); i++) {
        char c = str[i];
        if (std::isalpha(c)) {
            if (prev_word_boundary) {
                c = std::toupper(c);
            } else {
                c = std::tolower(c);
            }
            str[i] = c;
            prev_word_boundary = false;
        } else {
            prev_word_boundary = true;
        }
    }
}

class Person {
    std::string m_name;
    Date m_birth;
    Gender m_gender;

  public:
    Person(std::string name, Date birth, Gender gender) :
        m_name(name),
        m_birth(birth),
        m_gender(gender) {
        normalize_words(m_name);
    }

    const std::string& get_name() const {
        return m_name;
    }

    void print(std::ostream& os) const {
        os << m_name;
        os << ", ";

        m_birth.print(os);
        os << ", ";

        print_gender(m_gender, os);
        os << std::endl;
    }
};

int main() {
    test_compare();

    Person a("fiLIp grEgoR", Date(1, 1, 1), Gender::Female);
    a.print(std::cout);
}
