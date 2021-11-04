#include <Arduino.h>
#define STACKSIZE 8

class StringStack {
    static const int size = STACKSIZE;
    static const int bottom = 0;
public:
    StringStack();
    ~StringStack();
    void push(const String &element);
    String pop();
    String peek();
    int depth();
    bool search(const String &element);
    void clear();
private:
    int top;
    String *data;
};
