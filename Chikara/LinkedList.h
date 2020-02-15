#pragma once

#include <stdexcept>
#include <iostream>
using namespace std;

template <class T>
class LinkedList
{
private:
  struct node
  {
    T data;
    node* next;
  } *head;

public:
  LinkedList();
  ~LinkedList();
  void add(T d);
  void remove(T d);
  void clear();
  void makeCircular();
  bool isCircular();
  void display(const char* s);
};
