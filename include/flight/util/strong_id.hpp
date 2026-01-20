#pragma once

#include <compare>
#include <cstdint>
#include <ostream>

namespace flight::util {

// Strongly-typed integer IDs to avoid mixing e.g. FlightId with OrderId.
// Trivially copyable, comparable, and streamable.
template <typename Tag>
class StrongId final {
public:
  using value_type = std::uint64_t;

  constexpr StrongId() noexcept = default;
  explicit constexpr StrongId(value_type v) noexcept : value_(v) {}

  constexpr value_type value() const noexcept { return value_; }

  // Comparisons
  friend constexpr bool operator==(StrongId, StrongId) noexcept = default;
  friend constexpr std::strong_ordering operator<=>(StrongId a, StrongId b) noexcept {
    return a.value_ <=> b.value_;
  }

private:
  value_type value_{};
};

template <typename Tag>
inline std::ostream& operator<<(std::ostream& os, StrongId<Tag> id) {
  return os << id.value();
}

} // namespace flight::util
