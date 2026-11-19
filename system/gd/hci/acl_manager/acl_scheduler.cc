/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hci/acl_manager/acl_scheduler.h"

#include <bluetooth/log.h>
#include <com_android_bluetooth_flags.h>

#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>
#include <utility>
#include <variant>

#include "common/bind.h"
#include "os/alarm.h"

namespace bluetooth {
namespace hci {

namespace acl_manager {

struct AclCreateConnectionQueueEntry {
  Address address;
  common::ContextualOnceCallback<void()> callback;
};

struct RemoteNameRequestQueueEntry {
  Address address;
  common::ContextualOnceCallback<void()> callback;
  common::ContextualOnceCallback<void()> callback_when_cancelled;
};

using QueueEntry = std::variant<AclCreateConnectionQueueEntry, RemoteNameRequestQueueEntry>;

// How long an incoming accept waits for a colliding outgoing Create Connection to resolve on its
// own before the outgoing is cancelled. Well below the controller's Connection Accept Timeout.
constexpr std::chrono::milliseconds kCollisionGracePeriod(500);

struct AclScheduler::impl {
  impl(os::Handler* handler) : handler_(handler) {}

  void EnqueueOutgoingAclConnection(Address address,
                                    common::ContextualOnceCallback<void()> start_connection) {
    pending_outgoing_operations_.push_back(
            AclCreateConnectionQueueEntry{address, std::move(start_connection)});
    try_dequeue_next_operation();
  }

  void RegisterPendingIncomingConnection(Address address) {
    incoming_connecting_address_set_.insert(address);
  }

  void ReportAclConnectionCompletion(
          Address address, common::ContextualOnceCallback<void()> handle_outgoing_connection,
          common::ContextualOnceCallback<void()> handle_incoming_connection,
          common::ContextualOnceCallback<void(std::string)> handle_unknown_connection,
          bool succeeded) {
    // Check if an outgoing request (a) exists, (b) is a Create Connection, (c) matches the received
    // address
    if (outgoing_entry_.has_value()) {
      auto entry = std::get_if<AclCreateConnectionQueueEntry>(&outgoing_entry_.value());
      if (entry != nullptr && entry->address == address) {
        // If so, clear the current entry and advance the queue
        outgoing_entry_.reset();
        // If we cancelled this outgoing on purpose to let a colliding incoming connection win (see
        // OnDeferredIncomingExpired), the controller emits a separate Connection Complete for the
        // incoming, so the pending incoming survives a failed completion here. A successful one
        // is matched to the outgoing like any other, and the pending incoming is dropped.
        bool cancelled_for_incoming = cancelled_outgoing_address_set_.erase(address) != 0;
        handle_outgoing_connection();
        auto deferred = deferred_incoming_.find(address);
        if (deferred != deferred_incoming_.end()) {
          // The incoming request from the same peer was held back until this outgoing resolved.
          DeferredIncoming incoming = std::move(deferred->second);
          deferred_incoming_.erase(deferred);
          if (succeeded) {
            log::info("Outgoing connection to {} succeeded, rejecting colliding incoming", address);
            incoming_connecting_address_set_.erase(address);
            incoming.reject.Invoke();
          } else {
            log::info("Outgoing connection to {} failed, accepting colliding incoming", address);
            incoming.accept.Invoke();
          }
        } else if (incoming_connecting_address_set_.find(address) !=
                   incoming_connecting_address_set_.end()) {
          if (!cancelled_for_incoming || succeeded) {
            log::warn("Incoming connection request also exists for {}", address);
            incoming_connecting_address_set_.erase(address);
          }
        }
        try_dequeue_next_operation();
        return;
      }
    }

    // Otherwise check if it's an incoming request and advance the queue if so
    if (incoming_connecting_address_set_.find(address) != incoming_connecting_address_set_.end()) {
      incoming_connecting_address_set_.erase(address);
      handle_incoming_connection();
    } else {
      handle_unknown_connection(set_of_incoming_connecting_addresses());
    }
    try_dequeue_next_operation();
  }

