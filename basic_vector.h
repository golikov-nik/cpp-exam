//
// Created by nigo on 19/06/19.
//

#ifndef BASIC_VECTOR_H
#define BASIC_VECTOR_H

#include <cstddef>
#include <utility>
#include <algorithm>
#include <memory>
#include <cassert>
#include <iostream>

template <typename T, size_t _INITIAL_CAPACITY = 4>
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

  template <typename S, typename =
  std::enable_if_t<std::is_convertible_v<S, T>>>
  explicit basic_vector(S&& val) : basic_vector() {
    push_back(std::forward<S>(val));
  }

  size_t capacity(char const* p) const noexcept {
    return *reinterpret_cast<size_t const*>(p);
  }

  size_t capacity() const noexcept {
    return capacity(data_);
  }

  size_t size(char const* p) const noexcept {
    return *(reinterpret_cast<size_t const*>(p) + 1);
  }

  size_t size() const noexcept {
    return size(data_);
  }

  size_t ref_count(char const* p) const noexcept {
    return *(reinterpret_cast<size_t const*>(p) + 2);
  }

  size_t ref_count() const noexcept {
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

  template <typename... Args>
  void emplace_back(Args&& ... args) {
    if (size() != capacity()) {
      new(begin() + size()) T(std::forward<Args>(args)...);
    } else {
      char* new_data = make_copy(std::max(INITIAL_CAPACITY, 2 * capacity()));
      try {
        new(begin(new_data) + size()) T(std::forward<Args>(args)...);
      } catch (...) {
        revert(new_data, std::__move_if_noexcept_cond<T>());
        throw;
      }
      destroy_self();
      data_ = new_data;
    }
    ++size();
  }

  template <typename S>
  void push_back(S&& val) {
    static_assert(std::is_convertible_v<S, T>);
    emplace_back(std::forward<S>(val));
  }

  void pop_back() noexcept {
    assert(size());
    (begin() + (--size()))->~T();
  }

  void reserve(size_t n) {
    if (n > capacity()) {
      set_capacity(n);
    }
  }

  void shrink_to_fit() {
    set_capacity(size());
  }

  void detach() {
    if (ref_count() != 1) {
      set_capacity(capacity());
    }
  }

  void clear() noexcept {
    std::destroy_n(begin(), size());
    size() = 0;
  }

  ~basic_vector() noexcept {
    destroy_self();
  }

 private:

  static constexpr size_t const ALIGN_T = alignof(T);
  static constexpr size_t const EXTRA = 3 * sizeof(size_t);
  static constexpr size_t const GAP = (ALIGN_T - EXTRA % ALIGN_T) % ALIGN_T;
  static constexpr size_t const DATA_SHIFT = EXTRA + GAP;
  static constexpr size_t const INITIAL_CAPACITY = std::max<size_t>(4,
                                                                    _INITIAL_CAPACITY);
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
    destroy_self();
    data_ = new_data;
  }

  void destroy(char* p) noexcept {
    destroy_n(p, size(p));
  }

  void destroy_n(char* p, size_t n) noexcept {
    std::destroy_n(begin(p), n);
    operator delete(p);
  }

  void destroy_self() noexcept {
    if (!(--ref_count())) {
      destroy(data_);
    }
  }

  //  copying was performed
  void revert(char* new_data, std::true_type) {
    destroy(new_data);
  }

  //  move was performed
  void revert(char* new_data, std::false_type) {
    std::move(begin(new_data), begin(new_data) + size(), begin());
  }

  size_t& capacity(char* p) noexcept {
    return *reinterpret_cast<size_t*>(p);
  }

  size_t& capacity() noexcept {
    return capacity(data_);
  }

  size_t& size(char* p) noexcept {
    return *(reinterpret_cast<size_t*>(p) + 1);
  }

  size_t& size() noexcept {
    return size(data_);
  }

  size_t& ref_count(char* p) noexcept {
    return *(reinterpret_cast<size_t*>(p) + 2);
  }

  size_t& ref_count() noexcept {
    return ref_count(data_);
  }
};

#endif //BASIC_VECTOR_H
