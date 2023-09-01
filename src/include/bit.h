#ifndef BIT_H
#define BIT_H

#include <array>
#include <vector>
#include <bit>
#include <cassert>
#include <cstddef>     // size_t
#include <cstdint>
#include <limits>      // std::numeric_limits
#include <type_traits> // std::conditional

template <typename storage_t>
class bitarray_base {
public:
	using data_t = typename storage_t::value_type;
	constexpr static size_t typewidth = std::numeric_limits<data_t>::digits;

	bool get(size_t idx) const { return data[element(idx)] & mask(idx); }
	void set(size_t idx) { data[element(idx)] |= mask(idx); }
	void clear(size_t idx) { data[element(idx)] &= ~mask(idx); }
	void write(size_t idx, int val) { val ? set(idx) : clear(idx); }
	size_t insert() {
		size_t idx = first_free();
		assert(first_free() < capacity());
		set(first_free());
		return idx;
	}

	size_t size() const { return bitscan<std::popcount, scan_all>(0); }
	constexpr size_t capacity() const { return data.size() * typewidth; }

	size_t first_set() const { return bitscan<std::countr_zero, until_first>(0); }
	size_t first_free() const { return bitscan<std::countr_one, until_first>(0); }
	size_t next(size_t idx) {
		data_t backup = data[element(idx)];
		data_t clear_mask = (mask(idx) << 1) - 1;
		data[element(idx)] &= ~clear_mask;
		size_t scan = bitscan<std::countr_zero, until_first>(element(idx));
		data[element(idx)] = backup;
		return std::min(size_t(capacity()), scan + (element(idx) * typewidth));
	}

	class iterator {
	public:
		bool operator!=(const iterator& rhs) const { return idx != rhs.idx; }
		bool operator==(const iterator& rhs) const { return !(*this != rhs); }
		size_t operator*() const { return idx; }
		iterator operator++() {
			idx = ctr->next(idx);
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
	typedef int (*scan_function)(data_t);
	typedef bool (*predicate_function)(data_t);
	template <scan_function scan, predicate_function predicate>
	constexpr size_t bitscan(size_t elem_idx) const {
		size_t sum = 0;
		size_t value = INTMAX_MAX;
		for (unsigned i = elem_idx; i < data.size() && predicate(value); i++) {
			value = scan(data[i]);
			sum += value;
		}
		return sum;
	}

	data_t mask(size_t idx) const { return data_t(data_t(1) << data_t(idx % typewidth)); }
	size_t element(size_t idx) const { return idx / typewidth; }

	// countr_one/zero return integer type width, if no one/zero is found
	static bool until_first(data_t val) { return val >= typewidth; }
	static bool scan_all(data_t) { return true; }
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