  void ReportOutgoingAclConnectionFailure() {
    if (!outgoing_entry_.has_value()) {
      log::error("Outgoing connection failure reported, but none present!");
      return;
    }
    auto entry = std::get_if<AclCreateConnectionQueueEntry>(&outgoing_entry_.value());
    if (entry == nullptr) {
      log::error("Outgoing connection failure reported, but we're currently doing an RNR!");
      return;
    }
    // The Create Connection never started at the controller, so there is no collision left to wait
    // for: release an accept that was held back for this peer.
    auto deferred = deferred_incoming_.find(entry->address);
    if (deferred != deferred_incoming_.end()) {
      DeferredIncoming incoming = std::move(deferred->second);
      deferred_incoming_.erase(deferred);
      log::info("Outgoing connection to {} failed to start, accepting incoming", entry->address);
      incoming.accept.Invoke();
    }
    cancelled_outgoing_address_set_.erase(entry->address);
    outgoing_entry_.reset();
    try_dequeue_next_operation();
  }

  void CancelAclConnection(Address address,
                           common::ContextualOnceCallback<void()> cancel_connection,
                           common::ContextualOnceCallback<void()> cancel_connection_completed) {
    auto ok = cancel_outgoing_or_queued_connection(
            [&](auto& entry) {
              auto entry_ptr = std::get_if<AclCreateConnectionQueueEntry>(&entry);
              return entry_ptr != nullptr && entry_ptr->address == address;
            },
            [&]() { cancel_connection(); },
            [&](auto /* entry */) { cancel_connection_completed(); });
    if (ok) {
      // An accept held back for this peer was waiting on the outgoing connection we just asked to
      // cancel, so the collision is resolved: release it right behind the Create Connection Cancel
      // (both go through the same handler, in order) and keep the fallback timer from sending a
      // second Cancel.
      auto deferred = deferred_incoming_.find(address);
      if (deferred != deferred_incoming_.end()) {
        DeferredIncoming incoming = std::move(deferred->second);
        deferred_incoming_.erase(deferred);
        log::info("Outgoing connection to {} cancelled by the user, accepting incoming", address);
        cancelled_outgoing_address_set_.insert(address);
        incoming.accept.Invoke();
      }
      return;
    }
    // No outgoing or queued Create Connection matches. If the address is only registered as a
    // pending incoming connection that will never complete (for instance the caller gave up on
    // it), clear it here - otherwise it blocks every future outgoing operation (ACL connections
    // *and* Remote Name Requests, to any peer) until AclScheduler::Stop() runs.
    if (incoming_connecting_address_set_.erase(address) != 0) {
      log::info("Cleared pending incoming connection to {} after failed accept", address);
      try_dequeue_next_operation();
      return;
    }
    log::error("Attempted to cancel connection to {} that does not exist", address);
  }

  void HandleIncomingConnectionRequest(Address address,
                                       common::ContextualOnceCallback<void()> accept,
                                       common::ContextualOnceCallback<void()> reject,
                                       common::ContextualOnceCallback<void()> cancel_outgoing) {
    if (stopped_) {
      log::warn("Scheduler stopped, accepting incoming connection from {} without scheduling",
                address);
      accept.Invoke();
      return;
    }
    // A Create Connection to this address is already at the controller: accepting now would collide
    // with our own page. Hold the accept back until that outgoing resolves.
    if (outgoing_entry_.has_value()) {
      auto* entry = std::get_if<AclCreateConnectionQueueEntry>(&outgoing_entry_.value());
      if (entry != nullptr && entry->address == address) {
        log::info("Incoming connection from {} collides with outgoing, deferring accept", address);
        uint64_t id = ++next_deferred_id_;
        // The fallback timer lives in the record, so dropping the record (resolved, cancelled or
        // Stop()) also drops the timer.
        auto& deferred =
                deferred_incoming_
                        .insert_or_assign(address,
                                          DeferredIncoming{id, std::move(accept), std::move(reject),
                                                           std::move(cancel_outgoing), nullptr})
                        .first->second;
        deferred.alarm = std::make_unique<os::Alarm>(&handler_->thread(), /* isWakeAlarm */ false);
        deferred.alarm->Schedule(common::BindOnce(&impl::OnDeferredAlarmFired,
                                                  common::Unretained(this), address, id),
                                 kCollisionGracePeriod);
        return;
      }
    }
    // Otherwise drop any queued (not-yet-started) outgoing Create Connection to this address. Such
    // an entry never reached the controller, so there is no Connection Complete to wait for.
    auto it = std::find_if(pending_outgoing_operations_.begin(), pending_outgoing_operations_.end(),
                           [&](const QueueEntry& e) {
                             auto* p = std::get_if<AclCreateConnectionQueueEntry>(&e);
                             return p != nullptr && p->address == address;
                           });
    if (it != pending_outgoing_operations_.end()) {
      log::info("Dropping queued outgoing connection to {} to accept incoming", address);
      pending_outgoing_operations_.erase(it);
    }
    accept.Invoke();
  }

