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

/* Interface/master include file */

/* Notes */
// - Each resource fork you attempt to read from MUST be within
//   a single extent!

/* For future reference: */
// All floating types follow IEEE:
// (Metrowerks Inc., CodeWarrior ® C, C , and Assembly Language Reference. p. 28-29.)
// - float: 32-bit -> 1.17549e-38 to 3.40282e+38
// - short double: 64-bit-> 2.22507e-308 to 1.79769e+308
// - double: 64-bit -> 2.22507e-308 to 1.79769e+308
// - long double: 64-bit -> 2.22507e-308 to 1.79769e+308
// Note : Since long/short double is the same as double
// on PowerPC, you should replace all long/short doubles with
// regular doubles in the PowerPC code (makes life much easier).
// (Fun fact: PowerPC doesn't support long doubles at all.)

// Padding on PowerPC according to CodeWarrior:
// (Metrowerks Inc., CodeWarrior® Targeting Mac OS. p. 97.)
// - char: 1-byte boundary
// - int16_t: 2-byte boundary
// - int32_t: 4-byte boundary
// - int64_t: 8-byte boundary
// - float: 4-byte boundary
// - double: 8-byte boundary
// - long double: 8-byte boundary
// - struct: alignment of largest scalar type in struct

#ifndef RES_EXTRACTOR_HPP
#define RES_EXTRACTOR_HPP

#include "RESX/Defs.hpp"
#include "RESX/File.hpp"
#include "RESX/ResourceFork.hpp"

#endif // RES_EXTRACTOR_HPP
