//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include "common/exception.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  if (this->Size() == 0) {
    return false;
  }

  *frame_id = evictable_frames_.begin()->get()->GetFid();
  evictable_frames_.erase(evictable_frames_.begin());
  node_store_[*frame_id]->SetIsEvictable(false);
  node_store_[*frame_id]->ClearHistory();

  return true;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
  BUSTUB_ENSURE(frame_id >= 0 && frame_id < static_cast<frame_id_t>(replacer_size_), "Invalid frame_id");
  std::lock_guard<std::mutex> lock(latch_);

  if (node_store_.count(frame_id) == 0) {
    node_store_[frame_id] = std::make_shared<LRUKNode>(frame_id, k_);
  } else if (node_store_[frame_id]->IsEvictable()) {
    evictable_frames_.erase(node_store_[frame_id]);
  }

  node_store_[frame_id]->AddHistoryEntry(current_timestamp_++);
  if (node_store_[frame_id]->IsEvictable()) {
    evictable_frames_.insert(node_store_[frame_id]);
  }
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  BUSTUB_ENSURE(frame_id >= 0 && frame_id < static_cast<frame_id_t>(replacer_size_), "Invalid frame_id");
  std::lock_guard<std::mutex> lock(latch_);

  if (node_store_.count(frame_id) != 0) {
    bool is_evictable = node_store_[frame_id]->IsEvictable();
    node_store_[frame_id]->SetIsEvictable(set_evictable);

    if (is_evictable && !set_evictable) {
      evictable_frames_.erase(node_store_[frame_id]);
    } else if (!is_evictable && set_evictable) {
      evictable_frames_.insert(node_store_[frame_id]);
    }
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::lock_guard<std::mutex> lock(latch_);
  BUSTUB_ENSURE(frame_id >= 0 && frame_id < static_cast<frame_id_t>(replacer_size_), "Invalid frame_id");

  if (node_store_.count(frame_id) != 0) {
    BUSTUB_ENSURE(!node_store_[frame_id]->IsEvictable(), "Remove is called on a non-evictable frame");
    evictable_frames_.erase(node_store_[frame_id]);
  }
}

auto LRUKReplacer::Size() -> size_t { return evictable_frames_.size(); }

}  // namespace bustub