  // Runs on the alarm's reactor. Go through the handler queue so that anything already queued (such
  // as the Connection Complete that would resolve the collision) is handled first.
  void OnDeferredAlarmFired(Address address, uint64_t id) {
    handler_->CallOn(this, &impl::OnDeferredIncomingExpired, address, id);
  }

  // The outgoing Create Connection did not resolve the collision by itself within the grace period
  // (the controller is stuck paging and accepting the same peer), so cancel it and accept.
  void OnDeferredIncomingExpired(Address address, uint64_t id) {
    auto it = deferred_incoming_.find(address);
    if (it == deferred_incoming_.end() || it->second.id != id) {
      return;
    }
    DeferredIncoming incoming = std::move(it->second);
    deferred_incoming_.erase(it);
    log::info("Outgoing connection to {} still pending, cancelling it to accept incoming", address);
    cancelled_outgoing_address_set_.insert(address);
    incoming.cancel_outgoing.Invoke();
    incoming.accept.Invoke();
  }

  void ReportIncomingAcceptFailure(Address address) {
    if (incoming_connecting_address_set_.erase(address) != 0) {
      log::info("Cleared pending incoming connection to {} after failed accept", address);
      try_dequeue_next_operation();
    }
  }

  void EnqueueRemoteNameRequest(Address address,
                                common::ContextualOnceCallback<void()> start_request,
                                common::ContextualOnceCallback<void()> cancel_request_completed) {
    if (stopped_) {
      log::warn("Scheduler stopped, not enqueuing RemoteNameRequest.");
      return;
    }
    if ((incoming_connecting_address_set_.find(address) !=
         incoming_connecting_address_set_.end()) &&
        !outgoing_entry_.has_value()) {
      log::warn("send RemoteNameRequest when incoming acl accepted already");
      pending_outgoing_operations_.push_front(RemoteNameRequestQueueEntry{
              address, std::move(start_request), std::move(cancel_request_completed)});
      auto entry = std::move(pending_outgoing_operations_.front());
      pending_outgoing_operations_.pop_front();
      std::visit([](auto&& variant) { variant.callback.Invoke(); }, entry);
      outgoing_entry_ = std::move(entry);

    } else {
      pending_outgoing_operations_.push_back(RemoteNameRequestQueueEntry{
              address, std::move(start_request), std::move(cancel_request_completed)});
      try_dequeue_next_operation();
    }
  }

  void ReportRemoteNameRequestCompletion(Address /* address */) {
    if (!outgoing_entry_.has_value()) {
      log::error("Remote name request completion reported, but none taking place!");
      return;
    }

    std::visit(
            [](auto&& entry) {
              using T = std::decay_t<decltype(entry)>;
              if constexpr (std::is_same_v<T, RemoteNameRequestQueueEntry>) {
                log::info("Remote name request completed");
              } else if constexpr (std::is_same_v<T, AclCreateConnectionQueueEntry>) {
                log::error(
                        "Received RNR completion when ACL connection is outstanding - assuming the "
                        "connection has failed and continuing");
              } else {
                static_assert(!sizeof(T*), "non-exhaustive visitor!");
              }
            },
            outgoing_entry_.value());

    outgoing_entry_.reset();
    try_dequeue_next_operation();
  }

