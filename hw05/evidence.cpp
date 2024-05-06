#ifndef __PROGTEST__
    #include <algorithm>
    #include <cassert>
    #include <iostream>
    #include <sstream>
    #include <string>
    #include <vector>
#endif /* __PROGTEST__ */

struct Scope {
    bool is_last;
};

class Thing {
    std::vector<Thing*> children;

  public:
    virtual Thing* clone() const {
        return new Thing(*this);
    }

    Thing() {}

    Thing(const Thing& other) : children(other.children.size(), nullptr) {
        for (size_t i = 0; i < other.children.size(); i++) {
            children[i] = other.children[i]->clone();
        }
    }

    virtual ~Thing() {
        for (auto child : children) {
            delete child;
        }
    }

    Thing& operator=(const Thing& other) {
        if (this == &other) {
            return *this;
        }
        Thing copy(other);
        for (auto child : children) {
            delete child;
        }
        children = std::move(copy.children);
        return *this;
    }

    virtual void add_child(Thing* owned_child_ptr) {
        children.push_back(owned_child_ptr);
    }

    virtual void
    print_self(std::ostream& os, std::vector<Scope>& scopes) const {}

    void print_children(std::ostream& os, std::vector<Scope>& scopes) const {
        scopes.push_back({false});
        for (size_t i = 0; i < children.size(); i++) {
            scopes.back().is_last = (i + 1) == children.size();
            children[i]->print_recursive(os, scopes);
        }
        scopes.pop_back();
    }

    virtual void
    print_recursive(std::ostream& os, std::vector<Scope>& scopes) const {
        for (int i = 0; i < ((int)scopes.size()) - 1; i++) {
            if (scopes[i].is_last) {
                os << "  ";
            } else {
                os << "| ";
            }
        }

        if (!scopes.empty()) {
            if (scopes.back().is_last) {
                os << "\\-";
            } else {
                os << "+-";
            }
        }

        print_self(os, scopes);
        os << std::endl;
        print_children(os, scopes);
    }

    const std::vector<Thing*>& get_children() const {
        return children;
    }

    std::vector<Thing*>& get_children() {
        return children;
    }

    friend std::ostream& operator<<(std::ostream& os, const Thing& self) {
        std::vector<Scope> scopes {};
        self.print_recursive(os, scopes);
        return os;
    }
};

class Address: public Thing {
    std::string address;

  public:
    Thing* clone() const override {
        return new Address(*this);
    }

    Address(std::string address) : address(address) {}

    void
    print_self(std::ostream& os, std::vector<Scope>& scopes) const override {
        os << address;
    }
};

class CCPU: public Thing {
    unsigned core_count;
    unsigned mhz;

  public:
    Thing* clone() const override {
        return new CCPU(*this);
    }

    CCPU(unsigned core_count, unsigned mhz) :
        core_count(core_count),
        mhz(mhz) {}

    void
    print_self(std::ostream& os, std::vector<Scope>& scopes) const override {
        // CPU, 8 cores @ 1200MHz
        os << "CPU, " << core_count << " cores @ " << mhz << "MHz";
    }
};

class CMemory: public Thing {
    unsigned mib;

  public:
    Thing* clone() const override {
        return new CMemory(*this);
    }

    CMemory(unsigned mib) : mib(mib) {}

    void
    print_self(std::ostream& os, std::vector<Scope>& scopes) const override {
        // Memory, 2000 MiB
        os << "Memory, " << mib << " MiB";
    }
};

class Partition: public Thing {
    unsigned index;
    unsigned gib;
    std::string path;

  public:
    Thing* clone() const override {
        return new Partition(*this);
    }

    Partition(unsigned index, unsigned gib, std::string path) :
        index(index),
        gib(gib),
        path(path) {}

    void
    print_self(std::ostream& os, std::vector<Scope>& scopes) const override {
        // [0]: 60 GiB, /data
        os << "[" << index << "]: " << gib << " GiB, " << path;
    }
};

class CDisk: public Thing {
  public:
    Thing* clone() const override {
        return new CDisk(*this);
    }

    enum DiskType { SSD, MAGNETIC };

    CDisk(DiskType disk_type, unsigned gib) : disk_type(disk_type), gib(gib) {}

    CDisk& addPartition(unsigned gib, std::string path) {
        add_child(new Partition(this->get_children().size(), gib, path));
        return *this;
    }

