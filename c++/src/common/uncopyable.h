#ifndef _SRC_COMMON_UNCOPYABLE_H
#define _SRC_COMMON_UNCOPYABLE_H

class Uncopyable {
protected:
    Uncopyable() {}
    ~Uncopyable() {}

private:
    Uncopyable(const Uncopyable& o);
    Uncopyable& operator=(const Uncopyable& o);
};

#endif  // _SRC_COMMON_UNCOPYABLE_H
