#pragma once

#include <memory>

namespace opentelemetry
{
namespace nostd
{
/**
 * Provide a type-erased version of std::shared_ptr that has ABI stability.
 */
template <class T>
class shared_ptr
{
public:
  using element_type = T;
  using pointer      = element_type *;

private:
  static constexpr size_t kMaxSize   = 32;
  static constexpr size_t kAlignment = 8;

  struct alignas(kAlignment) PlacementBuffer
  {
    char data[kMaxSize];
  };

  class shared_ptr_wrapper
  {
  public:
    shared_ptr_wrapper() noexcept = default;

    shared_ptr_wrapper(std::shared_ptr<T> &&ptr) noexcept : ptr_{std::move(ptr)} {}

    virtual ~shared_ptr_wrapper() {}

    virtual void CopyTo(PlacementBuffer &buffer) const noexcept
    {
      new (buffer.data) shared_ptr_wrapper{*this};
    }

    virtual void MoveTo(PlacementBuffer &buffer) noexcept
    {
      new (buffer.data) shared_ptr_wrapper{std::move(this->ptr_)};
    }

    template <class U,
              typename std::enable_if<std::is_convertible<pointer, U *>::value>::type * = nullptr>
    void MoveTo(typename shared_ptr<U>::PlacementBuffer &buffer) noexcept
    {
      new (buffer.data) shared_ptr_wrapper{std::move(this->ptr_)};
    }

    virtual pointer Get() const noexcept { return ptr_.get(); }

    virtual void Reset() noexcept { ptr_.reset(); }

  private:
    std::shared_ptr<T> ptr_;
  };

  static_assert(sizeof(shared_ptr_wrapper) <= kMaxSize, "Placement buffer is too small");
  static_assert(alignof(shared_ptr_wrapper) <= kAlignment, "Placement buffer not properly aligned");

public:
  shared_ptr() noexcept { new (buffer_.data) shared_ptr_wrapper{}; }

  explicit shared_ptr(pointer ptr) noexcept
  {
#if __EXCEPTIONS
    try
#endif
    {
      std::shared_ptr<T> ptr_(ptr);
      new (buffer_.data) shared_ptr_wrapper{std::move(ptr_)};
    }
#if __EXCEPTIONS
    catch (const std::bad_alloc &)
    {
      new (buffer_.data) shared_ptr_wrapper{};
    }
#endif
  }

  shared_ptr(std::shared_ptr<T> ptr) noexcept
  {
    new (buffer_.data) shared_ptr_wrapper{std::move(ptr)};
  }

  shared_ptr(shared_ptr &&other) noexcept { other.wrapper().MoveTo(buffer_); }

  template <class U,
            typename std::enable_if<std::is_convertible<U *, pointer>::value>::type * = nullptr>
  shared_ptr(shared_ptr<U> &&other) noexcept
  {
    other.wrapper().template MoveTo<T>(buffer_);
  }

  shared_ptr(const shared_ptr &other) noexcept { other.wrapper().CopyTo(buffer_); }

  ~shared_ptr() { wrapper().~shared_ptr_wrapper(); }

  shared_ptr &operator=(shared_ptr &&other) noexcept
  {
    wrapper().~shared_ptr_wrapper();
    other.wrapper().MoveTo(buffer_);
    return *this;
  }

  shared_ptr &operator=(std::nullptr_t) noexcept
  {
    wrapper().Reset();
    return *this;
  }

  shared_ptr &operator=(const shared_ptr &other) noexcept
  {
    wrapper().~shared_ptr_wrapper();
    other.wrapper().CopyTo(buffer_);
    return *this;
  }

  element_type &operator*() const noexcept { return *wrapper().Get(); }

  pointer operator->() const noexcept { return wrapper().Get(); }

  operator bool() const noexcept { return wrapper().Get() != nullptr; }

  pointer get() const noexcept { return wrapper().Get(); }

  void swap(shared_ptr &other) noexcept { std::swap(buffer_, other.buffer_); }

  template <typename U>
  friend class shared_ptr;

private:
  PlacementBuffer buffer_;

  shared_ptr_wrapper &wrapper() noexcept
  {
    return *reinterpret_cast<shared_ptr_wrapper *>(buffer_.data);
  }

  const shared_ptr_wrapper &wrapper() const noexcept
  {
    return *reinterpret_cast<const shared_ptr_wrapper *>(buffer_.data);
  }
};

template <class T1, class T2>
bool operator!=(const shared_ptr<T1> &lhs, const shared_ptr<T2> &rhs) noexcept
{
  return lhs.get() != rhs.get();
}

template <class T1, class T2>
bool operator==(const shared_ptr<T1> &lhs, const shared_ptr<T2> &rhs) noexcept
{
  return lhs.get() == rhs.get();
}

template <class T>
inline bool operator==(const shared_ptr<T> &lhs, std::nullptr_t) noexcept
{
  return lhs.get() == nullptr;
}

template <class T>
inline bool operator==(std::nullptr_t, const shared_ptr<T> &rhs) noexcept
{
  return nullptr == rhs.get();
}

template <class T>
inline bool operator!=(const shared_ptr<T> &lhs, std::nullptr_t) noexcept
{
  return lhs.get() != nullptr;
}

template <class T>
inline bool operator!=(std::nullptr_t, const shared_ptr<T> &rhs) noexcept
{
  return nullptr != rhs.get();
}
}  // namespace nostd
}  // namespace opentelemetry
