#include "flight/infrastructure/in_memory_reservation_repository.hpp"

#include <mutex>

namespace flight::infrastructure {

void InMemoryReservationRepository::add(flight::domain::Reservation reservation) {
  std::unique_lock lk(mu_);
  reservations_.insert_or_assign(reservation.id().value(), std::move(reservation));
}

std::optional<flight::domain::Reservation> InMemoryReservationRepository::get(
    flight::domain::ReservationId id) const {
  std::shared_lock lk(mu_);
  auto it = reservations_.find(id.value());
  if (it == reservations_.end()) return std::nullopt;
  return it->second;
}

std::vector<flight::domain::Reservation> InMemoryReservationRepository::list_by_order(
    flight::domain::OrderId order_id) const {
  std::shared_lock lk(mu_);
  std::vector<flight::domain::Reservation> out;
  for (const auto& [_, r] : reservations_) {
    if (r.order_id() == order_id) {
      out.push_back(r);
    }
  }
  return out;
}

} // namespace flight::infrastructure
