#!/bin/bash

################################# ### ## # #  #    #
#
# Reads the file /usr/share/unicode/DerivedNormalizationProps.txt  (Debian package: unicode-data)
# and generates C++ code for the std::set<> containing the normalization properties
#
################################# ### ## # #  #    #

cat ./unicode/DerivedNormalizationProps.txt | sed -e 's/#.*//g' | grep NFC_QC | sed -e 's/; NFC_QC;//g' |

(

declare -a CHAR_NO
declare -a CHAR_MAYBE

echo -e '// This file is generated by scripts/gen_sets.sh\n// DO NOT EDIT IT!\n\n#include "nfc_sets.hh"\n\n'

U=dummyvalue

while [ -n "$U" ] ; do
	read U V
	if [ -n "$U" ] ; then 
		
		START=0x${U/..*/}
		END=0x${U/*../}
		
		for i in `seq $START $END` ; do
			case $V in
				"N")
					CHAR_NO+=($i)
				;;
				"M")
					CHAR_MAYBE+=($i)
				;;
				*)
					echo 'Unknown: V='$V
					exit 2
			esac
		
		done
	fi
done


# echo "const unsigned NFC_No_Size = ${#CHAR_NO[*]};"
echo -en 'const std::set<unsigned> NFC_No = {'

index=10
for u in "${CHAR_NO[@]}"; do
	if [ $index -ge 10 ] ; then
		echo -en '\n\t'
		index=0
	fi
	printf '0x%04X,' $u
	index=$(( index + 1 ))
done
echo -en '\n\t};\n\n'


# echo "const unsigned NFC_Maybe_Size = ${#CHAR_MAYBE[*]};"
echo -en 'const std::set<unsigned> NFC_Maybe = {'

index=10
for u in "${CHAR_MAYBE[@]}"; do
	if [ $index -ge 10 ] ; then
		echo -en '\n\t'
		index=0
	fi
	printf '0x%04X,' $u
	index=$(( index + 1 ))

done
echo -en '\n\t};\n\n'

)

echo 'const std::map<unsigned, unsigned char> NFC_CombiningClass = {'

cat ./unicode/UnicodeData.txt | cut -d';' -f 1,4 | grep -v -E ';0$' | sed 's/\([0-9A-F]*\);\([0-9]*\)/	{0x\1, \2},/g'

echo -en '};\n\n'


echo 'const std::map<unsigned, std::pair<int,int>> NFC_Decompose = {'

# cut codepoint and Decomposition_Mapping, remove compat mappings (containing <…>), add -1 for one-element mappings:
cat ./unicode/UnicodeData.txt | cut -d';' -f 1,6 | grep -v '<' | \
 sed -e 's/\([0-9A-F]*\);\([0-9A-F ]*\)/\1 @\2@/g' | grep -v @@ | \
 sed -e 's/@\([0-9A-F]*\) \([0-9A-F]*\)@/0x\1 0x\2/' | \
 sed -e 's/@\([0-9A-F]*\)@/0x\1 -1/' | \
 sed -e 's/\([0-9A-F]*\) \([0-9A-Fx]*\) \([0-9A-Fx-]*\)/{0x\1, {\2, \3}},/g'

echo -en '};\n\n'

echo 'std::map< std::pair<unsigned, unsigned>, unsigned> generate_nfc_compose();'
echo -en 'const std::map< std::pair<unsigned, unsigned>, unsigned> NFC_Compose = generate_nfc_compose();\n\n'

# end of file
