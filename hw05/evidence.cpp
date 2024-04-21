#ifndef __PROGTEST__
    #include <algorithm>
    #include <cassert>
    #include <functional>
    #include <iomanip>
    #include <iostream>
    #include <list>
    #include <map>
    #include <memory>
    #include <set>
    #include <sstream>
    #include <string>
    #include <vector>
#endif /* __PROGTEST__ */

class CComputer {};

class CNetwork {};

class CCPU {};

class CMemory {};

class CDisk {};

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
