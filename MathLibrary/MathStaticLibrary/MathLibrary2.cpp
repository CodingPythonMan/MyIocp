#include "pch.h"
#include "MathLibrary2.h"
#include <utility>
#include <limits.h>

unsigned long long MathLibrary2::previous_;  // Previous value, if any
unsigned long long MathLibrary2::current_;   // Current sequence value
unsigned MathLibrary2::index_;

void MathLibrary2::fibonacci_init2(const unsigned long long a, const unsigned long long b)
{
	index_ = 0;
	current_ = a;
	previous_ = b; // see special case when initialized
}

bool MathLibrary2::fibonacci_next2()
{
    // check to see if we'd overflow result or position
    if ((ULLONG_MAX - previous_ < current_) ||
        (UINT_MAX == index_))
    {
        return false;
    }

    // Special case when index == 0, just return b value
    if (index_ > 0)
    {
        // otherwise, calculate next sequence value
        previous_ += current_;
    }
    std::swap(current_, previous_);
    ++index_;
    return true;
}