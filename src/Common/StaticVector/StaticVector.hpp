#pragma once
#include <boost/container/static_vector.hpp>

template <typename T, size_t Capacity>
using StaticVector = boost::container::static_vector<T, Capacity>;