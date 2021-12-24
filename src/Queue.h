#pragma once
#include <Arduino.h>
#define QUEUESIZE 16

template <class T>
class Queue {
    static const int size = QUEUESIZE;
    static const int capacity = size - 1;
public:
    Queue(){
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
private:
    int head;
    int tail;
    T *data;
};
