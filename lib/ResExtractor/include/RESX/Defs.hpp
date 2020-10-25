// Copyright 2020 Carl Hewett
//
// This file is part of ResExtractor.
//
// ResExtractor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ResExtractor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with ResExtractor. If not, see <http://www.gnu.org/licenses/>.

#ifndef RESX_DEFINITIONS_HPP
#define RESX_DEFINITIONS_HPP

#include <cstdint>
#include <climits> // For CHAR_BIT
#include <cstddef> // For size_t

/* Global defines */

// Define if you are compiling for a little-endian machine.
// When undefined, big-endian machine is assumed (unless universal
// endianness hack is used).
// #define RESX_ON_LITTLE_ENDIAN_MACHINE

// If defined, will use union hack to figure out the machine's endianness
// at run-time. Use this if you wish to detect endianness automatically.
// Might cause undefined behavior/compile-time error on future compilers.
// Setting this to true will ignore RESX_ON_LITTLE_ENDIAN_MACHINE.
#define RESX_USE_UNIVERSAL_ENDIANNESS_HACK

/* *** */

namespace RESX
{

namespace Defs // Defs for "definitions"
{
    using addr = unsigned long int; // At least 32-bit

#ifdef RESX_USE_UNIVERSAL_ENDIANNESS_HACK
    // Is not guaranteed to work on all compilers.
    // From David Cournapeau
    // (https://stackoverflow.com/questions/1001307/detecting-endianness-programmatically-in-a-c-program)
    static bool isMachineLittleEndianHack()
    {
        union {
            uint32_t i;
            char c[4];
        } bint = {0x01020304};

        return !(bint.c[0] == 1);
    }

    const bool machineIsLittleEndian = isMachineLittleEndianHack();
#elif defined(RESX_ON_LITTLE_ENDIAN_MACHINE)
    const bool machineIsLittleEndian = true;
#else
    // Big-endian machine assumed, don't change endianness!
    const bool machineIsLittleEndian = false;
#endif /* RESX_USE_UNIVERSAL_ENDIANNESS_HACK*/

    // If you are running on a little-endian machine, you must call this
    // on each struct primitive (on every member and on every element of
    // all member arrays)!!!
    // Remember: endianness only applies to individual values (numbers)!
    // So, the struct is in the correct order, but the individual members
    // have the wrong byte order.
    // Note: this method also looks like a union hack.
    // From https://stackoverflow.com/questions/105252/how-do-i-convert-between-big-endian-and-little-endian-values-in-c
    template<typename T>
    static T swapEndian(T u)
    {
        static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

        union
        {
            T u;
            unsigned char u8[sizeof(T)];
        } source, dest;

        source.u = u;

        for (size_t k = 0; k < sizeof(T); k++)
            dest.u8[k] = source.u8[sizeof(T) - k - 1];

        return dest.u;
    }

    // Makes the value the correct endianness for the client machine.
    template<typename T>
    static T makeSafeEndian(T u)
    {
        if(machineIsLittleEndian)
            return swapEndian(u);
        else
            return u;
    }
} // namespace Defs

} // namespace RESX

#endif // RESX_DEFINITIONS_HPP
