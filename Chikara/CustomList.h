#pragma once

// barebones specialized doubly-linked list that allocates from a pool
// could be single in the future, but this is just to test an idea
#include <vector>

template<typename T>
class CustomListNode {
public:
  void Unlink() {
    if (next)
      next->prev = prev;
    if (prev)
      prev->next = next;
  }
  CustomListNode<T>* next = nullptr;
  CustomListNode<T>* prev = nullptr;
  T data;
  size_t idx;
};

template<typename T>
class CustomList {
public:
  static constexpr size_t ALLOC_BLOCK_SIZE = 1024;

  CustomListNode<T>* Front() const {
    return head;
  }

  void Delete(CustomListNode<T>* node) {
    if (node == head)
      head = node->next;
    node->Unlink();
    free.push_back(node->idx);
    size--;
  }

  // no push_back because it isn't used atm
  void PushFront(T& data) {
    auto node = AllocNode(data);
    if (head)
      head->prev = node;
    node->next = head;
    node->prev = nullptr;
    head = node;
    size++;
  }

  size_t Size() const {
    return size;
  }

  size_t Capacity() const {
    return pool.size() * ALLOC_BLOCK_SIZE;
  }
private:
  CustomListNode<T>* AllocNode(T& data) {
    if (free.empty()) {
      size_t prev_size = pool.size() * ALLOC_BLOCK_SIZE;
      pool.resize(pool.size() + 1);
      auto& new_pool = pool.back();
      new_pool.resize(ALLOC_BLOCK_SIZE);
      for (int i = 0; i < ALLOC_BLOCK_SIZE; i++)
        new_pool[i].idx = prev_size + i;
      for (int i = prev_size; i < prev_size + ALLOC_BLOCK_SIZE; i++)
        free.push_back(i);
    }
    size_t idx = free.back();
    free.pop_back();
    CustomListNode<T>* node = &pool[idx / ALLOC_BLOCK_SIZE][idx % ALLOC_BLOCK_SIZE];
    memcpy(&node->data, &data, sizeof(T));
    return node;
  }

  std::vector<std::vector<CustomListNode<T>>> pool; // not a vector of arrays because the allocation per block needs to be separate
  std::vector<size_t> free;

  CustomListNode<T>* head = nullptr;
  //CustomListNode* back = nullptr;

  size_t size = 0;
};