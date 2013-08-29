#pragma once

#include "SgSystem.h"
#include "SgArrayList.h"

#include <string>
#include <vector>
#include <bitset>

//---------------------------------------------------------------------------

template<typename T>
bool Contains(const std::vector<T>& v, const T& val)
{
    for (size_t i = 0; i < v.size(); ++i)
        if (v[i] == val)
            return true;
    return false;
}

template<typename T, int SIZE>
void Append(std::vector<T>& v, const SgArrayList<T,SIZE>& a)
{
    for (int i = 0; i < a.Length(); ++i) {
        if (!Contains(v, a[i]))
            v.push_back(a[i]);
    }
}

template<typename T>
void Include(std::vector<T>& v, const T& val)
{
    if (!Contains(v, val))
	v.push_back(val);
}

//---------------------------------------------------------------------------