    void
    print_self(std::ostream& os, std::vector<Scope>& scopes) const override {
        // HDD, 1500 GiB
        if (disk_type == SSD) {
            os << "SSD, ";
        } else {
            os << "HDD, ";
        }
        os << gib << " GiB";
    }

  private:
    DiskType disk_type;
    unsigned gib;
};

class CComputer: public Thing {
    std::string name;

  public:
    Thing* clone() const override {
        return new CComputer(*this);
    }

    CComputer(std::string name) : name(name) {}

    CComputer& addComponent(CCPU child) {
        add_child(new CCPU(child));
        return *this;
    }

    CComputer& addComponent(CMemory child) {
        add_child(new CMemory(child));
        return *this;
    }

    CComputer& addComponent(CDisk child) {
        add_child(new CDisk(child));
        return *this;
    }

    CComputer& addAddress(std::string child) {
        add_child(new Address(child));
        return *this;
    }

    void
    print_self(std::ostream& os, std::vector<Scope>& scopes) const override {
        std::vector<Thing*>& children =
            *const_cast<std::vector<Thing*>*>(&get_children());

        // 10x coder right here
        std::stable_sort(
            children.begin(),
            children.end(),
            [](Thing* a, Thing* b) -> bool {
                return dynamic_cast<Address*>(a) != 0
                    && dynamic_cast<Address*>(b) == 0;
            }
        );

        // Host: progtest.fit.cvut.cz
        os << "Host: " << name;
    }

    friend class CNetwork;
};

class CNetwork: public Thing {
    std::string name;

  public:
    Thing* clone() const override {
        return new CNetwork(*this);
    }

    CNetwork(std::string name) : name(name) {}

    CNetwork& addComputer(CComputer child) {
        add_child(new CComputer(child));
        return *this;
    }

    CComputer* findComputer(const std::string& name) {
        for (auto child : this->get_children()) {
            CComputer* computer = dynamic_cast<CComputer*>(child);

            if (computer->name == name) {
                return computer;
            }
        }
        return nullptr;
    }

    void
    print_self(std::ostream& os, std::vector<Scope>& scopes) const override {
        // Network: FIT network
        os << "Network: " << name;
    }
};

#ifndef __PROGTEST__
template<typename T_>
std::string toString(const T_& x) {
    std::ostringstream oss;
    oss << x;
    return oss.str();
}

template<typename T>
void compare(const T& n, std::string expected) {
    std::string got = toString(n);
    if (got != expected) {
        std::cout << "\nExpected:\n" << expected;
        std::cout << "\nGot:     \n" << got;

        assert(0);
    }
}

