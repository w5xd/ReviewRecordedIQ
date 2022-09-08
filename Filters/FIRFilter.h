/* Copyright (c) 2022, Wayne Wright, W5XD. All rights reserved. */
#pragma once
#include <vector>
#include <cmath>
#include <cstring>

typedef double FilterCoeficient_t;

// Finite Impulse Response filter.

 class CFIRFilter
    {
    public:
        CFIRFilter() 
        : m_lastIndex(0)
        , m_pCoef(&Dummy)
        , m_history(1, 0.f)
        {}

        CFIRFilter(unsigned len, const FilterCoeficient_t *pCoef) 
        {
            setFilterDefinition(len, pCoef);
        }

        void applySample(double b)
        {
            m_history[m_lastIndex++] = b;
            if (m_lastIndex >= m_history.size())
                m_lastIndex = 0;
        };

        double value() const
        {
            const unsigned len = static_cast<unsigned>(m_history.size());
            unsigned j = m_lastIndex;
            const FilterCoeficient_t *pCoef = m_pCoef;
            double v = 0;
            const double *pHist = &m_history[0];
            for (unsigned i = 0; i < len; i++)
            {
                v += pHist[j++] * *pCoef++;
                if (j >= len) j = 0;
            }
            return v;
        }

        void setFilterDefinition(unsigned len, const FilterCoeficient_t *pCoef)
        {
            if (len == 0)
                return; // not allowed
            if (len != m_history.size())
            {
                m_history.resize(len);
                memset(&m_history[0], 0, sizeof(float) * len);
                m_lastIndex = 0;
            }
            m_pCoef = pCoef;
        }

        // diagnostic
        double Rms() const {
            double v = 0;
            for (auto e : m_history)
                v += e * e;
            v = sqrt(v);
            v /= m_history.size();
            return v;
        }

        double FabsAvg() const {
            double v = 0;
            for (auto e : m_history)
                v += fabs(e);
            v /= m_history.size();
            return v;
        }

    protected:
        static  const FilterCoeficient_t Dummy;

        unsigned m_lastIndex;
        const FilterCoeficient_t *m_pCoef;
        std::vector<double> m_history;
    };
