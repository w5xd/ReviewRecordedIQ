/* Copyright (c) 2022, Wayne Wright, W5XD. All rights reserved. */
#pragma once
#include <vector>
class PrecomputeSinCos {
public:
    static bool ComputeSinCos(unsigned framesPerSec, 
        unsigned mixF, // use only positive here. caller flips Q on negative. up to framesPerSec/2
        std::vector<double> &coef, unsigned &Qindex, unsigned density = 1);

    static unsigned MinimizeSinCosGlitch(double prevMixI, double prevMixQ, unsigned Qindex, double QScale, const std::vector<double>& coef);
};
