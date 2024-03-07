#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdlib.h>

class Vector {
    size_t m_size = 0;
    size_t m_capacity = 0;
    int* m_data = nullptr;
public:
    Vector() {}
    
    Vector(size_t capacity) {
        m_capacity = capacity;
        m_data = new int[capacity];
    }

    ~Vector()
    {
        delete[] m_data;
        m_data = NULL;
    }

    size_t size() const {
        return m_size;
    }

    void ensure_capacity(int new_size) {
        if (new_size > m_capacity) {
            size_t new_capacity = m_capacity * 2;
            if (new_size > new_capacity) {
                new_capacity = new_size;
            }

            int* new_data = new int[new_size];
            std::memcpy(new_data, m_data, m_size * sizeof(int));
            
            delete[] m_data;
            m_data = new_data;
            m_capacity = new_capacity;
        }
    }

    void push_back(int element)
    {
        size_t old_size = m_size;
        size_t new_size = old_size + 1;

        ensure_capacity(new_size);        

        m_size = new_size;
        m_data[old_size] = element;
    }
    
    int pop_back()
    {
        assert(m_size > 0);
        m_size--;

        return m_data[m_size];
    }

    int back()
    {
        assert(m_size > 0);
        size_t index = m_size - 1; 
        return m_data[index];
    }

    void clear()
    {
        m_size = 0;
    }

    int& at(size_t pos) {
        assert(pos < m_size);
        return m_data[pos];
    }
    
    void insert(size_t index, int element)
    {
        // abc  
        //  ^
        //  size 3
        //  insert 1 i
        //  move_size 2
        //
        // aibc
        assert(index <= m_capacity);
        size_t new_size = m_size += 1;
        ensure_capacity(new_size);

        int* ptr = m_data + index + 1;
        size_t move_size = m_size - index;
        if (move_size > 0) {
            std::memmove((char*)(ptr + 1), (char*)ptr, move_size * sizeof(int));
        }

        *ptr = element;
        m_size = new_size;
    }

    int erase(size_t index)
    {
        // abc  
        //  ^
        //  size 3
        //  remove 1
        //  move_size 1
        //
        // ac
        assert(index < m_capacity);

        int* ptr = m_data + index + 1;
        int removed = *ptr;

        size_t move_size = m_size - (index + 1);
        if (move_size > 0) {
            std::memmove((char*)ptr, (char*)(ptr + 1), move_size * sizeof(int));
        }

        m_size--;
        return removed;
    }
    
    void print() const
    {
        // print_size();
        for (size_t i = 0; i < m_size; ++i) {
            std::cout << m_data[i] << " ";
        }
        std::cout << "     (" << m_size << " " << m_capacity << ")" << std::endl;
    }

    size_t lower_bound(int val) {
        int* first = m_data;
        size_t count = size();
        int* it;

        while (count > 0) {
            size_t step = count / 2;
            it = first + step;

            if (*it < val) {
                first = ++it;
                count -= step + 1;
            } else {
                count = step;
            }
        }

        return first - m_data;
    }
};

int main()
{
    Vector arr{};

    arr.push_back(1);
    arr.push_back(2);
    arr.push_back(3);
    arr.print();

    arr.pop_back();
    arr.print();
    
    arr.insert(1, 99);
    arr.insert(1, 98);
    arr.print();

    arr.erase(1);
    arr.print();

    arr.clear();
    arr.push_back(1);
    arr.push_back(2);
    arr.push_back(8);
    arr.push_back(9);
    std::cout << arr.lower_bound(0) << std::endl;
    std::cout << arr.lower_bound(2) << std::endl;
    std::cout << arr.lower_bound(3) << std::endl;

    return 0;
}
