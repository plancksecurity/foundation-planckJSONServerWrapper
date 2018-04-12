#ifndef NFC_SETS_HH
#define NFC_SETS_HH

#include <set>
#include <map>

// These data structures are filled by code generated automatically
// from Unicode's DerivedNormalizationProps.txt and UnicodeData.txt.
// see scripts/ subdirectory

// Contains all codepoints with NFC_No property.
extern const std::set<unsigned> NFC_No;

// Contains all codepoints with NFC_Maybe property.
extern const std::set<unsigned> NFC_Maybe;

// Contains CanonicalCombiningClass for given codepoints. All others have value 0.
extern const std::map<unsigned, unsigned> NFC_CombiningClass;

#endif // NFC_SETS_HH
