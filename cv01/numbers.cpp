#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int main() {
    int count = -1;
    std::string name = std::string();

    std::cin >> name;
    std::cin >> count;

    if (count < 0 || name.empty()) {
        std::cout << "Neplatny vstup\n";
        return 1;
    }

    std::vector<int> numbers{};
    for (int i = 0; i < count; i++) {
        int number = 0;
        std::cin >> number;
        numbers.push_back(number);
    }

    std::sort(numbers.begin(), numbers.end());

    if (ends_with(name, ".txt")) {
        std::cout << "Writing text file" << std::endl;
        std::ofstream output_file(name);
        for (int number : numbers) {
            output_file << number << std::endl;
        }
        output_file.close();
    } else {
        std::cout << "Writing binary file" << std::endl;
        std::ofstream output_file(name, std::ios::binary);
        output_file.write((char*)numbers.data(), numbers.size() * sizeof(int));
        output_file.close();
    }
}