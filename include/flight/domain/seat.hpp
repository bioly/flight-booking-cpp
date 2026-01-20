#pragma once

#include <compare>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace flight::domain {

// Airline seat like 12A.
class Seat final {
public:
  Seat(std::uint16_t row, char letter) : row_(row), letter_(letter) {
    if (row_ == 0) {
      throw std::invalid_argument("Seat row must be >= 1");
    }
    if (letter_ >= 'a' && letter_ <= 'z') {
      letter_ = static_cast<char>(letter_ - 'a' + 'A');
    }
    if (!(letter_ >= 'A' && letter_ <= 'Z')) {
      throw std::invalid_argument("Seat letter must be A-Z");
    }
  }

  std::uint16_t row() const noexcept { return row_; }
  char letter() const noexcept { return letter_; }

  std::string to_string() const {
    return std::to_string(row_) + std::string(1, letter_);
  }

  friend bool operator==(const Seat&, const Seat&) = default;
  friend std::strong_ordering operator<=>(const Seat& a, const Seat& b) noexcept {
    if (auto c = a.row_ <=> b.row_; c != 0) return c;
    return a.letter_ <=> b.letter_;
  }

private:
  std::uint16_t row_{};
  char letter_{};
};

} // namespace flight::domain