int main() {
    CNetwork n("FIT network");
    n.addComputer(
         CComputer("progtest.fit.cvut.cz")
             .addAddress("147.32.232.142")
             .addComponent(CCPU(8, 2400))
             .addComponent(CCPU(8, 1200))
             .addComponent(CDisk(CDisk::MAGNETIC, 1500)
                               .addPartition(50, "/")
                               .addPartition(5, "/boot")
                               .addPartition(1000, "/var"))
             .addComponent(CDisk(CDisk::SSD, 60).addPartition(60, "/data"))
             .addComponent(CMemory(2000))
             .addComponent(CMemory(2000))
    )
        .addComputer(CComputer("courses.fit.cvut.cz")
                         .addAddress("147.32.232.213")
                         .addComponent(CCPU(4, 1600))
                         .addComponent(CMemory(4000))
                         .addComponent(CDisk(CDisk::MAGNETIC, 2000)
                                           .addPartition(100, "/")
                                           .addPartition(1900, "/data")))
        .addComputer(CComputer("imap.fit.cvut.cz")
                         .addAddress("147.32.232.238")
                         .addComponent(CCPU(4, 2500))
                         .addAddress("2001:718:2:2901::238")
                         .addComponent(CMemory(8000)));
    compare(
        n,
        "Network: FIT network\n"
        "+-Host: progtest.fit.cvut.cz\n"
        "| +-147.32.232.142\n"
        "| +-CPU, 8 cores @ 2400MHz\n"
        "| +-CPU, 8 cores @ 1200MHz\n"
        "| +-HDD, 1500 GiB\n"
        "| | +-[0]: 50 GiB, /\n"
        "| | +-[1]: 5 GiB, /boot\n"
        "| | \\-[2]: 1000 GiB, /var\n"
        "| +-SSD, 60 GiB\n"
        "| | \\-[0]: 60 GiB, /data\n"
        "| +-Memory, 2000 MiB\n"
        "| \\-Memory, 2000 MiB\n"
        "+-Host: courses.fit.cvut.cz\n"
        "| +-147.32.232.213\n"
        "| +-CPU, 4 cores @ 1600MHz\n"
        "| +-Memory, 4000 MiB\n"
        "| \\-HDD, 2000 GiB\n"
        "|   +-[0]: 100 GiB, /\n"
        "|   \\-[1]: 1900 GiB, /data\n"
        "\\-Host: imap.fit.cvut.cz\n"
        "  +-147.32.232.238\n"
        "  +-2001:718:2:2901::238\n"
        "  +-CPU, 4 cores @ 2500MHz\n"
        "  \\-Memory, 8000 MiB\n"
    );
    n = n;
    CNetwork x = n;
    auto c = x.findComputer("imap.fit.cvut.cz");
    compare(
        *c,
        "Host: imap.fit.cvut.cz\n"
        "+-147.32.232.238\n"
        "+-2001:718:2:2901::238\n"
        "+-CPU, 4 cores @ 2500MHz\n"
        "\\-Memory, 8000 MiB\n"
    );
    c->addComponent(CDisk(CDisk::MAGNETIC, 1000)
                        .addPartition(100, "system")
                        .addPartition(200, "WWW")
                        .addPartition(700, "mail"));
    compare(
        x,
        "Network: FIT network\n"
        "+-Host: progtest.fit.cvut.cz\n"
        "| +-147.32.232.142\n"
        "| +-CPU, 8 cores @ 2400MHz\n"
        "| +-CPU, 8 cores @ 1200MHz\n"
        "| +-HDD, 1500 GiB\n"
        "| | +-[0]: 50 GiB, /\n"
        "| | +-[1]: 5 GiB, /boot\n"
        "| | \\-[2]: 1000 GiB, /var\n"
        "| +-SSD, 60 GiB\n"
        "| | \\-[0]: 60 GiB, /data\n"
        "| +-Memory, 2000 MiB\n"
        "| \\-Memory, 2000 MiB\n"
        "+-Host: courses.fit.cvut.cz\n"
        "| +-147.32.232.213\n"
        "| +-CPU, 4 cores @ 1600MHz\n"
        "| +-Memory, 4000 MiB\n"
        "| \\-HDD, 2000 GiB\n"
        "|   +-[0]: 100 GiB, /\n"
        "|   \\-[1]: 1900 GiB, /data\n"
        "\\-Host: imap.fit.cvut.cz\n"
        "  +-147.32.232.238\n"
        "  +-2001:718:2:2901::238\n"
        "  +-CPU, 4 cores @ 2500MHz\n"
        "  +-Memory, 8000 MiB\n"
        "  \\-HDD, 1000 GiB\n"
        "    +-[0]: 100 GiB, system\n"
        "    +-[1]: 200 GiB, WWW\n"
        "    \\-[2]: 700 GiB, mail\n"
    );
    compare(
        n,
        "Network: FIT network\n"
        "+-Host: progtest.fit.cvut.cz\n"
        "| +-147.32.232.142\n"
        "| +-CPU, 8 cores @ 2400MHz\n"
        "| +-CPU, 8 cores @ 1200MHz\n"
        "| +-HDD, 1500 GiB\n"
        "| | +-[0]: 50 GiB, /\n"
        "| | +-[1]: 5 GiB, /boot\n"
        "| | \\-[2]: 1000 GiB, /var\n"
        "| +-SSD, 60 GiB\n"
        "| | \\-[0]: 60 GiB, /data\n"
        "| +-Memory, 2000 MiB\n"
        "| \\-Memory, 2000 MiB\n"
        "+-Host: courses.fit.cvut.cz\n"
        "| +-147.32.232.213\n"
        "| +-CPU, 4 cores @ 1600MHz\n"
        "| +-Memory, 4000 MiB\n"
        "| \\-HDD, 2000 GiB\n"
        "|   +-[0]: 100 GiB, /\n"
        "|   \\-[1]: 1900 GiB, /data\n"
        "\\-Host: imap.fit.cvut.cz\n"
        "  +-147.32.232.238\n"
        "  +-2001:718:2:2901::238\n"
        "  +-CPU, 4 cores @ 2500MHz\n"
        "  \\-Memory, 8000 MiB\n"
    );
    return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
