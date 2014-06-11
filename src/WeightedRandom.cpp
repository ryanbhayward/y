//----------------------------------------------------------------------------
/** @file WeightedRandom.cpp 

    Implementation inspired by the weighted random selection used in
    Castro (https://github.com/tewalds/castro/).
*/
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "SgRandom.h"
#include "WeightedRandom.h"
#include "Board.h"
#include <math.h>

//----------------------------------------------------------------------------

WeightedRandom::WeightedRandom()
{
    Init(Y_MAX_CELL);
}

WeightedRandom::WeightedRandom(int size)
{
    Init(size);
}

void WeightedRandom::Init(int size)
{
    if (size > 256) size = 512;
    else if (size > 128) size = 256;
    else if (size > 64) size = 128;
    else if (size > 32) size = 64;
    else if (size > 16) size = 32;
    else if (size > 8) size = 16;
    else size = 8;
    m_size = size;
    m_weights = new float[2 * m_size];
    Clear();
}

void WeightedRandom::Clear()
{
    for (int i = 0; i < 2 * m_size; ++i) 
        m_weights[i] = 0.0f;
}

void WeightedRandom::SetWeightAndUpdate(int p, float w)
{
    p += m_size;
    m_weights[p] = w;
    while (p /= 2)
        m_weights[p] = m_weights[2*p] + m_weights[2*p + 1];
}

void WeightedRandom::SetWeight(int p, float w)
{
    p += m_size;
    m_weights[1] -= m_weights[p] - w;
    m_weights[p] = w;
}

void WeightedRandom::Build()
{
    for (int i = m_size - 1; i >= 1; --i)
        m_weights[i] = m_weights[2*i] + m_weights[2*i + 1];
}

float WeightedRandom::Total() const
{
    return m_weights[1];
}

int WeightedRandom::Choose(SgRandom& random) const
{
    int i;
    do {
        float r = random.Float(m_weights[1]);
        i = 2;
        while (i < m_size)
        {
            // std::cerr << "i=" << i << " r=" << r 
            //           << " w[" << i << "]=" << m_weights[i] 
            //           << " w[" << i+1 << "]=" << m_weights[i+1]
            //           << '\n';
            if (r > m_weights[i]) 
            {
                r -= m_weights[i];
                i++;
            }
            i *= 2;
        }
        if (r > m_weights[i])
            i++;
    } while (m_weights[i] <= 1e-7);
    return i - m_size;
}

int WeightedRandom::ChooseLinear(SgRandom& random) const
{
    int i;
    do {
        const float rand = random.Float(m_weights[1]);
        float r = rand;
        for (i = m_size; i < 2 * m_size; ++i) 
        {
            r -= m_weights[i];
            if (r <= 0.00009 && m_weights[i] > 0.0f)
                return i - m_size;
        }
        // std::cerr() << "Didn't find it!\n"
        //             << "total=" << m_weights[1] << " rand=" << rand 
        //             << " r=" << r << '\n';
    } while (1);
    return i - m_size;
}

//----------------------------------------------------------------------------