  void CancelRemoteNameRequest(Address address,
                               common::ContextualOnceCallback<void()> cancel_request) {
    auto ok = cancel_outgoing_or_queued_connection(
            [&](auto& entry) {
              auto entry_ptr = std::get_if<RemoteNameRequestQueueEntry>(&entry);
              return entry_ptr != nullptr && entry_ptr->address == address;
            },
            [&]() { cancel_request(); },
            [](auto entry) {
              std::get<RemoteNameRequestQueueEntry>(entry).callback_when_cancelled();
            });
    if (!ok) {
      log::error("Attempted to cancel remote name request to {} that does not exist", address);
    }
  }

  void Stop() {
    stopped_ = true;
    pending_outgoing_operations_.clear();
    outgoing_entry_.reset();
    incoming_connecting_address_set_.clear();
    cancelled_outgoing_address_set_.clear();
    deferred_incoming_.clear();
    log::info("AclScheduler stopped: state cleared.");
  }

private:
  bool ready_to_send_next_operation() const {
    if (stopped_) {
      return false;
    }
    if (pending_outgoing_operations_.empty()) {
      return false;
    }
    if (const RemoteNameRequestQueueEntry* peek =
                std::get_if<RemoteNameRequestQueueEntry>(&pending_outgoing_operations_.front())) {
      if (incoming_connecting_address_set_.contains(peek->address) &&
          !outgoing_entry_.has_value()) {
        log::info("Pending incoming connection and outgoing RNR to same peer:{}", peek->address);
        return true;
      }
    }
    return incoming_connecting_address_set_.empty() && !outgoing_entry_.has_value();
  }

  std::stringstream log_queue_entry(const QueueEntry& entry) {
    std::stringstream ss;
    if (const RemoteNameRequestQueueEntry* peek =
                std::get_if<RemoteNameRequestQueueEntry>(&entry)) {
      ss << "RNR to " << peek->address.ToRedactedStringForLogging();
    } else if (const AclCreateConnectionQueueEntry* peek =
                       std::get_if<AclCreateConnectionQueueEntry>(&entry)) {
      ss << "ACL connection to " << peek->address.ToRedactedStringForLogging();
    } else {
      ss << "Unknown entry type";
    }
    return ss;
  }

  inline void log_try_dequeue_next_operation() {
    log::info(
            "Could not send next operation postponed to next iteration, stopped: {}, "
            "pending_outgoing_operations_ is_empty: "
            "{}, outgoing_entry_ has_value: {}, incoming_connecting_address_set_ is_empty: {}",
            stopped_, pending_outgoing_operations_.empty(), outgoing_entry_.has_value(),
            incoming_connecting_address_set_.empty());


    // log the contents of the pending_outgoing_operations_
    if (!pending_outgoing_operations_.empty()) {
      std::stringstream log_message;
      log_message << "Pending Outgoing Operations: ";
      for (const auto& entry : pending_outgoing_operations_) {
        log_message << log_queue_entry(entry).str() << ", ";
      }
      log::info("{}", log_message.str());
    }


    // Aggregate contents of the incoming_connecting_address_set_
    if (!incoming_connecting_address_set_.empty()) {
      std::stringstream log_message;
      log_message << "Incoming Connections from: ";
      for (const auto& address : incoming_connecting_address_set_) {
        log_message << address.ToRedactedStringForLogging() << ", ";
      }
      log::info("{}", log_message.str());
    }


    // Aggregate contents of the outgoing_entry_
    if (outgoing_entry_.has_value()) {
      log::info("Current Outgoing Entry: {}", log_queue_entry(outgoing_entry_.value()).str());
    }
  }

