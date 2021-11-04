#include <Arduino.h>
#include "StringQueue.h"

StringQueue::StringQueue(){
    head = 0;
    tail = capacity;
    data = new String[size];
}

StringQueue::~StringQueue() {
    delete[] data;
}

void StringQueue::push(const String &element) {
    if(length() == capacity) {
        Serial.println("This Queue is already full.");
        return;
    }
    tail = (tail + 1) % size;
    data[tail] = element;
}

String StringQueue::pop() {
    if(length() == 0) {
        Serial.println("This Queue is already empty.");
        return "";
    }
    String element = data[head];
    head = (head + 1) % size;
    return element;
}

int StringQueue::length() {
    if((tail + 1) % size == head) {
        return 0;
    } else if(head <= tail) {
        return tail - head + 1;
    } else {
        return (size - head) + (tail + 1);
    }
}

bool StringQueue::search(const String &element) {
    if(length() == 0) return false;
    int i = head;
    while(true) {
        if(data[i] == element) return true;
        if(i == tail) return false;
        i = (i + 1) % size;
    }
}
