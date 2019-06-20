//
// Created by nigo on 15/06/19.
//

#ifndef VECTOR_H
#define VECTOR_H

//  exception-safe
//  small-object
//  sizeof(vector<T> <= sizeof(void*) + max(sizeof(void*), sizeof(T))
//  copy-on-write
//  1 allocation max
//  no default constructor
//  iterators

#include <variant>
#include <utility>
#include <cassert>
#include <algorithm>

#include "basic_vector.h"

template <typename T>
struct vector {
  using value_type = T;

  using iterator = typename basic_vector<T>::iterator;
  using const_iterator = typename basic_vector<T>::const_iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  vector() noexcept = default;

  vector(size_t count, T const& value) {
    reserve(count);
    while (count--) {
      push_back(value);
    }
  }

  explicit vector(size_t count) : vector(count, T()) {
  }

  vector(vector const& other) : data_(other.data_) {
  }

  template <typename InputIterator, typename = std::_RequireInputIter<InputIterator>>
  vector(InputIterator first, InputIterator last) {
    reserve(std::distance(first, last));
    std::copy(first, last, std::back_inserter(*this));
  }

  vector(std::initializer_list<T> init) : vector(init.begin(), init.end()) {
  }

  template <typename InputIterator>
  void assign(InputIterator first, InputIterator last) {
    *this = vector(first, last);
  }

  vector(vector&& other) {
    swap(other);
  }

  vector& operator=(vector const& other) {
    vector copy_(other);
    swap(copy_);
    return *this;
  }

  vector& operator=(vector&& other) {
    swap(other);
    return *this;
  }

  ~vector() noexcept = default;

  void clear() {
    if (holds_nothing()) {
      return;
    }
    if (holds_value()) {
      data_ = empty_t();
      return;
    }
    as_vector().detach();
    as_vector().clear();
  }

  void swap(vector& other) {
    std::swap(data_, other.data_);
  }

  friend void swap(vector& lhs, vector& rhs) {
    lhs.swap(rhs);
  }

  size_t size() const noexcept {
    if (holds_nothing()) {
      return 0;
    }
    if (holds_value()) {
      return 1;
    }
    return as_vector().size();
  }

