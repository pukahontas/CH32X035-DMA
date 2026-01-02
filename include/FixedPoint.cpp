#pragma once
#include "debug.cpp"

// Define signed fixed point type
// 8 fixed fractional bits
typedef int Fixed8;

// Define angular unit
// Fixed point representation with 10 bits representing one full rotation
// i.e a Fixed8 can represent a quarter turn = 90° = pi/2 rad
typedef int AngleHz;

#define FP_FIXED_BITS 8
#define FP_FIXED_VAL (1 << FP_FIXED_BITS)
#define FP_FRACTION_MASK (FP_FIXED_VAL - 1)

#define FP_PI FP_FIXED_VAL * 2
#define FP_2PI FP_PI * 2
#define FP_PI_2 FP_PI / 2

Fixed8 add(Fixed8 a, Fixed8 b)
{
    return a + b;
}

Fixed8 sub(Fixed8 a, Fixed8 b)
{
    return a - b;
}

Fixed8 mult(Fixed8 a, Fixed8 b)
{
    int h1 = a >> FP_FIXED_BITS, h2 = b >> FP_FIXED_BITS;     // Integer portion of the number
    int l1 = a & FP_FRACTION_MASK, l2 = b & FP_FRACTION_MASK; // Fractional portion of the number

    // Implement n = (h1 + l1 2^-8) * (h2 + l2 2^-8)
    // Want to return n 2^8 for representation in fixed point
    // n 2^8 = h1 h2 2^8 + (h1 l2 + h2 l1) + l1 l2 2^-8

    return ((h1 * h2) << FP_FIXED_BITS) + (h1 * l2 + h2 * l1) + ((l1 * l2) >> FP_FIXED_BITS);
}

constexpr Fixed8 computeSinLUT(AngleHz x)
{
    // Define polynomial coeffciients
    const double a = 0.00809167377688, b = -0.166592452584;

    // Convert to radians
    double xRad = x * M_PI_2 / FP_FIXED_VAL;

    // sinx = ax^5 + bx^3 + x
    return xRad * (1.0 + xRad * xRad * (b + xRad * xRad * a)) * FP_FIXED_VAL;
}

#define LUT_BITS 4
#define BIN_SIZE (1 << (FP_FIXED_BITS - LUT_BITS))
constexpr Fixed8 sinLUT[(1 << LUT_BITS) + 1] = {
    computeSinLUT(0 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(1 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(2 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(3 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(4 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(5 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(6 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(7 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(8 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(9 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(10 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(11 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(12 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(13 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(14 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(15 * FP_FIXED_VAL / BIN_SIZE),
    computeSinLUT(16 * FP_FIXED_VAL / BIN_SIZE)};

Fixed8 sinFP(AngleHz x)
{
    // Save the sign of the
    int sign = x < 0 ? -1 : 1;
    x = abs(x);

    int xFrac = x & FP_FRACTION_MASK;
    int xInt = x >> FP_FIXED_BITS;

    // If x is in an odd quadrant (0-indexed), reverse the bits
    if (xInt % 2 == 1)
        xFrac = FP_FIXED_VAL - xFrac;

// Look up the sin value
#if LUT_BITS == 8
    // LUT table has a value for each possible fraction
    Fixed8 sinVal = sinLUT[xFrac];
#else
    // Perform linear interpolation between the LUT values
    size_t a = xFrac / BIN_SIZE; // Find the nearest 16th value to look up in the table
    size_t offset = xFrac & ((1 << LUT_BITS) - 1);

    Fixed8 sinVal = (offset * sinLUT[a + 1] + (BIN_SIZE - offset) * sinLUT[a]) / BIN_SIZE;
#endif

    // If it's in quadrant 2 or 3 (0-indexed), negate the return value
    if (xInt % 4 >= 2)
        sign *= -1;

    return sinVal * sign;
}

Fixed8 cosFP(AngleHz x)
{
    return sinFP(FP_PI_2 - x);
}

Fixed8 sqrtFP(Fixed8 x)
{ 
    if (x < 0)
        x = -x; // Return the sqrt of the magnitude of the value

    if (x == 0)
        return 0;
    
    Fixed8 xn;
    // Choose a good starting guess based on highest bit set
           if (x & 0xFF000000) {
        xn = x >> 12;
    } else if (x & 0x00FF0000) {
        xn = x >> 6;
    } else if (x & 0x0000FF00) {
        xn = x >> 2;
    } else {
        xn = x << 2;
    }

    // If the input value is big enough it would overflow if we used the regular Newton's algorithm,
    //  use a slightly altered version, less-accurate version to keep it within a 32-bit int

    if (x & (INT32_MAX - (INT32_MAX >> 2))) 
    {
        // If the two highest bits are set, Newton's algorithm can overflow when adding to x
        // Use the most conservative formula to prevent overflow
        for (int i = 0; i < 6; i++)
        {
            // Newton's algorithm, taking into account the fixed point shift
            //  6 iterations are always enough with a good starting guess
                            //USBSerial.println(xn);

            xn = (xn / FP_FIXED_VAL + x / xn) * (FP_FIXED_VAL / 2);
        
        }
    } else if (x & (INT32_MAX - (INT32_MAX >> (FP_FIXED_BITS - 1))))
    {
        // If one of the bits in the top 7-bits is set, it can overflow when multiplying by FP_FIXED_VAL / 2
        // Use a modified formula to prevent overflow, this one divides xn 
        for (int i = 0; i < 6; i++)
        {
            // Newton's algorithm, taking into account the fixed point shift
            //  6 iterations are always enough with a good starting guess


            Fixed8 xn_2 = xn >> (FP_FIXED_BITS / 2);
            xn = (xn_2 * xn_2 + x) / xn * (FP_FIXED_VAL / 2);
        
        }
    }
    else if (x & (INT32_MAX - (INT32_MAX >> (FP_FIXED_BITS)))) {
        // Slightly less accurate than the main formula, but because we multiply by FP_FIXED_VAL / 2
        // Instead of FP_FIXED_VAL, we get one extra bit before overflow
        // Only use this if the 8-th highest bit is set
         for (int i = 0; i < 5; i++)
        {
            // Newton's algorithm, taking into account the fixed point shift
            // 6 iterations are always enough with a good starting guess
            xn = (xn + 1) / 2 + x * (FP_FIXED_VAL / 2) / xn;
        }
    } else
    {
        // Fastest and most accurate Newton's method formula, use this when there is no chance of overflow
        for (int i = 0; i < 5; i++)
        {
            // Newton's algorithm, taking into account the fixed point shift
            // 6 iterations are always enough with a good starting guess
            xn = (xn + x * FP_FIXED_VAL / xn + 1) / 2;
        }
    }

    return xn;
}
