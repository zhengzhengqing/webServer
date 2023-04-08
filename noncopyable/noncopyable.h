#ifndef NONCOPYABLE_H_
#define NONCOPYABLE_H_

#include <iostream>
using namespace std;

class noncopyable
{
    protected:
        noncopyable() = default;
        ~noncopyable() = default;

    private:
        noncopyable(const noncopyable & non) = delete;
        noncopyable & operator=(const noncopyable & non) = delete;
};

#endif