  size_t capacity() const noexcept {
    if (holds_nothing() || holds_value()) {
      return 1;
    }
    return as_vector().capacity();
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  iterator begin() {
    if (holds_nothing()) {
      return nullptr;
    }
    if (holds_value()) {
      return std::addressof(as_value());
    }
    as_vector().detach();
    return as_vector().begin();
  }

  const_iterator begin() const noexcept {
    if (holds_nothing()) {
      return nullptr;
    }
    if (holds_value()) {
      return std::addressof(as_value());
    }
    return as_vector().begin();
  }

  T* data() {
    return begin();
  }

  T const* data() const noexcept {
    return begin();
  }

  iterator end() {
    return begin() + size();
  }

  const_iterator end() const noexcept {
    return begin() + size();
  }

  reverse_iterator rbegin() {
    return reverse_iterator(end());
  }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  reverse_iterator rend() {
    return reverse_iterator(begin());
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  T& operator[](size_t at) {
    assert(at < size());
    return *(begin() + at);
  }

  T const& operator[](size_t at) const noexcept {
    assert(at < size());
    return *(begin() + at);
  }

  T& front() {
    return (*this)[0];
  }

  T const& front() const noexcept {
    return (*this)[0];
  }

  T& back() {
    return (*this)[size() - 1];
  }

  T const& back() const noexcept {
    return (*this)[size() - 1];
  }

  friend bool operator==(vector const& lhs, vector const& rhs) {
    if (lhs.size() != rhs.size()) {
      return false;
    }
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
  }

  friend bool operator!=(vector const& lhs, vector const& rhs) {
    return !(lhs == rhs);
  }

  friend bool operator<(vector const& lhs, vector const& rhs) {
    return std::lexicographical_compare(lhs.begin(),
                                        lhs.end(),
                                        rhs.begin(),
                                        rhs.end());
  }

  friend bool operator>(vector const& lhs, vector const& rhs) {
    return rhs < lhs;
  }

  friend bool operator<=(vector const& lhs, vector const& rhs) {
    return !(lhs > rhs);
  }

  friend bool operator>=(vector const& lhs, vector const& rhs) {
    return !(lhs < rhs);
  }

  template <typename... Args>
  void emplace_back(Args&& ... args) {
    if (holds_nothing()) {
      data_ = T(std::forward<Args>(args)...);
      return;
    }
    if (holds_value()) {
      auto new_data = basic_vector<T>(as_value());
      new_data.emplace_back(std::forward<T>(args)...);
      data_ = new_data;
      return;
    }
    as_vector().detach();
    as_vector().emplace_back(std::forward<Args>(args)...);
  }

  template <typename S, typename = std::enable_if_t<std::is_convertible_v<S, T>>>
  void push_back(S&& val) {
    emplace_back(std::forward<S>(val));
  }

  void push_back(T const& val, size_t n) {
    size_t sz = size();
    reserve(sz + n);
    while (n--) {
      try {
        push_back(val);
      } catch (...) {
        erase(begin() + sz, end());
        throw;
      }
    }
  }

  void pop_back() {
    assert(!empty());
    if (holds_value()) {
      clear();
      return;
    }
    as_vector().detach();
    as_vector().pop_back();
  }

  void reserve(size_t n) {
    if (n <= capacity()) {
      return;
    }
    if (holds_nothing()) {
      data_ = basic_vector<T>();
    }
    if (holds_value()) {
      data_ = basic_vector<T>(std::move_if_noexcept(as_value()));
    }
    as_vector().detach();
    as_vector().reserve(n);
  }

  void shrink_to_fit() {
    if (capacity() != size()) {
      as_vector().shrink_to_fit();
    }
  }

  template <typename... Args>
  iterator emplace(const_iterator pos, Args&& ... args) {
    auto pos_i = pos - begin();
    emplace_back(std::forward<Args>(args)...);
    pos = begin() + pos_i;
    for (auto it = end() - 1; it != pos; --it) {
      std::iter_swap(it, it - 1);
    }
    return iterator(pos);
  }

  template <typename S, typename = std::enable_if_t<std::is_convertible_v<S, T>>>
  iterator insert(const_iterator pos, S&& val) {
    return emplace(pos, std::forward<S>(val));
  }

  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  }

  iterator erase(const_iterator first, const_iterator last) {
    auto result = iterator(last);
    if (first == last) {
      return result;
    }
    size_t last_i = last - begin();
    size_t sz = size();
    size_t rotate = last_i % sz;
    std::rotate(begin(), begin() + rotate, end());
    size_t new_first = ((first == begin() ? last_i : 0) - rotate + sz) % sz;
    size_t n = last - first;
    while (n--) {
      pop_back();
    }
    std::rotate(begin(), begin() + new_first, end());
    return result;
  }

  void resize(size_t n) {
    resize_unspecified(n, std::is_default_constructible<T>());
  }

  void resize(size_t n, T const& val) {
    if (n <= size()) {
      erase(begin() + n, end());
      return;
    }
    push_back(val, n - size());
  }

 private:

  using empty_t = std::monostate;
  using union_t = std::variant<empty_t, T, basic_vector<T>>;

  union_t data_;

  bool holds_nothing() const noexcept {
    return std::holds_alternative<empty_t>(data_);
  }

  bool holds_value() const noexcept {
    return std::holds_alternative<T>(data_);
  }

  bool holds_vector() const noexcept {
    return std::holds_alternative<basic_vector<T>>(data_);
  }

  T& as_value() noexcept {
    return std::get<T>(data_);
  }

  T const& as_value() const noexcept {
    return std::get<T>(data_);
  }

  basic_vector<T>& as_vector() noexcept {
    return std::get<basic_vector<T>>(data_);
  }

  basic_vector<T> const& as_vector() const noexcept {
    return std::get<basic_vector<T>>(data_);
  }

  void resize_unspecified(size_t n, std::false_type) {
    assert(n <= size());
    erase(begin() + n, end());
  }

  void resize_unspecified(size_t n, std::true_type) {
    if (n <= size()) {
      erase(begin() + n, end());
      return;
    }
    push_back(T(), n - size());
  }
};

template <typename InputIterator> vector(InputIterator first,
                                         InputIterator last) ->
vector<typename std::iterator_traits<InputIterator>::value_type>;

#endif //VECTOR_H
