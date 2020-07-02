#ifndef PEP_JSON_INTEGER_SEQUENCE_HH
#define PEP_JSON_INTEGER_SEQUENCE_HH

#if (__cplusplus >= 201402)  // C++14 and above
#include <utility>

template<std::size_t... I>
using IndexSequence = std::index_sequence<I...>;

template<std::size_t Num>
using MakeIndexSequence = std::make_index_sequence<Num>;

#else // C++11

#include <boost/mp11/integer_sequence.hpp>

template<std::size_t... I>
using IndexSequence = boost::mp11::index_sequence<I...>;

template<std::size_t Num>
using MakeIndexSequence = boost::mp11::make_index_sequence<Num>;

#endif // C++ version switch

#endif // PEP_JSON_INTEGER_SEQUENCE_HH
