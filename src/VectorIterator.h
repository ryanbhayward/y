//----------------------------------------------------------------------------
/** @file VectorIterator.h
 */
//----------------------------------------------------------------------------

#ifndef VECTORITERATOR_H
#define VECTORITERATOR_H

#include <vector>

//----------------------------------------------------------------------------

template<typename T>
class VectorIterator
{
public:
    VectorIterator(const std::vector<T>& vector);
    
    const T& operator*() const;

    operator bool() const;

    void operator++();

private:
    std::size_t m_index;

    const std::vector<T>& m_vector;
};

template<typename T>
inline VectorIterator<T>::VectorIterator(const std::vector<T>& vector)
    : m_index(0),
      m_vector(vector)
{
}
 
template<typename T>
inline const T& VectorIterator<T>::operator*() const
{
    return m_vector[m_index];
}

template<typename T>
inline VectorIterator<T>::operator bool() const
{
    return m_index < m_vector.size();
}

template<typename T>
inline void VectorIterator<T>::operator++()
{
    m_index++;
}

//----------------------------------------------------------------------------

#endif // VECTORITERATOR_H
