#include "primer/trie.h"
#include <stack>
#include <string_view>
#include "common/exception.h"

namespace bustub {

template <class T>
auto Trie::Get(std::string_view key) const -> const T* {
  auto node = root_;
  for (auto c : key) {
    if(!node) {
      return nullptr;
    }
    auto next = node->children_.find(c);
    if (next == node->children_.cend()) {
      return nullptr;
    }
    node = next->second;
  }

  auto value_node = dynamic_cast<const TrieNodeWithValue<T>*>(node.get());
  return (value_node != nullptr) ? value_node->value_.get() : nullptr;
}

template <class T>
auto Trie::Put(std::string_view key, T value) const -> Trie {
  auto value_ptr = std::make_shared<T>(std::move(value));

  if(key.empty()) {
    if (root_) {
      return Trie(std::make_shared<const TrieNodeWithValue<T>>(root_->children_, value_ptr));
    }
    return Trie(std::make_shared<const TrieNodeWithValue<T>>(value_ptr));
  }

  auto root = root_ ? root_->Clone() : std::make_shared<TrieNode>();
  auto this_ptr = std::const_pointer_cast<TrieNode>(root_);
  auto clone_ptr = root;
  bool mismatch = root_ == nullptr;

  for (auto c = key.begin(); c != key.end() - 1; ++c) {
    if(mismatch) {
      clone_ptr->children_[*c] = std::make_shared<const TrieNode>();
    } else {
      auto next = this_ptr->children_.find(*c);
      if (next == this_ptr->children_.end()) {
        clone_ptr->children_[*c] = std::make_shared<const TrieNode>();
        mismatch = true;
      } else {
        clone_ptr->children_[*c] = next->second ?  next->second->Clone() : nullptr;
      }
      this_ptr = std::const_pointer_cast<TrieNode>(next->second);
    }

    clone_ptr = std::const_pointer_cast<TrieNode>(clone_ptr->children_[*c]);
  }

  if (clone_ptr->children_.count(key.back()) == 0) {
    clone_ptr->children_[key.back()] = std::make_shared<const TrieNodeWithValue<T>>(value_ptr);
  } else {
    auto children(clone_ptr->children_[key.back()]->children_);
    clone_ptr->children_[key.back()] = std::make_shared<const TrieNodeWithValue<T>>(children, value_ptr);
  }

  return Trie(root);
}

auto Trie::Remove(std::string_view key) const -> Trie {
  if (!root_ || key.empty()) {
    return Trie(root_);
  }

  std::shared_ptr<TrieNode> root = root_->Clone();
  auto node = root;
  std::shared_ptr<TrieNode> prev = nullptr;

  for (char c : key) {
    auto next = node->children_.find(c);
    if (next == node->children_.end()) {
      return Trie(root);
    }

    node->children_[c] = node->children_[c]->Clone();
    prev = node;

    node = std::const_pointer_cast<TrieNode>(next->second);
  }

  if (node->children_.empty()) {
    prev->children_.erase(key.back());
  } else {
    prev->children_[key.back()] = std::make_shared<const TrieNode>(node->children_);
  }

  return Trie(root);
}

// Below are explicit instantiation of template functions.
//
// Generally people would write the implementation of template classes and functions in the header file. However, we
// separate the implementation into a .cpp file to make things clearer. In order to make the compiler know the
// implementation of the template functions, we need to explicitly instantiate them here, so that they can be picked up
// by the linker.

template auto Trie::Put(std::string_view key, uint32_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint32_t *;

template auto Trie::Put(std::string_view key, uint64_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint64_t *;

template auto Trie::Put(std::string_view key, std::string value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const std::string *;

// If your solution cannot compile for non-copy tests, you can remove the below lines to get partial score.

using Integer = std::unique_ptr<uint32_t>;

template auto Trie::Put(std::string_view key, Integer value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const Integer *;

template auto Trie::Put(std::string_view key, MoveBlocked value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const MoveBlocked *;

}  // namespace bustub
