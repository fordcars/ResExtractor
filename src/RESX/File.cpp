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

#include "RESX/File.hpp"
#include "RESX/ResourceFork.hpp"
#include "RESX/Defs.hpp"

#include <iostream>

namespace RESX
{

// blockSize in bytes
File::File(const std::string& HFSFileName, unsigned int blockSize)
    : mHFSFileName(HFSFileName),
    mHFSFile(new std::ifstream), // Create the file stream
    mBlockSize(blockSize)
{
    mHFSFile->open(mHFSFileName, std::ios::binary);
    if( !(*mHFSFile) )
        std::cerr << "Could not open HFS file!" << std::endl;
}

File::~File()
{

}

// Factory method
ResourceFork File::loadResourceFork(unsigned int firstBlock)
{
    Defs::addr blockStartAddress = firstBlock * mBlockSize;
    return ResourceFork(mHFSFile, blockStartAddress);
}

} // namespace RESX
