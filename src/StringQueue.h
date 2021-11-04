#include <Arduino.h>
#define QUEUESIZE 16

class StringQueue {
    static const int size = QUEUESIZE;
    static const int capacity = size - 1;
public:
    StringQueue();
    ~StringQueue();
    void push(const String &element);
    String pop();
    int length();
    bool search(const String &element);
    void clear();
private:
    int head;
    int tail;
    String *data;
};
