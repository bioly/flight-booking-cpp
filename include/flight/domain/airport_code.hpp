#pragma once

#include <stdexcept>
#include <string>

namespace flight::domain {

// Simple 3-letter IATA-like code.
class AirportCode final {
public:
  explicit AirportCode(std::string code) : code_(std::move(code)) {
    if (code_.size() != 3) {
      throw std::invalid_argument("AirportCode must be exactly 3 characters");
    }
    for (char& c : code_) {
      if (c >= 'a' && c <= 'z') {
        c = static_cast<char>(c - 'a' + 'A');
      }
      if (!(c >= 'A' && c <= 'Z')) {
        throw std::invalid_argument("AirportCode must contain only letters");
      }
    }
  }

  const std::string& value() const noexcept { return code_; }

  friend bool operator==(const AirportCode&, const AirportCode&) = default;

private:
  std::string code_;
};

} // namespace flight::domain
