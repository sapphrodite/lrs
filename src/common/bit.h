#ifndef BIT_H
#define BIT_H

#include <array>
#include <vector>
#include <bit>
#include <cstddef>     // size_t
#include <type_traits> // std::conditional
#include <limits>      // std::numeric_limits
#include <cstdint>


template <typename storage_t>
class bitarray_base {
public:
    using element_type = typename storage_t::value_type;
    constexpr static size_t bits_per_element = std::numeric_limits<element_type>::digits;

    bool get(size_t index) const { return data[_element(index)] & _mask(index); }
    void set(size_t index) { data[_element(index)] |= _mask(index); }
    void clear(size_t index) { data[_element(index)] &= ~_mask(index); }
    void write(size_t index, int val) { val ? set(index) : clear(index); }

    size_t size() const { return bitscan<std::popcount, scan_all>(0); }
    constexpr size_t capacity() const { return data.size() * bits_per_element; }

    size_t least_set_bit() const { return bitscan<std::countr_zero, until_first>(0); }
    size_t least_unset_bit() const { return bitscan<std::countr_one, until_first>(0); }
    size_t next_set_bit(size_t index) {
        element_type elem_backup = data[_element(index)];
        element_type clear_mask = (_mask(index) << 1) - 1;
        data[_element(index)] &= ~clear_mask;
        size_t scan = bitscan<std::countr_zero, until_first>(_element(index));
        data[_element(index)] = elem_backup;
        return std::min(size_t(capacity()), scan + (_element(index) *  bits_per_element));
    }

    size_t popcnt_before(size_t index) {
        if (index == 0)
            return 0;
        size_t size = 0;
        for (size_t i = 0; i < index / bits_per_element; i++) {
            size += std::popcount(data[i]);
        }

        size_t bruh = bits_per_element;
        auto bmask = (1 << (index % bits_per_element) + 1) - 1;
        return size + std::popcount(data[_element(index)] & bmask);
    }

    class iterator {
    public:
        bool operator!=(const iterator& rhs) const { return idx != rhs.idx; }
        bool operator==(const iterator& rhs) const { return !(*this != rhs); }
        size_t operator*() const { return idx; }
        iterator operator++() {
            idx = ctr->next_set_bit(idx);
            return *this;
        }
        bool pte() { return idx >= ctr->capacity(); }
    private:
        iterator(size_t idx, bitarray_base<storage_t>* ctr) : idx(idx), ctr(ctr) {
            if (idx != ctr->capacity() && ctr->get(idx) == false)
                ++(*this);
        };

        size_t idx = 0;
        bitarray_base<storage_t>* ctr = nullptr;
        friend class bitarray_base<storage_t>;
    };

    iterator begin() { return iterator(0, this); }
    iterator end() { return iterator(capacity(), this); }
private:
    typedef int (*scan_function)(element_type);
    typedef bool (*predicate_function)(element_type);
    template <scan_function scan, predicate_function predicate>
    constexpr size_t bitscan(size_t element_index) const {
        size_t sum = 0;
        size_t scanned_value = INTMAX_MAX;
        for (unsigned i = element_index; i < data.size() && predicate(scanned_value); i++) {
            scanned_value = scan(data[i]);
            sum += scanned_value;
        }
        return sum;
    }

    element_type _mask(size_t index) const { return element_type(element_type(1) << element_type(index % bits_per_element)); }
    size_t _element(size_t index) const { return index / bits_per_element; }
    size_t _element_bit(size_t index) const { return index % bits_per_element; }

    // countr_one/zero return integer type width, if no one/zero is found
    static bool until_first(element_type val) { return val >= bits_per_element; }
    static bool scan_all(element_type) { return true; }
protected:
    storage_t data;
};


// todo - rename these, put them somewhere else with other calculation functions
constexpr static size_t elems_for(size_t num_bits) { return 1 + ((num_bits - 1) / 64); }

template <unsigned num_bits>
using int_t = std::conditional_t<num_bits <= 8, uint8_t,
    typename std::conditional_t<num_bits <= 16, uint16_t,
    typename std::conditional_t<num_bits <= 32, uint32_t, size_t>>>;

template <unsigned num_bits>
class bitarray : public bitarray_base<std::array<int_t<num_bits>, elems_for(num_bits)>> {};

class bitvector: public bitarray_base<std::vector<size_t>> {
public:
    bitvector() = default;
    bitvector(size_t num_bits) { resize(num_bits); }
    void resize(size_t num_bits) { this->data.resize(elems_for(num_bits)); }
};

#endif //BIT_H
