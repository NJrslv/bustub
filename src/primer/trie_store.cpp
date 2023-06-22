#include "primer/trie_store.h"
#include "common/exception.h"

namespace bustub {

template <class T>
auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<T>> {
  root_lock_.lock();
  auto root = root_;
  root_lock_.unlock();

  if (auto value = root.Get<T>(key); value) {
    return std::make_optional<ValueGuard<T>>(root, *value);
  }

  return std::nullopt;
}

template <class T>
void TrieStore::Put(std::string_view key, T value) {
  write_lock_.lock();

  root_lock_.lock();
  auto root = root_.Put(key, std::move(value));
  std::swap(root, root_);
  root_lock_.unlock();

  write_lock_.unlock();
}

void TrieStore::Remove(std::string_view key) {
  write_lock_.lock();

  root_lock_.lock();
  auto root = root_.Remove(key);
  std::swap(root, root_);
  root_lock_.unlock();

  write_lock_.unlock();
}

// Below are explicit instantiation of template functions.

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<uint32_t>>;
template void TrieStore::Put(std::string_view key, uint32_t value);

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<std::string>>;
template void TrieStore::Put(std::string_view key, std::string value);

// If your solution cannot compile for non-copy tests, you can remove the below lines to get partial score.

using Integer = std::unique_ptr<uint32_t>;

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<Integer>>;
template void TrieStore::Put(std::string_view key, Integer value);

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<MoveBlocked>>;
template void TrieStore::Put(std::string_view key, MoveBlocked value);

}  // namespace bustub