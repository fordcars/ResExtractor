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

#include "main.hpp"
#include "ResExtractor.hpp"

#include <cstddef> // For size_t
#include <type_traits> // For is_same
#include <limits>
#include <iostream>
#include <memory>

#include <string>
#include <iomanip>
#include <cstdint>
#include <fstream>

std::string gVersion = "v1.0";
const unsigned READ_LENGTH = 1e6; // Sorry! 1 MB, must be known at compile time.

using Big = long long int;

// Safe types according to CodeWarrior PowerPC documentation
// for reading binary files.
using sShort = std::int16_t;   // Safe short
using sUShort = std::uint16_t; // Safe unsigned short
using sInt = std::int32_t;     // Safe int
using sUInt = std::uint32_t;   // Safe unsigned int
using sLong = std::int32_t;    // Safe long
using sULong = std::uint32_t;  // Safe unsigned long
// For future reference:
// (Metrowerks Inc., CodeWarrior ® C, C , and Assembly Language Reference. p. 28-29.)
// All floating types follow IEEE
// float: 32-bit -> 1.17549e-38 to 3.40282e+38
// short double: 64-bit-> 2.22507e-308 to 1.79769e+308
// double: 64-bit -> 2.22507e-308 to 1.79769e+308
// long double: 64-bit -> 2.22507e-308 to 1.79769e+308
// Note : Since long/short double is the same as double
// on PowerPC, replace all long/short doubles with doubles
// (makes my life much easier).
// (Fun fact: PowerPC doesn't actually support long double at all.)

// Padding on PowerPC according to CodeWarrior:
// (Metrowerks Inc., CodeWarrior® Targeting Mac OS. p. 97.)
// char: 1-byte boundary
// int16_t: 2-byte boundary
// int32_t: 4-byte boundary
// int64_t: 8-byte boundary
// float: 4-byte boundary
// double: 8-byte boundary
// long double: 8-byte boundary
// struct: alignment of largest scalar type in struct

void printHelp()
{
    std::cout <<
        "********************************" << std::endl <<
        "**     ResExtractorCmdLine    **" << std::endl <<
        "**        Version: " << gVersion << "      **" << std::endl <<
        "********************************" << std::endl <<
        std::endl <<
        "Reads a resource from a resource fork file." << std::endl <<
        "Note: errors such as 'End-of-file reached...' and 'Expected to read X bytes...' " <<
        "are normal and can be safely discarded." << std::endl <<
        std::endl <<
        "Usage: ResExtractorCmdLine [-input INPUT_FILE -length BYTES -resourceID ID -resourceType TYPE " <<
        "[-blocksize BYTES] [-output OUTPUT_FILE] [-startblock BLOCK]]" << std::endl <<
        std::endl <<
        " --help, --h                 display help" << std::endl <<
        std::endl <<
        " -blocksize                  set block size in bytes, 4 KiB by default" << std::endl <<
        " -input                      set input file containing resource fork (.hfs or .rsrc)" << std::endl <<
        " -length                     set length (in bytes) of the resource to extract" << std::endl <<
        " -output                     set output file, will print resource to cmdline if unspecified" << std::endl <<
        " -resourceID                 set resource ID to extract" << std::endl <<
        " -resourceType               set resource type to extact" << std::endl <<
        " -startblock                 set first block of resource fork, 0 by default" << std::endl;
}

