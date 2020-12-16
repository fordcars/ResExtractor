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

#ifndef RESX_RESOURCE_FORK_HPP
#define RESX_RESOURCE_FORK_HPP

#include "ResExtractor.hpp"
#include "RESX/Defs.hpp"

#include <fstream>
#include <utility> // For pair
#include <string>
#include <cstdlib> // For std::malloc
#include <memory> // For smart pointers

#include <ios>
#include <iostream> // For istream and cerr
#include <vector>
#include <algorithm> // For reverse()
#include <iterator>

#include <cstring> // For std::memcpy (why is this in <cstring>)
#include <cstddef> // For std::size_t

namespace RESX
{

// Class definition, not instantiation!
struct freeDelete
{
    void operator()(void* x) { free(x); }
};

class ResourceFork
{
public:
    // Type aliases
    // To save time typing the looooonnnggg type.
    using ifstreamPointer = std::shared_ptr<std::ifstream>;

private:
    // First: (number of resources -1) of this type in our map.
    // Second: address of reference list for this type.
    using ReferenceListPointerPair = std::pair<int, Defs::addr>;

    ifstreamPointer mHFSFile;

    // The address of the resource fork itself within the parent file
    Defs::addr mStartAddr;

    // Header
    // "Zone" added to avoid confusion with actual resource data.
    // This section of memory is simply known as "resource data"
    // in official documentation.
    Defs::addr mResourceDataZoneAddr;
    Defs::addr mResourceMapAddr;
    Defs::addr mResourceDataLength;
    Defs::addr mResourceMapLength;

    // Resource Map
    Defs::addr mResourceTypeListAddr;
    Defs::addr mResourceNameListAddr;
    int mNumberOfTypesMinusOne; // Can be negative

    static inline void checkFloatingTypes();

    // Casts typeToCastFrom* to std::unique_ptr<typeToCastTo>.
    // Copies and returns data on heap.
    // https://www.fluentcpp.com/2017/08/15/function-templates-partial-specialization-cpp/
    template<typename typeToCastTo, typename typeToCastFrom>
    // const typeToCastFrom* because C++ references can't be incremented.
    static std::unique_ptr<typeToCastTo> saferReinterpretCastToHeap(const typeToCastFrom* thingToCast,
                                                        std::size_t sizeOfThingToCast)
    {
        // The only use of sizeOfThingToCast lol
        if(sizeOfThingToCast != sizeof(typeToCastTo))
            std::cerr << "Value to reinterpret cast is not the same size " <<
                "as new type! Returned value might contain garbage. " << std::endl;

        // Allocate new memory!
        std::unique_ptr<typeToCastTo> newData(new typeToCastTo());

        // Safest (to the best of my knowledge) way of type punning in C++:
        // From Shafik Yaghmour
        // (https://gist.github.com/shafik/848ae25ee209f698763cffee272a58f8)
        std::memcpy(newData.get(), thingToCast, sizeof(typeToCastTo));

        return newData;
    }

    // Read a single primitive value from binary file.
    // Corrects endianness if necessary.
    // Use seekg to move the cursor to the chunk of bytes
    // you want to read.
    // (This function moves the file cursor to the end of
    // the read bytes.)
    template<typename B>
    static B readSinglePrimitive(ifstreamPointer file, std::size_t bytesToRead)
    {
        std::vector<char> rawData;
        // Size of rawData must be >= to sizeof(B) to avoid garbage data when
        // reinterpret casting; if rawData > sizeof(B), will truncate or not
        // depending on endianness, so don't do this. Thus, set sizeof(rawData)
        // to sizeof(B) for portability.
        // Allocate and set all elements to 0:
        rawData.resize(sizeof(B), 0);

        // Add padding if necessary, to avoid garbage data from seeping in.
        std::size_t paddingRequired = 0;
        // sizeof(B) should never be < than bytesToRead if you are a good coder!
        if(sizeof(B) > bytesToRead)
            paddingRequired = sizeof(B) - bytesToRead;

        // file.read() wants a char* and not an unsigned char* for some reason.
        // Data in HFS file is in big-endian!! HFS+ specification.
        file->read(rawData.data()+paddingRequired, bytesToRead);
        checkFileReadErrors(file, bytesToRead, "single primitive");

        // If we are on little endian, invert endianness!
        // (Convert from big-endian to little-endian)
        if(Defs::machineIsLittleEndian)
            // Could of called Defs::makeSafeEndian() on the reinterpreted value
            // itself, but this is simpler in our case.
            std::reverse(std::begin(rawData), std::end(rawData));

        return *reinterpret_cast<B*>(rawData.data());
    }

    // Read an array having elements of size char (and ONLY of size char,
    // don't try to read an array from file with elements larger than char)
    // from binary file.
    template<typename B>
    static std::vector<B> readByteArray(ifstreamPointer file, std::size_t bytesToRead)
    {
        std::vector<char> rawData;
        rawData.resize(bytesToRead, 0);
        file->read(rawData.data(), bytesToRead);
        checkFileReadErrors(file, bytesToRead, "byte array");

        // May or may not copy
        // Type conversion for each element (since they are all
        // of size char).
        return std::vector<B>(rawData.begin(), rawData.end());
    }

    void parseHeader();
    void parseResourceMapFields();
    ReferenceListPointerPair findReferenceListPointer(const std::string& type);

    std::string getResourceName(Defs::addr resourceNameAddr);
    Defs::addr findResourceAddress(const std::string& type, int ID);
    Defs::addr findResourceAddress(const std::string& type, const std::string& name);

public:
    ResourceFork(ifstreamPointer HFSFile, Defs::addr startAddress);
    ~ResourceFork();

    static void checkFileReadErrors(ifstreamPointer file, std::size_t bytesExpected,
                                                 const std::string& dataTryingToReadName);

    std::unique_ptr<char, freeDelete> getResourceData(const std::string& type, int ID, std::size_t* size);

    // Returns unique_ptr to requested type.
    template<typename requestedType>
    std::unique_ptr<requestedType> getResource(const std::string& type, int ID)
    {
        std::size_t size;
        std::unique_ptr<char, freeDelete> rawData = getResourceData(type, ID, &size);

        if(resourceSize != sizeof(requestedType))
        {
            std::cerr << "Size of found resource (type: \"" << type << "\", ID: " <<
                ID << ") is " << resourceSize << " bytes, when " << sizeof(requestedType)
                << " bytes was expected!" << std::endl;
        }

        // Cast from char* to requestedType*, then create and return smart pointer:
        return saferReinterpretCastToHeap<requestedType>(rawData.get(), sizeof(requestedType));
    }
};

} // namespace RESX
#endif // RESX_RESOURCE_FORK_HPP
