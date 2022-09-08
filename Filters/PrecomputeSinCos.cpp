/* Copyright (c) 2022, Wayne Wright, W5XD. All rights reserved. */
#include "PrecomputeSinCos.h"
#include <cmath>

namespace {
    const short MAX_SHORT = 0x7fff;
}

bool PrecomputeSinCos::ComputeSinCos(unsigned framesPerSec, unsigned  mixF, std::vector<double> &coef, unsigned &Qindex, unsigned density)
{   // rather than call trig functions in the inner sampling loop, 
    // make a table of the values we know we're going to need based on
    // the known frequencies.

    coef.clear();
    if (mixF == 0)
    {   // special case
        coef.push_back(0.5 * ::sqrt(2.));
        Qindex = 0;
        return false;
    }
    coef.push_back(0); // sine(0) is zero

    bool closestOneNeg = false;
    const int n = framesPerSec * density;
    const int MAX_CYCLES = 10; // should not go this far, but, if so, make-do.
    static const double TARGET_ZERO = 1.5f / MAX_SHORT; // makes sense for 16 bit signed samples
    double closestOne = 0;
    double closestZero = 1;
    int closestZeroIndex = 0;
    int closestOneIdx = 0;
    static const double TwoPi = 2. * 3.14159265358979323846264338;
    for (long i = 1; i < n * MAX_CYCLES; i++)
    {
        double rads = (TwoPi * i  * mixF) / n;
        double v = ::sin(rads);
        coef.push_back(v); // add the sine to the table

        // the rest of this loop is searching for the best
        // place in the table to start the cosine lookup.
        // The answer is: the index where the magnitude is closest
        // to unity. If the value at that index is negative, then
        // we flag that in closestOneNeg because that means
        // the starting value in the table is not +1.f but is -1.f
        double m = ::fabs(v);
        if (m > closestOne)
        {
            closestOneIdx = i;
            closestOne = m;
            closestOneNeg = v < 0.f; // best cosine might be 180 out
        }
        unsigned frac = (2 * i  * mixF) /  n;
        if (frac >= 1 && ((frac % 2) == 0)) // on upward going part of sine?
        {
            if (m < closestZero)
            {
                closestZeroIndex = i;
                closestZero = m;
            }
            if ((i > 1) && (closestZeroIndex == i) && (m <= TARGET_ZERO))
                break;
        }
    }
    coef.resize(closestZeroIndex);
    Qindex = closestOneIdx;
    return closestOneNeg;
}

// At a snapshot in time, we want to change the mixer frequency. The output will glitch if we just to the beginning of the
// table. MinimizeSinCosGlitch searches the new table for the previous values and suggests where to start in the new table
// based on a measure of how bit the glitch will be.
unsigned PrecomputeSinCos::MinimizeSinCosGlitch(double prevMixI, double prevMixQ, unsigned Qindex, double Qscale, const std::vector<double>& sine)
{
    static const unsigned MIN_TABLE_SIZE_TO_SEARCH = 100;
    // If the new frequency is zero, there is no table to speak of. Its just one entry, with unity magnitude
    unsigned ret(0);
    if (sine.size() < MIN_TABLE_SIZE_TO_SEARCH)
        return 0;
    double minDiffMag = 2.0f;
    unsigned bestIdx = 0;
    for (unsigned g = 0; g < sine.size(); g++)
    {
        double mixI = sine[g];
        double mixQ = sine[(g + Qindex) % sine.size()];
        mixQ *= Qscale;
        double Idiff = mixI - prevMixI;
        double Qdiff = mixQ - prevMixQ;
        double diffMag = (Idiff * Idiff) + (Qdiff * Qdiff);
        if (diffMag < minDiffMag)
        {
            minDiffMag = diffMag;
            bestIdx = g;
        }
    }
    return ret;
}
