#pragma once
class MathLibrary2
{
public:
	// The Fibonacci recurrence relation describes a sequence F
	// where F(n) is { n = 0, a
	//               { n = 1, b
	//               { n > 1, F(n-2) + F(n-1)
	// for some initial integral values a and b.
	// If the sequence is initialized F(0) = 1, F(1) = 1,
	// then this relation produces the well-known Fibonacci
	// sequence: 1, 1, 2, 3, 5, 8, 13, 21, 34, ...

	// Initialize a Fibonacci relation sequence
	// such that F(0) = a, F(1) = b.
	// This function must be called before any other function.
	static void fibonacci_init2(const unsigned long long a, const unsigned long long b);

	// Produce the next value in the sequence.
	// Returns true on success and updates current value and index;
	// false on overflow, leaves current value and index unchanged.
	static bool fibonacci_next2();

private:
	static unsigned long long previous_;  // Previous value, if any
	static unsigned long long current_;   // Current sequence value
	static unsigned index_;               // Current seq. position
};