  void try_dequeue_next_operation() {
    log::info("Enter, outgoing_entry_.has_value() : {}", outgoing_entry_.has_value());
    if (stopped_) {
      log::warn("Scheduler stopped, skipping callback invocation.");
      return;
    }
    if (ready_to_send_next_operation()) {
      log::info("Pending connections is not empty; so sending next connection");
      auto entry = std::move(pending_outgoing_operations_.front());
      pending_outgoing_operations_.pop_front();
      outgoing_entry_ = std::move(entry);
      std::visit([](auto& v) {
        auto cb = std::move(v.callback);
        cb.Invoke();
      }, outgoing_entry_.value());
      log::info("Exit, outgoing_entry_.has_value() : {}", outgoing_entry_.has_value());
      return;
    }

    if (!pending_outgoing_operations_.empty()) {
      if (const auto* peek =
              std::get_if<RemoteNameRequestQueueEntry>(&pending_outgoing_operations_.front())) {
        if (incoming_connecting_address_set_.contains(peek->address) && !outgoing_entry_.has_value()) {
          log::info("Front is eligible RNR; dispatching");
          auto entry = std::move(pending_outgoing_operations_.front());
          pending_outgoing_operations_.pop_front();
          outgoing_entry_ = std::move(entry);
          std::visit([](auto& v) {
            auto cb = std::move(v.callback);
            cb.Invoke();
          }, outgoing_entry_.value());
          log::info("Exit, outgoing_entry_.has_value() : {}", outgoing_entry_.has_value());
          return;
        }
      }

      if (!outgoing_entry_.has_value() && !incoming_connecting_address_set_.empty()) {
        auto it = std::find_if(
            pending_outgoing_operations_.begin(), pending_outgoing_operations_.end(),
            [this](const QueueEntry& e) {
              if (auto* rnr = std::get_if<RemoteNameRequestQueueEntry>(&e)) {
                return incoming_connecting_address_set_.count(rnr->address) > 0;
              }
              return false;
            });
        if (it != pending_outgoing_operations_.end()) {
          auto entry = std::move(*it);
          pending_outgoing_operations_.erase(it);
          outgoing_entry_ = std::move(entry);
          std::visit([](auto& v) {
            auto cb = std::move(v.callback);
            cb.Invoke();
          }, outgoing_entry_.value());
          log::info("Promoted and dispatched eligible RNR from queue.");
          log::info("Exit, outgoing_entry_.has_value() : {}", outgoing_entry_.has_value());
          return;
        }
      }
    }

    log_try_dequeue_next_operation();
    log::info("Exit, outgoing_entry_.has_value() : {}", outgoing_entry_.has_value());
  }

  template <typename T, typename U, typename V>
  bool cancel_outgoing_or_queued_connection(T matcher, U cancel_outgoing, V cancelled_queued) {
    // Check if relevant connection is currently outgoing
    if (outgoing_entry_.has_value()) {
      if (matcher(outgoing_entry_.value())) {
        cancel_outgoing();
        return true;
      }
    }
    // Otherwise, clear from the queue
    auto it = std::find_if(pending_outgoing_operations_.begin(), pending_outgoing_operations_.end(),
                           matcher);
    if (it == pending_outgoing_operations_.end()) {
      return false;
    }
    cancelled_queued(std::move(*it));
    pending_outgoing_operations_.erase(it);
    return true;
  }

  const std::string set_of_incoming_connecting_addresses() const {
    std::stringstream buffer;
    for (const auto& c : incoming_connecting_address_set_) {
      buffer << " " << c.ToRedactedStringForLogging();
    }
    return buffer.str();
  }

