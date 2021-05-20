#!/bin/sh

if [ $# -eq 0 ]
then
	source "${PWD}/build.default.sh"
	echo "Centaurus CMake build default configuration."
elif [ $# -lt 2 ]
then
	echo "$0 <generator> <build type>"
else
	source "$PWD/buildconfig.sh"

	CENTAURUS_GENERATOR="$1"
	CENTAURUS_BUILD_TYPE="$2"
	
	if [ "$CENTAURUS_BUILD_TYPE" != "Release" ]
	then
		CENTAURUS_BUILD_TYPE="Debug"
	fi
fi

if [ -f "$CENTAURUS_PATH_BINARY" ]
then
	echo "Deleting CMake cache.."
	rm -rf "$CENTAURUS_PATH_BINARY"
fi

printf "Centaurus Build\n\tConfig:\t$CENTAURUS_BUILD_TYPE\n"
printf "\tSource:\t$CENTAURUS_PATH_SOURCE\n"
printf "\tBinary:\t$CENTAURUS_PATH_BINARY\n"

cmake -G "$CENTAURUS_GENERATOR" -S "$CENTAURUS_PATH_SOURCE"	\
	-B "$CENTAURUS_PATH_BINARY" -DCMAKE_BUILD_TYPE=$CENTAURUS_BUILD_TYPE
