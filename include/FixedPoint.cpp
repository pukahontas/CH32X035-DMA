#include <math.h>

// Define signed fixed point type
// 8 fixed fractional bits 
typedef int Fixed8;

// Define angular unit
// Fixed point representation with 10 bits representing one full rotation
// i.e a Fixed8 can represent a quarter turn = 90° = pi/2 rad
typedef int AngleHz; 

#define FIXED_BITS 8
#define FIXED_VAL (1 << FIXED_BITS)
#define FRACTION_MASK (FIXED_VAL - 1)

Fixed8 add(Fixed8 a, Fixed8 b) {
    return a + b;
}

Fixed8 sub(Fixed8 a, Fixed8 b) {
    return a - b;
}

Fixed8 mult(Fixed8 a, Fixed8 b) {
    int h1 = a >> FIXED_BITS, h2 = b >> FIXED_BITS;  // Integer portion of the number
    int l1 = a & FRACTION_MASK, l2 = b & FRACTION_MASK; // Fractional portion of the number

    // Implement n = (h1 + l1 2^-8) * (h2 + l2 2^-8)
    // Want to return n 2^8 for representation in fixed point
    // n 2^8 = h1 h2 2^8 + (h1 l2 + h2 l1) + l1 l2 2^-8

    return ((h1 * h2) << FIXED_BITS) + (h1 * l2 + h2 * l1) + ((l1 * l2) >> FIXED_BITS);
}

constexpr Fixed8 computeSinLUT(AngleHz x) {
    // Define polynomial coeffciients
    const double a = 0.00809167377688, b = -0.166592452584;

    // Convert to radians
    double xRad = x * M_PI_2 / FIXED_VAL;

    // sinx = ax^5 + bx^3 + x
    return xRad * (1.0 + xRad * xRad * (b + xRad * xRad * a)) * FIXED_VAL;
}

#define LUT_BITS 4
#define BIN_SIZE (1 << (FIXED_BITS - LUT_BITS))
constexpr Fixed8 sinLUT[(1 << LUT_BITS) + 1] = {
    computeSinLUT(0 * FIXED_VAL / BIN_SIZE),
    computeSinLUT(1  * FIXED_VAL / BIN_SIZE),
    computeSinLUT(2  * FIXED_VAL / BIN_SIZE),
    computeSinLUT(3  * FIXED_VAL / BIN_SIZE),
    computeSinLUT(4  * FIXED_VAL / BIN_SIZE),
    computeSinLUT(5  * FIXED_VAL / BIN_SIZE),
    computeSinLUT(6  * FIXED_VAL / BIN_SIZE),
    computeSinLUT(7  * FIXED_VAL / BIN_SIZE),
    computeSinLUT(8  * FIXED_VAL / BIN_SIZE),
    computeSinLUT(9  * FIXED_VAL / BIN_SIZE),
    computeSinLUT(10 * FIXED_VAL / BIN_SIZE),
    computeSinLUT(11 * FIXED_VAL / BIN_SIZE),
    computeSinLUT(12 * FIXED_VAL / BIN_SIZE),
    computeSinLUT(13 * FIXED_VAL / BIN_SIZE),
    computeSinLUT(14 * FIXED_VAL / BIN_SIZE),
    computeSinLUT(15 * FIXED_VAL / BIN_SIZE),
    computeSinLUT(16 * FIXED_VAL / BIN_SIZE)
};

Fixed8 sinFixed (AngleHz x) {
    // Save the sign of the 
    int sign = x < 0 ? -1 : 1;
    x = abs(x);

    int xFrac = x & FRACTION_MASK;
    int xInt = x >> FIXED_BITS;

    // If x is in an odd quadrant (0-indexed), reverse the bits 
    if (xInt % 2 == 1)
        xFrac = FIXED_VAL - xFrac;

    // Look up the sin value
    size_t a = xFrac / BIN_SIZE; // Find the nearest 16th value to look up in the table
    size_t offset = xFrac & ((1 << LUT_BITS) - 1);

    Fixed8 sinVal = (offset * sinLUT[a + 1] + (BIN_SIZE - offset) * sinLUT[a])  / BIN_SIZE;

    // If it's in quadrant 2 or 3 (0-indexed), negate the return value
    if (xInt % 4 >= 2)
        sign *= -1;

    return sinVal * sign;
}

Fixed8 cosFixed (AngleHz x) {
    return sinFixed(FIXED_VAL - x);
}

