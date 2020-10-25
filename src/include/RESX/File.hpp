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

#ifndef RESX_FILE_HPP
#define RESX_FILE_HPP

#include "Defs.hpp"

#include <string>
#include <fstream>
#include <memory> // For smart pointers

namespace RESX
{

class ResourceFork;
class File
{
public:
    // Type aliases
    using ifstreamPointer = std::shared_ptr<std::ifstream>;

private:
    std::string mHFSFileName;
    ifstreamPointer mHFSFile;
    int mBlockSize;

public:
    File(const std::string& HFSFileName, unsigned int blockSize);
    ~File();

    ResourceFork loadResourceFork(unsigned int firstBlock);
};

} // namespace RESX
#endif // RESX_FILE_HPP
