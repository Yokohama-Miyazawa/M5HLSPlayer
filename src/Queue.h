/*
  Queue
  Queue program of template
  Copyright (C) 2021  Osamu Miyazawa

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <Arduino.h>
#define DEFAULT_QUEUE_CAPACITY 15

template <class T>
class Queue {
public:
  Queue(){
    capacity = DEFAULT_QUEUE_CAPACITY;
    size = capacity + 1;
    head = 0;
    tail = capacity;
    data = new T[size];
  }
  Queue(int capa){
    if(capa < 2) {
      Serial.println("The specified size is too small.");
      exit(1);
    }
    capacity = capa;
    size = capacity + 1;
    head = 0;
    tail = capacity;
    data = new T[size];
  }
  ~Queue(){
    delete[] data;
  }
  void push(const T &element){
    if(length() == capacity) {
      Serial.println("This Queue is already full.");
      return;
    }
    tail = (tail + 1) % size;
    data[tail] = element;
  }
  T pop(){
    if(length() == 0) {
      Serial.println("This Queue is already empty.");
      exit(1);
    }
    T element = data[head];
    head = (head + 1) % size;
    return element;
  }
  int length(){
    if((tail + 1) % size == head) {
      return 0;
    } else if(head <= tail) {
      return tail - head + 1;
    } else {
      return (size - head) + (tail + 1);
    }
  }
  bool search(const T &element){
    if(length() == 0) return false;
    int i = head;
    while(true) {
      if(data[i] == element) return true;
      if(i == tail) return false;
      i = (i + 1) % size;
    }
  }
  void clear(){
    head = 0;
    tail = capacity;
  }
  // left numbers of elements from head, others are tear-off
  void tearOff(const int &leftNumber){
    if(leftNumber <= 0) { return; }
    tail = (head + leftNumber - 1) % size;
  }
protected:
  int size;
  int capacity;
  int head;
  int tail;
  T *data;
};
