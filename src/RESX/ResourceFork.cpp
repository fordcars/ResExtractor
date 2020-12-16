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

#include "RESX/ResourceFork.hpp"

namespace RESX
{

// Note: It would be chill to have a const HFSFile, but since
// you always need to modify a file stream to read from it (move
// the cursor around, etc), a const file stream is pretty much
// useless.
ResourceFork::ResourceFork(ifstreamPointer HFSFile, Defs::addr startAddress)
    : mHFSFile(HFSFile),
    mStartAddr(startAddress),
    mResourceDataZoneAddr(0),
    mResourceMapAddr(0),
    mResourceDataLength(0),
    mResourceMapLength(0),

    mResourceTypeListAddr(0),
    mResourceNameListAddr(0),
    mNumberOfTypesMinusOne(0)
{
    checkFloatingTypes();

    if(mHFSFile->is_open())
    {
        parseHeader();
        parseResourceMapFields();
    } else
    {
        std::cerr << "HFS file is not open! Cannot create resource fork!" << std::endl;
    }
}

ResourceFork::~ResourceFork()
{

}

// Static inline
// To make sure reading floating-types from files will work on this system.
void ResourceFork::checkFloatingTypes()
{
    // Check floating types to make sure they match with PowerPC's!

    static_assert(std::numeric_limits<float>::is_iec559,
                  "Our float is not IEEE 754 compliant! Note: PowerPC uses IEEE 754 compliant floats.");

    // Don't check for short double, since it is the same as double on PowerPC.
    // Btw, wtf is short double.

    static_assert(std::numeric_limits<double>::is_iec559,
                  "Our double is not IEEE 754 compliant! Note: PowerPC uses IEEE 754 compliant doubles.");

    // Don't check for long double, since it is the same as double on PowerPC.
}

void ResourceFork::parseHeader()
{
    // Set cursor to start of resource fork
    mHFSFile->seekg(mStartAddr, std::ios::beg);

    mResourceDataZoneAddr = mStartAddr +
                            readSinglePrimitive<Defs::addr>(mHFSFile, 4UL);
    mResourceMapAddr = mStartAddr +
                            readSinglePrimitive<Defs::addr>(mHFSFile, 4UL);
    mResourceDataLength = readSinglePrimitive<Defs::addr>(mHFSFile, 4UL);
    mResourceMapLength = readSinglePrimitive<Defs::addr>(mHFSFile, 4UL);
}

// Call after passing header!
void ResourceFork::parseResourceMapFields()
{
    // Set cursor to the start of the resource map
    mHFSFile->seekg(mResourceMapAddr, std::ios::beg);
    // Skip reserved and attributes sections
    mHFSFile->seekg(16 + 4 + 2 + 2, std::ios::cur);

    // Documentation was a bit misleading. The resource type list
    // actually starts at the numberOfTypesMinusOne field. Keep this in mind.
    mResourceTypeListAddr = mResourceMapAddr +
                                readSinglePrimitive<Defs::addr>(mHFSFile, 2UL);
    mResourceNameListAddr = mResourceMapAddr +
                                readSinglePrimitive<Defs::addr>(mHFSFile, 2UL);

    // This field follows right after resourceNameListAddr.
    // Using seekg() again just to make it clear that resourceTypeListAddr
    // points to here.
    // This is the same as mHFSFile.seekg(0UL, std::ios::cur);
    mHFSFile->seekg(mResourceTypeListAddr, std::ios::beg);
    mNumberOfTypesMinusOne = readSinglePrimitive<int>(mHFSFile, 2UL); // Can be negative
}

// Find reference list pointer in the resource type list.
ResourceFork::ReferenceListPointerPair ResourceFork::findReferenceListPointer(const std::string& type)
{
    // First: (number of resources -1) of this type in our map.
    // So, initialize to -1 for safety (in-case type is not found).
    ReferenceListPointerPair referenceListPointerPair(-1, 0);
    bool foundType = false;

    // Set cursor to start of resource type list.
    // +2 to skip the numberOfTypesMinusOne field.
    mHFSFile->seekg(mResourceTypeListAddr + 2UL, std::ios::beg);

    // Iterate through all types
    for(int i = 0; i <= mNumberOfTypesMinusOne; i++)
    {
        // Not null-terminated.
        // Types are case sensitive (Apple HFS+ specification).
        std::vector<char> rawString = readByteArray<char>(mHFSFile, 4UL);

        // Makes it all nice and useable.
        std::string readType(rawString.begin(), rawString.end());

        if(type == readType) // String compare
        {
            foundType = true;

            // Number of resources -1 of this type in map
            referenceListPointerPair.first = readSinglePrimitive<int>(mHFSFile, 2UL);

            // Address of reference list for this type.
            referenceListPointerPair.second = mResourceTypeListAddr +
                                readSinglePrimitive<Defs::addr>(mHFSFile, 2UL);
            break;
        } else
        {
            // Not our type!
            // Skip the next 2 fields to go to the next type.
            mHFSFile->seekg(2UL + 2UL, std::ios::cur);
        }
    }

    if(!foundType)
        std::cerr << "Could not find resource type '" << type << "'!" << std::endl;

    return referenceListPointerPair;
}

// Stream position is restored
std::string ResourceFork::getResourceName(Defs::addr resourceNameAddr)
{
    Defs::addr oldAddress = mHFSFile->tellg();
    mHFSFile->seekg(resourceNameAddr, std::ios::beg);
    std::size_t nameLength = readSinglePrimitive<std::size_t>(mHFSFile, 1UL);
    std::vector<char> rawString = readByteArray<char>(mHFSFile, nameLength);

    mHFSFile->seekg(oldAddress, std::ios::beg);

    // Makes it all nice and useable.
    return std::string(rawString.begin(), rawString.end());
}

// Find resource address by ID in the reference list.
Defs::addr ResourceFork::findResourceAddress(const std::string& type, int ID)
{
    ReferenceListPointerPair referenceListPointerPair = findReferenceListPointer(type);

    bool foundResource = false;
    Defs::addr resourceAddress = 0;

    // Move cursor to reference list for this type
    mHFSFile->seekg(referenceListPointerPair.second, std::ios::beg);

    // Iterate through all resources of this type.
    for(int i = 0; i <= referenceListPointerPair.first; i++)
    {
        // Read resource ID
        int readID = readSinglePrimitive<unsigned int>(mHFSFile, 2UL);

        if(readID == ID)
        {
            // Found our resource!
            foundResource = true;

            // Skip resource name (not used here) and resource attributes
            mHFSFile->seekg(2 + 1, std::ios::cur);

            resourceAddress = mResourceDataZoneAddr +
                        readSinglePrimitive<Defs::addr>(mHFSFile, 3UL);
            break;
        } else
        {
            // Not our resource!
            // Go to the next resource.
            mHFSFile->seekg(2 + 1 + 3 + 4, std::ios::cur);
        }
    }

    if(!foundResource)
    {
        std::cerr << "Could not find resource with ID '" << std::to_string(ID) << "'!"
            << std::endl;
    }

    return resourceAddress;
}

// Find resource address by name in the reference list.
Defs::addr ResourceFork::findResourceAddress(const std::string& type, const std::string& name)
{
    ReferenceListPointerPair referenceListPointerPair = findReferenceListPointer(type);

    bool foundResource = false;
    Defs::addr resourceAddress = 0;

    // Move cursor to reference list for this type
    mHFSFile->seekg(referenceListPointerPair.second, std::ios::beg);

    // Iterate through all resources of this type.
    for(int i = 0; i <= referenceListPointerPair.first; i++)
    {
        // Skip resource ID
        mHFSFile->seekg(2, std::ios::cur);
        Defs::addr resourceNameAddr = mResourceNameListAddr +
                        readSinglePrimitive<Defs::addr>(mHFSFile, 2UL);

        // Get resource name
        std::string readName = getResourceName(resourceNameAddr);

        if(readName == name)
        {
            // Found our resource!
            foundResource = true;

            // Skip resource attributes
            mHFSFile->seekg(1, std::ios::cur);

            resourceAddress = mResourceDataZoneAddr +
                        readSinglePrimitive<Defs::addr>(mHFSFile, 3UL);
            break;
        } else
        {
            // Not our resource!
            // Go to the next resource.
            mHFSFile->seekg(1 + 3 + 4, std::ios::cur);
        }
    }

    if(!foundResource)
    {
        std::cerr << "Could not find resource with name '" << name << "'!"
            << std::endl;
    }

    return resourceAddress;
}

// Static
// Use after every fileStream.read()!
// Cerrs nice error messages.
void ResourceFork::checkFileReadErrors(ifstreamPointer file, std::size_t bytesExpected,
                                                 const std::string& dataTryingToReadName)
{
    if(!file->is_open())
        std::cerr << "File not open! Cannot read file stream!" << std::endl;

    if(file->eof() && file->fail())
        std::cerr << "End-of-file reached before reading all requested bytes for " <<
            dataTryingToReadName << "!" << std::endl;
    else if(file->bad())
        std::cerr << "Read error while reading bytes for " << dataTryingToReadName <<
            "!" << std::endl << "Loss of integrity of the stream?" << std::endl;
    else if(file->fail())
        std::cerr << "Internal logical error while reading bytes for " <<
            dataTryingToReadName << "!" << std::endl;

    std::streamsize bytesRead = file->gcount();
    if(bytesRead != bytesExpected)
        std::cerr << "Expected to read " <<  bytesExpected << " bytes for " << dataTryingToReadName <<
            ", but got " << bytesRead << " bytes!" << std::endl;
}

// Get all IDs for resource type.
std::vector<unsigned int> ResourceFork::getResourcesIDs(const std::string& type)
{
    std::vector<unsigned int> IDs;
    ReferenceListPointerPair referenceListPointerPair = findReferenceListPointer(type);

    // Move cursor to reference list for this type
    mHFSFile->seekg(referenceListPointerPair.second, std::ios::beg);

    // Iterate through all resources of this type.
    for(int i = 0; i <= referenceListPointerPair.first; i++)
    {
        // Read resource ID
        int readID = readSinglePrimitive<unsigned int>(mHFSFile, 2UL);
        IDs.push_back(readID);

        // Go to the next resource.
        mHFSFile->seekg(2 + 1 + 3 + 4, std::ios::cur);
    }

    return IDs;
}

// Get all names for resource type.
std::vector<std::string> ResourceFork::getResourcesNames(const std::string& type)
{
    std::vector<std::string> names;
    ReferenceListPointerPair referenceListPointerPair = findReferenceListPointer(type);

    // Move cursor to reference list for this type
    mHFSFile->seekg(referenceListPointerPair.second, std::ios::beg);

    // Iterate through all resources of this type.
    for(int i = 0; i <= referenceListPointerPair.first; i++)
    {
        // Skip resource ID
        mHFSFile->seekg(2, std::ios::cur);
        Defs::addr resourceNameAddr = mResourceNameListAddr +
                        readSinglePrimitive<Defs::addr>(mHFSFile, 2UL);

        // Get resource name
        std::string readName = getResourceName(resourceNameAddr);
        names.push_back(readName);

        // Go to the next resource.
        mHFSFile->seekg(1 + 3 + 4, std::ios::cur);
    }

    return names;
}

// Get resource data from ID.
std::unique_ptr<char, freeDelete> ResourceFork::getResourceData(const std::string& type, int ID, std::size_t* size)
{
    // Find the resource!
    Defs::addr resourceAddress = findResourceAddress(type, ID);
    mHFSFile->seekg(resourceAddress, std::ios::beg);

    std::size_t resourceSize = readSinglePrimitive<std::size_t>(mHFSFile, 4UL);
    /* File cursor now at actual resource data */

    // void* to unique_ptr<char>
    std::unique_ptr<char, freeDelete> rawData(static_cast<char*>(
        std::malloc(resourceSize)
    ));

    // Read the data and store on heap.
    mHFSFile->read(rawData.get(), resourceSize);
    checkFileReadErrors(mHFSFile, resourceSize, "resource");

    *size = resourceSize;
    return rawData;
}

// Get resource data from name.
std::unique_ptr<char, freeDelete> ResourceFork::getResourceData(const std::string& type,
    const std::string& name, std::size_t* size)
{
    // Find the resource!
    Defs::addr resourceAddress = findResourceAddress(type, name);
    mHFSFile->seekg(resourceAddress, std::ios::beg);

    std::size_t resourceSize = readSinglePrimitive<std::size_t>(mHFSFile, 4UL);
    /* File cursor now at actual resource data */

    // void* to unique_ptr<char>
    std::unique_ptr<char, freeDelete> rawData(static_cast<char*>(
        std::malloc(resourceSize)
    ));

    // Read the data and store on heap.
    mHFSFile->read(rawData.get(), resourceSize);
    checkFileReadErrors(mHFSFile, resourceSize, "resource");

    *size = resourceSize;
    return rawData;
}


} // namespace RESX
