//  Copyright 2019 Nikita Golikov

#ifndef LIST_H
#define LIST_H

#include <iterator>
#include <utility>
#include <optional>
#include <cassert>

template <bool>
struct enable_if {
};

template <>
struct enable_if<true> {
  using type = void;
};

template <typename T>
struct list {
 private:
  template <typename U>
  struct typed_iterator;
 public:
  using iterator = typed_iterator<T>;

  using const_iterator = typed_iterator<T const>;

  using reverse_iterator = std::reverse_iterator<iterator>;

  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  list() noexcept = default;

  list(list const& other) : list() {
    for (auto const& x : other) {
      push_back(x);
    }
  }

  list& operator=(list other) {
    swap(other);
    return *this;
  }

  ~list() noexcept {
    clear();
  }

  list(list&& other) noexcept {
    swap(other);
  }

  list& operator=(list&& other) noexcept {
    clear();
    swap(other);
    return *this;
  }

  void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
  }

  bool empty() const noexcept {
    return begin() == end();
  }

  template <typename... Args>
  void emplace_back(Args&& ... args) {
    insert(end(), std::forward<Args>(args)...);
  }

  template <typename S>
  std::enable_if_t<std::is_convertible_v<S, T>> push_back(S&& val) {
    emplace_back(std::forward<S>(val));
  }

  void pop_back() noexcept {
    assert(!empty());
    erase(std::prev(end()));
  }

  T& back() noexcept {
    assert(!empty());
    return *std::prev(end());
  }

  T const& back() const noexcept {
    assert(!empty());
    return *std::prev(end());
  }

  template <typename... Args>
  void emplace_front(Args&& ... args) {
    insert(begin(), std::forward<Args>(args)...);
  }

  template <typename S>
  std::enable_if_t<std::is_convertible_v<S, T>> push_front(S&& val) {
    emplace_front(std::forward<S>(val));
  }

  void pop_front() {
    assert(!empty());
    erase(begin());
  }

  T& front() noexcept {
    assert(!empty());
    return *begin();
  }

  T const& front() const noexcept {
    assert(!empty());
    return *begin();
  }

  iterator begin() noexcept {
    return iterator(end_.next_);
  }

  const_iterator begin() const noexcept {
    return const_iterator(end_.next_);
  }

  reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  iterator end() noexcept {
    return iterator(&end_);
  }

  const_iterator end() const noexcept {
    return const_iterator(const_cast<node*>(&end_));
  }

  reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  template <typename... Args>
  iterator emplace(const_iterator pos, Args&& ... args) {
    node* n = new node(pos.n_->prev_, pos.n_, std::forward<Args>(args)...);
    n->prev_->next_ = n;
    n->next_->prev_ = n;
    return iterator(n);
  }

  template <typename S>
  std::enable_if_t<std::is_convertible_v<S, T>, iterator>
  insert(const_iterator pos, S&& val) {
    return emplace(pos, std::forward<S>(val));
  }

  iterator erase(const_iterator pos) noexcept {
    assert(pos != end());
    node* next = std::next(pos).n_;
    node* prev = std::prev(pos).n_;
    prev->next_ = next;
    next->prev_ = prev;
    delete pos.n_;
    return iterator(next);
  }

  void splice(const_iterator pos, list&, const_iterator first,
              const_iterator last) noexcept {
    node* old_first_prev = first.n_->prev_;
    pos.n_->prev_->next_ = first.n_;
    first.n_->prev_ = pos.n_->prev_;
    old_first_prev->next_ = last.n_;
    last.n_->prev_->next_ = pos.n_;
    pos.n_->prev_ = last.n_->prev_;
    last.n_->prev_ = old_first_prev;
  }

  void swap(list& other) noexcept {
    list tmp;
    tmp.splice(tmp.begin(), other, other.begin(), other.end());
    other.splice(other.begin(), *this, begin(), end());
    splice(begin(), tmp, tmp.begin(), tmp.end());
  }

  friend void swap(list& lhs, list& rhs) noexcept {
    lhs.swap(rhs);
  }

 private:
  struct node {
    std::optional<T> value_;
    node* prev_;
    node* next_;

    node() noexcept : prev_(this), next_(prev_) {

    }

    template <typename... Args>
    node(node* prev, node* next, Args&& ... args)
            : value_(std::forward<Args>(args)...), prev_(prev), next_(next) {
    }
  } end_;

};

template <typename T>
template <typename U>
struct list<T>::typed_iterator {
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = U;
  using difference_type = ptrdiff_t;
  using pointer = U*;
  using reference = U&;
  using const_reference = U const&;

  typed_iterator() noexcept = default;

  typed_iterator(typed_iterator const& other) noexcept = default;

  typed_iterator& operator=(typed_iterator const& other) noexcept = default;

  friend bool
  operator==(typed_iterator const& lhs, typed_iterator const& rhs) noexcept {
    return lhs.n_ == rhs.n_;
  }

  friend bool
  operator!=(typed_iterator const& lhs, typed_iterator const& rhs) noexcept {
    return !(lhs == rhs);
  }

  operator typed_iterator<U const>() const noexcept {
    return typed_iterator<U const>(n_);
  }

  typed_iterator& operator++() noexcept {
    n_ = n_->next_;
    return *this;
  }

  typed_iterator operator++(int) noexcept {
    auto result = *this;
    ++*this;
    return result;
  }

  typed_iterator& operator--() noexcept {
    n_ = n_->prev_;
    return *this;
  }

  typed_iterator operator--(int) noexcept {
    auto result = *this;
    --*this;
    return result;
  }

  reference operator*() const noexcept {
    return *n_->value_;
  }

  pointer operator->() const noexcept {
    return n_->value_.operator->();
  }

 private:

  friend struct list;

  explicit typed_iterator(node* n) noexcept : n_(n) {

  }

  node* n_;
};

#endif //LIST_H