int main(int argc, char **argv)
{
    // Terminal command, pointer to value to modify, textual type name.
    using argDefinitionTuple = std::tuple<std::string, void*, std::string>;
    using argDefinitionVector = std::vector<argDefinitionTuple>;

    // Modifiable with arguments
    Big blockSize = 4096LL; // Default: 4 kibibytes
    Big lengthToExtract = -1LL;
    Big startBlock = 0LL; // 0 by default

    std::string inputFile;
    std::string outputFile;

    int resourceID = -1;
    std::string resourceType;

    // Wow! So easy!!!!!!!!!!!!!! :ooooo
    argDefinitionVector argDefinitions = {
                    argDefinitionTuple("--help", nullptr, "printHelp()"),
                    argDefinitionTuple("--h", nullptr, "printHelp()"),

                    argDefinitionTuple("-blocksize", &blockSize, "Big"),
                    argDefinitionTuple("-input", &inputFile, "std::string"),
                    argDefinitionTuple("-length", &lengthToExtract, "Big"),
                    argDefinitionTuple("-output", &outputFile, "std::string"),
                    argDefinitionTuple("-resourceID", &resourceID, "int"),
                    argDefinitionTuple("-resourceType", &resourceType, "std::string"),
                    argDefinitionTuple("-startblock", &startBlock, "Big"),
    };

    std::vector<std::string> args(argv, argv+argc);
    if(args.size() <= 1U) // This will never be < 1.
    {
        printHelp();
        return 0; // Quit
    }

    for(argDefinitionTuple argDefinition : argDefinitions)
    {
        std::string command = std::get<0>(argDefinition);
        void* associatedVariable = std::get<1>(argDefinition);
        std::string textualType = std::get<2>(argDefinition);

        // Find arg definition in args
        auto foundStringIt = std::find(args.begin(), args.end(), command);
        if(foundStringIt != args.end())
        {
            // Arg definition command found in given args!

            try
            {
                // Try to use the next argument as a parameter value.
                if(textualType == "std::string")
                    *static_cast<std::string*>(associatedVariable) = *(foundStringIt + 1); // Next arg

                else if(textualType == "int")
                    *static_cast<int*>(associatedVariable) = std::stoi(*(foundStringIt + 1));

                else if(textualType == "unsigned int")
                    *static_cast<unsigned int*>(associatedVariable) = std::stoul(*(foundStringIt + 1));

                else if(textualType == "std::size_t")
                    *static_cast<std::size_t*>(associatedVariable) = std::stoul(*(foundStringIt + 1));

                else if(textualType == "Big")
                    *static_cast<Big*>(associatedVariable) = std::stoll(*(foundStringIt + 1));

                else if(textualType == "float")
                    *static_cast<float*>(associatedVariable) = std::stof(*(foundStringIt + 1));
                else if(textualType == "printHelp()")
                {
                    printHelp();
                    return 0; // Quit
                }

            } catch(const std::invalid_argument& ia)
            {
                std::cerr << "Invalid value for '" + command + "'!"
                    << std::endl;
                return 1;
            }
        }
    }

    // Do errors:
    if(inputFile.empty())
    {
        std::cerr << "Error: input file not specified, you must specify it with -input" << std::endl;
        return 1;
    }

    if(lengthToExtract == -1LL)
    {
        std::cerr << "Error: length not specified, you must specify it with -length" << std::endl;
        return 1;
    }

    if(resourceID == -1)
    {
        std::cerr << "Error: resource ID not specified, you must specify it with -resourceID" << std::endl;
        return 1;
    }

    if(resourceType.empty())
    {
        std::cerr << "Error: resource type not specified, you must specify it with -resourceType" << std::endl;
        return 1;
    }

    // The only way to avoid problems casting to static array later on.
    struct ReadData
    {
        char data[READ_LENGTH];
    };

    RESX::File myFile(inputFile, blockSize);
    std::unique_ptr<struct ReadData> resourceData = myFile.loadResourceFork(startBlock).getResource<struct ReadData>(resourceType, resourceID);

    // Print resource if outputFile is not specified.
    if(outputFile.empty())
    {
        // From https://stackoverflow.com/questions/7639656/getting-a-buffer-into-a-stringstream-in-hex-representation/7639754#7639754
        std::cout << std::hex << std::setfill('0');
        for(std::size_t i = 0; i < lengthToExtract; ++i)
        {
            // Reinterpret casting to unsigned char to avoid << printing a negative hex value, while
            // making sure we keep the exact same value bitwise.
            // Casting to unsigned int to tell << we want a number, not an ASCII character.
            //
            // (Note: reinterpret_cast can only return reference or pointer, since it does not compile
            // to any CPU instructions at all, and only exists for the compiler. Thus, it cannot return
            // by value, but can only "return" a reference or pointer (which tells the compiler to use the
            // variable as is, without any implicit conversions or compiler errors).)
            std::cout << std::setw(2) << static_cast<unsigned>(reinterpret_cast<unsigned char&>(resourceData->data[i]));

            if((i+1)%8 == 0)
                std::cout << "  ";
            else
                std::cout << ' ';

            if((i+1)%16 == 0)
                std::cout << std::endl;
        }

        std::cout << std::endl;
    } else
    {
        std::ofstream file(outputFile, std::ofstream::out | std::ofstream::trunc);

        if(file.fail())
        {
            std::cerr << "Error: cannot open file '" << outputFile << "' for writing!" << std::endl;
            return 1;
        }

        file.write(resourceData->data, lengthToExtract);
        if(file.fail())
        {
            std::cerr << "Error: writing to  '" << outputFile << "' failed!" << std::endl;
            return 1;
        }
    }
}
