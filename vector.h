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

//  TODO: clear() should not change capacity
//  TODO: sizeof

#include <variant>
#include <memory>
#include <algorithm>
#include <utility>
#include <cassert>

template <typename T, size_t INITIAL_CAPACITY = 4>
struct buffer {
  using iterator = T*;
  using const_iterator = T const*;

  buffer() : data_(allocate(INITIAL_CAPACITY)) {
    size() = 0;
    ref_count() = 1;
  }

  buffer(buffer const& other) noexcept : data_(other.data_) {
    ++ref_count();
  }

  explicit buffer(T const& val) : buffer() {
    push_back(val);
  }

  explicit buffer(T&& val) : buffer() {
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
    set_capacity(n);
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
    if (!(--ref_count())) {
      destroy(data_);
    }
  }

  ~buffer() noexcept {
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
    auto end = begin() + size();
    for (auto it = begin(); it != end; ++it) {
      auto where = begin(new_data) + (it - begin());
      try {
        new(where) T(std::move_if_noexcept(*it));
      } catch (...) {
        std::destroy(begin(new_data), where);
        operator delete(new_data);
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
    std::destroy_n(begin(p), size(p));
    operator delete(p);
  }
};

template <typename T>
struct vector {
  using value_type = T;

  using iterator = typename buffer<T>::iterator;
  using const_iterator = typename buffer<T>::const_iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  vector() noexcept = default;

  vector(vector const& other) : data_(other.data_) {
  }

  template <typename InputIterator>
  vector(InputIterator first, InputIterator last) {
    reserve(std::distance(first, last));
    std::copy(first, last, std::back_inserter(*this));
  }

  template <typename InputIterator>
  void assign(InputIterator first, InputIterator last) {
    *this = std::move(vector(first, last));
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
    clear();
    swap(other);
    return *this;
  }

  ~vector() noexcept {
    clear();
  }

  void clear() noexcept {
    data_ = empty_t();
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
    return as_buffer().size();
  }

  size_t capacity() const noexcept {
    if (holds_nothing() || holds_value()) {
      return 1;
    }
    return as_buffer().capacity();
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
    as_buffer().detach();
    return as_buffer().begin();
  }

  const_iterator begin() const noexcept {
    if (holds_nothing()) {
      return nullptr;
    }
    if (holds_value()) {
      return std::addressof(as_value());
    }
    return as_buffer().begin();
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

  void push_back(T const& val) {
    if (holds_nothing()) {
      data_ = val;
      return;
    }
    if (holds_value()) {
      auto new_buf = buffer<T>(std::move_if_noexcept(as_value()));
      new_buf.push_back(val);
      data_ = new_buf;
      return;
    }
    as_buffer().detach();
    as_buffer().push_back(val);
  }

  void push_back(T&& val) {
    if (holds_nothing()) {
      data_ = std::move(val);
      return;
    }
    if (holds_value()) {
      auto new_buf = buffer<T>(std::move_if_noexcept(as_value()));
      new_buf.push_back(std::move(val));
      data_ = new_buf;
      return;
    }
    as_buffer().detach();
    as_buffer().push_back(std::move(val));
  }

  template <typename... Args>
  void emplace_back(Args&& ... args) {
    if (holds_nothing()) {
      data_ = T(std::forward<Args>(args)...);
      return;
    }
    if (holds_value()) {
      auto new_buf = buffer<T>(std::move_if_noexcept(as_value()));
      new_buf.emplace_back(std::forward<Args>(args)...);
      data_ = new_buf;
      return;
    }
    as_buffer().detach();
    as_buffer().emplace_back(std::forward<Args>(args)...);
  }

  void push_back(T const& val, size_t n) {
    size_t sz = size();
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
    as_buffer().detach();
    as_buffer().pop_back();
  }

  void reserve(size_t n) {
    if (n <= capacity()) {
      return;
    }
    if (holds_nothing()) {
      data_ = buffer<T>();
    }
    if (holds_value()) {
      move_to_buffer();
    }
    as_buffer().detach();
    as_buffer().reserve(n);
  }

  void shrink_to_fit() {
    if (capacity() != size()) {
      as_buffer().shrink_to_fit();
    }
  }

  iterator insert(const_iterator pos, T const& val) {
    auto pos_i = pos - begin();
    push_back(val);
    pos = begin() + pos_i;
    for (auto it = end() - 1; it != pos; --it) {
      std::iter_swap(it, it - 1);
    }
    return iterator(pos);
  }

  iterator insert(const_iterator pos, T&& val) {
    auto pos_i = pos - begin();
    push_back(std::move(val));
    pos = begin() + pos_i;
    for (auto it = end() - 1; it != pos; --it) {
      std::iter_swap(it, it - 1);
    }
    return iterator(pos);
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

  std::variant<empty_t, T, buffer<T>> data_;

  void move_to_buffer() {
    data_ = buffer<T>(as_value());
  }

  bool holds_nothing() const noexcept {
    return std::holds_alternative<empty_t>(data_);
  }

  bool holds_value() const noexcept {
    return std::holds_alternative<T>(data_);
  }

  bool holds_buffer() const noexcept {
    return std::holds_alternative<buffer<T>>(data_);
  }

  T& as_value() noexcept {
    return std::get<T>(data_);
  }

  T const& as_value() const noexcept {
    return std::get<T>(data_);
  }

  buffer<T>& as_buffer() noexcept {
    return std::get<buffer<T>>(data_);
  }

  buffer<T> const& as_buffer() const noexcept {
    return std::get<buffer<T>>(data_);
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
