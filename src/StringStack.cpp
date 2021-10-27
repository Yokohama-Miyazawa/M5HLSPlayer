#include <Arduino.h>
#include "StringStack.h"

StringStack::StringStack(){
    top = -1;
}

StringStack::~StringStack() {
    delete[] data;
}

void StringStack::push(const String &element) {
    if(depth() == size) {
        Serial.println("This Stack is already full.");
        return;
    }
    data[++top] = element;
}

String StringStack::pop() {
    if(depth() == 0) {
        Serial.println("This Stack is already empty.");
        return "";
    }
    return data[top--];
}

String StringStack::peek() {
    if(depth() == 0) {
        Serial.println("This Stack is already empty.");
        return "";
    }
    return data[top];
}

int StringStack::depth() {
    return top + 1;
}

bool StringStack::search(const String &element) {
    if(depth() == 0) return false;
    for(int i = bottom; i <= top; i++) {
        if(data[i] == element) return true;
    }
    return false;
}