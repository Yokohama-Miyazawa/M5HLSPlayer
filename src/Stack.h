/*
  Stack
  Stack program of template
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
#define STACKSIZE 8

template <class T>
class Stack {
  static const int size = STACKSIZE;
  static const int bottom = 0;
public:
  Stack(){
    top = -1;
    data = new T[size];
  }
  ~Stack(){
    delete[] data;
  }
  void push(const T &element){
    if(depth() == size) {
      Serial.println("This Stack is already full.");
      return;
    }
    data[++top] = element;
  }
  T pop(){
    if(depth() == 0) {
      Serial.println("This Stack is already empty.");
      exit(1);
    }
    return data[top--];
  }
  T peek(){
    if(depth() == 0) {
      Serial.println("This Stack is already empty.");
      exit(1);
    }
    return data[top];
  }
  int depth(){
    return top + 1;
  }
  bool search(const T &element){
    if(depth() == 0) return false;
    for(int i = bottom; i <= top; i++) {
      if(data[i] == element) return true;
    }
    return false;
  }
  void clear(){
    top = -1;
  }
private:
  int top;
  T *data;
};
