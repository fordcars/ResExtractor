# ResExtractor
A library/utility for extracting resources from a HFS+ resource fork.

# Limitations
* The resource fork which contains the resource you would like to extract must be on one single extent.

# Usage
    ResExtractorCmdLine -input INPUT_FILE -resourceID ID -resourceType TYPE 
       [-blocksize BYTES] [-output OUTPUT_FILE] [-startblock BLOCK]

     --help, --h                 display help

     -blocksize                  set block size in bytes, 4 KiB by default
     -input                      set input file containing resource fork (.hfs or .rsrc)
     -output                     set output file, will print resource to cmdline if unspecified
     -resourceID                 set resource ID to extract
     -resourceType               set resource type to extact
     -startblock                 set first block of resource fork, 0 by default
