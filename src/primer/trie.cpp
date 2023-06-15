#include "primer/trie.h"
#include <string_view>
#include "common/exception.h"

namespace bustub {

template <class T>
auto Trie::Get(std::string_view key) const -> const T * {
  // You should walk through the trie to find the node corresponding to the key. If the node doesn't exist, return
  // nullptr. After you find the node, you should use `dynamic_cast` to cast it to `const TrieNodeWithValue<T> *`. If
  // dynamic_cast returns `nullptr`, it means the type of the value is mismatched, and you should return nullptr.
  // Otherwise, return the value.
  auto node = root_;

  for(auto c : key) {
    if (!node) {
      return nullptr;
    }
    auto it = node->children_.find(c);
    if (it == node->children_.end()) {
      return nullptr;
    }
    node = it->second;
  }

  if (auto value_node = dynamic_cast<const TrieNodeWithValue<T> *>(node.get())) {
    return value_node->value_.get();
  }

  return nullptr;
}

template <class T>
auto Trie::Put(std::string_view key, T value) const -> Trie {
  // Note that `T` might be a non-copyable type. Always use `std::move` when creating `shared_ptr` on that value.
  // You should walk through the trie and create new nodes if necessary. If the node corresponding to the key already
  // exists, you should create a new `TrieNodeWithValue`.

  // Clone of the Trie
  auto root = root_ ? root_->Clone() : std::make_shared<TrieNode>();
  auto node = root;

  // Check if it is empty key -> Put at root
  if(key.empty()) {
    if (auto value_node = std::dynamic_pointer_cast<TrieNodeWithValue<T>>(root); value_node) {
      value_node->value_ = std::make_shared<T>(std::move(value));
      //root = value_node;
    } else {
      root = std::make_shared<TrieNodeWithValue<T>>(root->children_, std::make_shared<T>(std::move(value)));
    }
    return Trie(root);
  }

  // Walking through the trie and creating new nodes if necessary
  for (auto c = key.begin(); c != key.end() - 1; ++c) {
    if(auto next = node->children_.find(*c); next == node->children_.end()) {
      auto new_node = std::make_shared<TrieNode>();
      node->children_.emplace(*c, new_node);
      node = new_node;
    } else {
      node = std::const_pointer_cast<TrieNode>(next->second);
    }
  }

  // Putting last value node
  if(auto last = node->children_.find(key.back()); last == node->children_.end()) {
    node->children_.emplace(key.back(), std::make_shared<TrieNodeWithValue<T>>(std::make_shared<T>(std::move(value))));
  } else {
    auto value_node = std::dynamic_pointer_cast<TrieNodeWithValue<T>>(std::const_pointer_cast<TrieNode>(last->second));
    if (value_node) {
      value_node->value_ = std::make_shared<T>(std::move(value));
    } else {
      node->children_[key.back()] = std::make_shared<TrieNodeWithValue<T>>(last->second->children_ ,std::make_shared<T>(std::move(value)));
    }
  }

  return Trie(root);
}

auto Trie::Remove(std::string_view key) const -> Trie {
  throw NotImplementedException("Trie::Remove is not implemented.");

  // You should walk through the trie and remove nodes if necessary. If the node doesn't contain a value anymore,
  // you should convert it to `TrieNode`. If a node doesn't have children anymore, you should remove it.
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
