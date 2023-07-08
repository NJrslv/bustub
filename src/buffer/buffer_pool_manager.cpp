//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"

#include "common/exception.h"
#include "common/macros.h"
#include "storage/page/page_guard.h"

namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), log_manager_(log_manager) {
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  std::lock_guard<std::mutex> lock(latch_);
  auto page = GetAvailablePage(AllocatePage(), AccessType::Unknown);
  if (page != nullptr) {
    *page_id = page->page_id_;
  }
  return page;
}

auto BufferPoolManager::GetAvailablePage(page_id_t page_id, [[maybe_unused]] AccessType access_type) -> Page * {
  frame_id_t frame_id;
  Page *page;
  if (!free_list_.empty()) { /* page from the free_list_ */
    frame_id = free_list_.front();
    free_list_.pop_front();
    page = &pages_[frame_id];
  } else { /* evicted page */
    if (!replacer_->Evict(&frame_id)) {
      return nullptr;
    }

    page = &pages_[frame_id];
    if (page->is_dirty_) {
      disk_manager_->WritePage(page->page_id_, page->GetData());
      page->is_dirty_ = false;
    }

    page->ResetMemory();
    page_table_.erase(page->page_id_);
  }
  // Initializing new page
  page_table_[page_id] = frame_id;
  page->pin_count_ = 1;
  page->page_id_ = page_id;
  replacer_->RecordAccess(frame_id, access_type);
  return page;
}

auto BufferPoolManager::FetchPage(page_id_t page_id, [[maybe_unused]] AccessType access_type) -> Page * {
  std::lock_guard<std::mutex> lock(latch_);
  if (page_id == INVALID_PAGE_ID) {
    return nullptr;
  }

  Page *page;
  if (page_table_.count(page_id) != 0) {
    auto frame_id = page_table_[page_id];
    replacer_->RecordAccess(frame_id);
    replacer_->SetEvictable(frame_id, false);
    page = &pages_[frame_id];
    page->pin_count_++;
  } else {
    page = GetAvailablePage(page_id, access_type);
    if (page != nullptr) {
      disk_manager_->ReadPage(page_id, page->GetData());
    }
  }
  return page;
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty, [[maybe_unused]] AccessType access_type) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  if (page_table_.count(page_id) != 0) {
    auto frame_id = page_table_[page_id];
    auto page = &pages_[frame_id];
    page->is_dirty_ = page->is_dirty_ || is_dirty;
    if (page->pin_count_ <= 0) {
      return false;
    }

    page->pin_count_--;
    if (page->pin_count_ == 0) {
      replacer_->SetEvictable(frame_id, true);
    }
    return true;
  }
  return false;
}

auto BufferPoolManager::FlushPageCommon(page_id_t page_id) -> bool {
  if (page_table_.count(page_id) != 0) {
    auto page = &pages_[page_table_[page_id]];
    disk_manager_->WritePage(page_id, page->GetData());
    page->is_dirty_ = false;
    return true;
  }
  return false;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  return FlushPageCommon(page_id);
}

void BufferPoolManager::FlushAllPages() {
  std::lock_guard<std::mutex> lock(latch_);
  for (size_t fid_it = 0; fid_it < pool_size_; ++fid_it) {
    FlushPageCommon(pages_[fid_it].page_id_);
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  if (page_table_.count(page_id) != 0) {
    auto frame_id = page_table_[page_id];
    auto page = &pages_[frame_id];
    if (page->pin_count_ > 0) {
      return false;
    }
    replacer_->Remove(frame_id);
    free_list_.emplace_back(frame_id);
    page_table_.erase(page_id);
    page->ResetMemory();
    page->pin_count_ = 0;
    page->is_dirty_ = false;
    page_id = INVALID_PAGE_ID;
    DeallocatePage(page_id);
  }
  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t { return next_page_id_++; }

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard { return {this, FetchPage(page_id)}; }

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  auto page = FetchPage(page_id);
  page->RLatch();
  return {this, page};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  auto page = FetchPage(page_id);
  page->WLatch();
  return {this, page};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard { return {this, NewPage(page_id)}; }

}  // namespace bustub
