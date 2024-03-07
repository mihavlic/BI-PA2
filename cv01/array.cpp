#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <stdlib.h>

class Array {
    size_t size = 0;
    size_t capacity = 0;
    int* data = nullptr;
public:
    Array(size_t array_size)
    {
        size_t i;
        this->size = array_size;
        this->capacity = array_size;
        this->data = new int[array_size];
        for (i = 0; i < this->size; ++i) {
            this->data[i] = i;
        }
    }

    void push(int element)
    {
        size_t old_size = this->size;
        size_t new_size = old_size + 1;

        if (new_size > this->capacity) {
            size_t new_capacity = this->capacity * 2;
            if (new_size > new_capacity) {
                new_capacity = new_size;
            }

            int* new_data = new int[new_size];
            for (size_t i = 0; i < this->size; i++) {
                new_data[i] = this->data[i];
            }
            
            delete[] this->data;
            this->data = new_data;
            this->capacity = new_capacity;
        }        

        this->size = new_size;
        this->data[old_size] = element;
    }

    void print() const
    {
        for (size_t i = 0; i < this->size; ++i) {
            std::cout << this->data[i] << " ";
        }
        std::cout << std::endl;
    }

    ~Array()
    {
        delete[] this->data;
        this->data = NULL;
    }
};

int main()
{
    std::cout << "What is the size of an array I should create?" << std::endl;
    
    int array_size = -1;
    std::cin >> array_size;

    if (array_size <= 0) {
        std::cout << "Invalid input" << std::endl;
        return 1;
    }

    Array arr(array_size);

    arr.push(99);
    arr.print();

    return 0;
}
