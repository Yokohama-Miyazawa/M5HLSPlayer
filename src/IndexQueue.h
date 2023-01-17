/*
  IndexQueue
  Extended queue program in order to reference besides the head
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
#include "Queue.h"

template <class T>
class IndexQueue : public Queue <T> {
public:
  IndexQueue(){
    index = this->capacity;
  }
  IndexQueue(int capa){
    index = this->capacity;
  }
  ~IndexQueue(){}
  void push(const T &element){
    if(this->length() == this->capacity) pop();
    this->tail = (this->tail + 1) % this->size;
    this->data[this->tail] = element;
  }
  T pop(){
    if(this->length() == 0) {
      Serial.println("This IndexQueue is already empty.");
      exit(1);
    }
    T element = this->data[this->head];
    if(index == this->head) index = (index + 1) % this->size;
    this->head = (this->head + 1) % this->size;
    return element;
  }
  T next(){
    int nextIndex = (index + 1) % this->size;
    if(nextIndex > this->tail && nextIndex <= this->head) {
      Serial.println("Index have already reached the tail.");
      exit(1);
    }
    index = (index + 1) % this->size;
    return this->data[index];
  }
  int margin(){
    if(index <= this->tail) {
      return this->tail - index;
    } else {
      return this->size - index + this->tail;
    }
  }
  int rearMargin(){
    if(index < this->head) {
      return index + ((this->size - this->head) + 1);
    } else {
      return index - this->head;
    }
  }
  T former(){
    if(!rearMargin()) {
      Serial.println("Index have already reached the head.");
      exit(1);
    }
    index = index == 0 ? this->size - 1 : index - 1;
    return this->data[index];
  }
protected:
  int index;
};
