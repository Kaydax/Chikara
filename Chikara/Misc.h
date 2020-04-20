#pragma once
#ifndef MISC_H
#define MISC_H

#include <deque>
#include <mutex>

template <class T>
class ThreadSafeDeque {
public:
  void push_back(const T& val) {
    mtx.lock();
    deque.push_back(val);
    mtx.unlock();
  }
  void push_front(const T& val) {
    mtx.lock();
    deque.push_front(val);
    mtx.unlock();
  }
  void pop_back() {
    mtx.lock();
    deque.pop_back();
    mtx.unlock();
  }
  void pop_front() {
    mtx.lock();
    deque.pop_front();
    mtx.unlock();
  }
  T& front() {
    mtx.lock();
    T& ret = deque.front();
    mtx.unlock();
    return ret;
  }
  T& back() {
    mtx.lock();
    T& ret = deque.back();
    mtx.unlock();
    return ret;
  }
  size_t size() {
    mtx.lock();
    size_t ret = deque.size();
    mtx.unlock();
    return ret;
  }
private:
  std::deque<T> deque;
  std::mutex mtx;
};
#endif