  std::optional<QueueEntry> outgoing_entry_;
  std::deque<QueueEntry> pending_outgoing_operations_;
  std::unordered_set<Address> incoming_connecting_address_set_;
  // Incoming connection requests whose accept is held back because an outgoing Create Connection to
  // the same peer is at the controller (see HandleIncomingConnectionRequest).
  struct DeferredIncoming {
    uint64_t id;
    common::ContextualOnceCallback<void()> accept;
    common::ContextualOnceCallback<void()> reject;
    common::ContextualOnceCallback<void()> cancel_outgoing;
    std::unique_ptr<os::Alarm> alarm;
  };
  std::map<Address, DeferredIncoming> deferred_incoming_;
  uint64_t next_deferred_id_ = 0;
  // Addresses whose outstanding outgoing Create Connection we cancelled on purpose to resolve a
  // page/accept collision in favor of an incoming connection. Used to avoid erasing the pending
  // incoming when the cancelled outgoing's failed completion arrives (a successful one means our
  // connection won after all, and the pending incoming is dropped).
  std::unordered_set<Address> cancelled_outgoing_address_set_;
  bool stopped_ = false;

public:
  os::Handler* handler_;
};

AclScheduler::AclScheduler(os::Handler* handler) : pimpl_(std::make_unique<impl>(handler)) {
  log::verbose("module started !!");
}

AclScheduler::~AclScheduler() {
  pimpl_->Stop();
  pimpl_.reset();
  log::verbose("module stopped !!");
}

void AclScheduler::EnqueueOutgoingAclConnection(
        Address address, common::ContextualOnceCallback<void()> start_connection) {
  pimpl_->handler_->Call(&impl::EnqueueOutgoingAclConnection, common::Unretained(pimpl_.get()),
                         address, std::move(start_connection));
}

void AclScheduler::RegisterPendingIncomingConnection(Address address) {
  pimpl_->handler_->Call(&impl::RegisterPendingIncomingConnection, common::Unretained(pimpl_.get()),
                         address);
}

void AclScheduler::ReportAclConnectionCompletion(
        Address address, common::ContextualOnceCallback<void()> handle_outgoing_connection,
        common::ContextualOnceCallback<void()> handle_incoming_connection,
        common::ContextualOnceCallback<void(std::string)> handle_unknown_connection,
        bool succeeded) {
  pimpl_->handler_->Call(&impl::ReportAclConnectionCompletion, common::Unretained(pimpl_.get()),
                         address, std::move(handle_outgoing_connection),
                         std::move(handle_incoming_connection),
                         std::move(handle_unknown_connection), succeeded);
}

void AclScheduler::ReportOutgoingAclConnectionFailure() {
  pimpl_->handler_->Call(&impl::ReportOutgoingAclConnectionFailure,
                         common::Unretained(pimpl_.get()));
}

void AclScheduler::CancelAclConnection(
        Address address, common::ContextualOnceCallback<void()> cancel_connection,
        common::ContextualOnceCallback<void()> cancel_connection_completed) {
  pimpl_->handler_->Call(&impl::CancelAclConnection, common::Unretained(pimpl_.get()), address,
                         std::move(cancel_connection), std::move(cancel_connection_completed));
}

void AclScheduler::HandleIncomingConnectionRequest(
        Address address, common::ContextualOnceCallback<void()> accept,
        common::ContextualOnceCallback<void()> reject,
        common::ContextualOnceCallback<void()> cancel_outgoing) {
  pimpl_->handler_->Call(&impl::HandleIncomingConnectionRequest, common::Unretained(pimpl_.get()),
                         address, std::move(accept), std::move(reject), std::move(cancel_outgoing));
}

void AclScheduler::ReportIncomingAcceptFailure(Address address) {
  pimpl_->handler_->Call(&impl::ReportIncomingAcceptFailure, common::Unretained(pimpl_.get()),
                         address);
}

void AclScheduler::EnqueueRemoteNameRequest(
        Address address, common::ContextualOnceCallback<void()> start_request,
        common::ContextualOnceCallback<void()> cancel_request_completed) {
  pimpl_->handler_->Call(&impl::EnqueueRemoteNameRequest, common::Unretained(pimpl_.get()), address,
                         std::move(start_request), std::move(cancel_request_completed));
}

void AclScheduler::ReportRemoteNameRequestCompletion(Address address) {
  pimpl_->handler_->Call(&impl::ReportRemoteNameRequestCompletion, common::Unretained(pimpl_.get()),
                         address);
}

void AclScheduler::CancelRemoteNameRequest(Address address,
                                           common::ContextualOnceCallback<void()> cancel_request) {
  pimpl_->handler_->Call(&impl::CancelRemoteNameRequest, common::Unretained(pimpl_.get()), address,
                         std::move(cancel_request));
}

}  // namespace acl_manager
}  // namespace hci
}  // namespace bluetooth
