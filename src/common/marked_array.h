#ifndef BIT_TYPES_H
#define BIT_TYPES_H

#include <common/bit.h>
#include <common/assertion.h>

#include <array>
#include <bit>
#include <cstddef>

template <typename storage_t, typename marking_t>
class marked_storage {
public:
    using element = storage_t::value_type;

    constexpr bool exists(size_t id) const { return markers.get(id); }
    constexpr void remove(size_t id) { markers.clear(id); }
    constexpr element& insert(size_t id, const element& e) {
        data[id] = e;
        markers.set(id);
        return data[id];
    }

    constexpr element& operator[] (size_t id) const {
        assertion(exists(id), "Cannot access unmarked element");
        return (element&) data[id];
    }

    constexpr size_t size() const { return markers.size(); }
    constexpr size_t capacity() const { return data.size(); }
    unsigned first_free_id() const { return markers.least_unset_bit(); }

    class iterator {
    public:
        size_t index() const { return *impl; }
        bool operator!=(const iterator& rhs) const { return index() != rhs.index(); }
        bool operator==(const iterator& rhs) const { return !(*this == rhs); }
        element& operator*() { return (element&) ctr->data[index()]; };
        iterator operator++() {
            ++impl;
            return *this;
        }
    private:
        iterator(marked_storage* ctr, marking_t::iterator itr) : ctr(ctr), impl(itr) {}

        marking_t::iterator impl = markers.begin();
        marked_storage* ctr = nullptr;
        friend class marked_storage;
    };

    iterator begin() { return iterator(this, markers.begin()); }
    iterator end() { return iterator(this, markers.end()); }
protected:
    marking_t markers;
    storage_t data;
};


template <typename element, unsigned size>
class marked_array : public marked_storage<std::array<element, size>, bitarray<size>> {};

template <typename element>
class marked_vector : public marked_storage<std::vector<element>, bitvector> {
public:
    marked_vector() = default;
    marked_vector(size_t size) { set_capacity(size); }
    void set_capacity(size_t new_size) {
        this->markers.resize(new_size);
        this->data.resize(new_size);
    }
    // todo - both of these functions need better names
    size_t find_spot() {
        if (this->first_free_id() == this->data.size()) {
            set_capacity(this->data.size() * 2 + 1);
        }
        return this->first_free_id();
    }
    size_t insert_any(const element& e) {
        size_t spot = find_spot();
        (*this).insert(spot, e);
        return spot;
    }
};

#endif //BIT_TYPES_H
