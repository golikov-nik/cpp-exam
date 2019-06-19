//
// Created by nigo on 19/06/19.
//

#ifndef BASIC_VECTOR_H
#define BASIC_VECTOR_H

#include <cstddef>
#include <utility>
#include <algorithm>
#include <memory>

template <typename T, size_t INITIAL_CAPACITY = 4>
struct basic_vector {
  using iterator = T*;
  using const_iterator = T const*;

  basic_vector() : data_(allocate(INITIAL_CAPACITY)) {
    size() = 0;
    ref_count() = 1;
  }

  basic_vector(basic_vector const& other) noexcept : data_(other.data_) {
    ++ref_count();
  }

  explicit basic_vector(T const& val) : basic_vector() {
    push_back(val);
  }

  explicit basic_vector(T&& val) : basic_vector() {
    push_back(std::move(val));
  }

  size_t capacity(char const* p) const noexcept {
    return *reinterpret_cast<size_t const*>(p);
  }

  size_t& capacity(char* p) noexcept {
    return *reinterpret_cast<size_t*>(p);
  }

  size_t capacity() const noexcept {
    return capacity(data_);
  }

  size_t& capacity() noexcept {
    return capacity(data_);
  }

  size_t size(char const* p) const noexcept {
    return *(reinterpret_cast<size_t const*>(p) + 1);
  }

  size_t& size(char* p) noexcept {
    return *(reinterpret_cast<size_t*>(p) + 1);
  }

  size_t size() const noexcept {
    return size(data_);
  }

  size_t& size() noexcept {
    return size(data_);
  }

  size_t ref_count(char const* p) const noexcept {
    return *(reinterpret_cast<size_t const*>(p) + 2);
  }

  size_t& ref_count(char* p) noexcept {
    return *(reinterpret_cast<size_t*>(p) + 2);
  }

  size_t ref_count() const noexcept {
    return ref_count(data_);
  }

  size_t& ref_count() noexcept {
    return ref_count(data_);
  }

  iterator begin(char* p) noexcept {
    return reinterpret_cast<T*>(p + DATA_SHIFT);
  }

  const_iterator begin(char const* p) const noexcept {
    return reinterpret_cast<T const*>(p + DATA_SHIFT);
  }

  iterator begin() noexcept {
    return begin(data_);
  }

  const_iterator begin() const noexcept {
    return begin(data_);
  }

  void push_back(T const& val) {
    if (size() != capacity()) {
      new(begin() + size()) T(val);
    } else {
      char* new_data = make_copy(std::max(INITIAL_CAPACITY, 2 * capacity()));
      try {
        new(begin(new_data) + size()) T(val);
      } catch (...) {
        destroy(new_data);
        throw;
      }
      clear();
      data_ = new_data;
    }
    ++size();
  }

  void push_back(T&& val) {
    if (size() != capacity()) {
      new(begin() + size()) T(std::move(val));
    } else {
      char* new_data = make_copy(std::max(INITIAL_CAPACITY, 2 * capacity()));
      try {
        new(begin(new_data) + size()) T(std::move(val));
      } catch (...) {
        destroy(new_data);
        throw;
      }
      clear();
      data_ = new_data;
    }
    ++size();
  }

  template <typename... Args>
  void emplace_back(Args&& ... args) {
    if (size() != capacity()) {
      new(begin() + size()) T(std::forward<Args>(args)...);
    } else {
      char* new_data = make_copy(std::max(INITIAL_CAPACITY, 2 * capacity()));
      try {
        new(begin(new_data) + size()) T(std::forward<Args>(args)...);
      } catch (...) {
        destroy(new_data);
        throw;
      }
      clear();
      data_ = new_data;
    }
    ++size();
  }

  void pop_back() noexcept {
    assert(size());
    (begin() + (--size()))->~T();
  }

  void reserve(size_t n) {
    assert(n > capacity());
    set_capacity(n);
  }

  void shrink_to_fit() {
    assert(capacity() != size());
    set_capacity(size());
  }

  void detach() {
    if (ref_count() != 1) {
      set_capacity(capacity());
    }
  }

  void clear() noexcept {
    if (!(--ref_count())) {
      destroy(data_);
    }
  }

  ~basic_vector() noexcept {
    clear();
  }

 private:
  static constexpr size_t const ALIGN_T = alignof(T);
  static constexpr size_t const EXTRA = 3 * sizeof(size_t);
  static constexpr size_t const GAP = (ALIGN_T - EXTRA % ALIGN_T) % ALIGN_T;
  static constexpr size_t const DATA_SHIFT = EXTRA + GAP;

  char* data_;

  char* allocate(size_t cap) {
    char* result = static_cast<char*>(operator new(
            DATA_SHIFT + cap * sizeof(T)));
    capacity(result) = cap;
    return result;
  }

  char* make_copy(size_t cap) {
    char* new_data = allocate(cap);
    size(new_data) = size();
    ref_count(new_data) = 1;
    for (auto src = begin(), end = src + size(), dst = begin(new_data);
         src != end; ++src, ++dst) {
      try {
        new(dst) T(std::move_if_noexcept(*src));
      } catch (...) {
        destroy_n(new_data, dst - begin(new_data));
        throw;
      }
    }
    return new_data;
  }

  void set_capacity(size_t cap) {
    char* new_data = make_copy(cap);
    clear();
    data_ = new_data;
  }

  void destroy(char* p) noexcept {
    destroy_n(p, size(p));
  }

  void destroy_n(char* p, size_t n) noexcept {
    std::destroy_n(begin(p), n);
    operator delete(p);
  }
};

#endif //BASIC_VECTOR_H
