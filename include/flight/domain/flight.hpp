#pragma once

#include "flight/domain/airport_code.hpp"
#include "flight/domain/ids.hpp"
#include "flight/domain/seat.hpp"

#include <chrono>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>

namespace flight::domain {

class Flight final {
public:
  using time_point = std::chrono::system_clock::time_point;

  Flight(FlightId id,
         AirportCode origin,
         AirportCode destination,
         time_point departure,
         std::uint16_t rows,
         std::uint8_t seats_per_row)
      : id_(id), origin_(std::move(origin)), destination_(std::move(destination)), departure_(departure),
        rows_(rows), seats_per_row_(seats_per_row) {
    if (rows_ == 0 || seats_per_row_ == 0) {
      throw std::invalid_argument("rows and seats_per_row must be > 0");
    }
    if (seats_per_row_ > 26) {
      throw std::invalid_argument("seats_per_row must be <= 26 (A-Z)");
    }
  }

  FlightId id() const noexcept { return id_; }
  const AirportCode& origin() const noexcept { return origin_; }
  const AirportCode& destination() const noexcept { return destination_; }
  time_point departure() const noexcept { return departure_; }

  std::uint16_t rows() const noexcept { return rows_; }
  std::uint8_t seats_per_row() const noexcept { return seats_per_row_; }
  std::uint32_t capacity() const noexcept { return static_cast<std::uint32_t>(rows_) * seats_per_row_; }

  bool is_seat_valid(const Seat& seat) const noexcept {
    if (seat.row() < 1 || seat.row() > rows_) return false;
    const auto max_letter = static_cast<char>('A' + seats_per_row_ - 1);
    return seat.letter() >= 'A' && seat.letter() <= max_letter;
  }

  // NOTE: Flight itself is NOT thread-safe. Concurrency is handled at repository/service layer.
  bool is_booked(const Seat& seat) const {
    return booked_seats_.contains(seat);
  }

  void book_seat(const Seat& seat) {
    if (!is_seat_valid(seat)) {
      throw std::invalid_argument("Seat is not valid for this flight");
    }
    auto [it, inserted] = booked_seats_.insert(seat);
    if (!inserted) {
      throw std::runtime_error("Seat already booked");
    }
  }

  void release_seat(const Seat& seat) {
    booked_seats_.erase(seat);
  }

private:
  FlightId id_;
  AirportCode origin_;
  AirportCode destination_;
  time_point departure_;
  std::uint16_t rows_;
  std::uint8_t seats_per_row_;

  std::set<Seat> booked_seats_;
};

} // namespace flight::domain
