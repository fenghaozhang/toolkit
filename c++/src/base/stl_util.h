#ifndef _SRC_BASE_STL_UTIL_H
#define _SRC_BASE_STL_UTIL_H

template<class ForwardIterator>
void STLDeleteContainerPointers(
        ForwardIterator begin,
        ForwardIterator end)
{
    while (begin != end)
    {
        ForwardIterator temp = begin;
        ++begin;
        delete *temp;
    }
}

template<class T>
void STLDeleteElements(T* container)
{
    if (container != NULL)
    {
        STLDeleteContainerPointers(container->begin(), container->end());
        container->clear();
    }
}

template <class ForwardIterator>
void STLDeleteContainerSecondPointers(
        ForwardIterator begin,
        ForwardIterator end)
{
    while (begin != end)
    {
        ForwardIterator temp = begin;
        ++begin;
        delete temp->second;
    }
}

#endif // _SRC_BASE_STL_UTIL_H
