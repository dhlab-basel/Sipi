//
// Created by Lukas Rosenthaler on 04.06.17.
//

#ifndef SIPI_SIPIFILENAMEHASH_H
#define SIPI_SIPIFILENAMEHASH_H

#include <string>
#include <vector>

/*!
 * Create a hash value of a filename consisting of a vector of chars 'A'-'Z'
 *
 * Usually there should not be too many files in one directory. 1) it may – depending
 * on the file system used – degrade performance severely, and 2) it makes operations
 * like rsync, file listings etc. cumbersome if there are thousands of files in
 * one directroy. A common way is to create a tree of subdirectories. In SIPI we use
 * subdirectories named 'A'-'Z' and a configurable number of levels of subdirectories.
 * In order to calculate the path efficiently we use avery prinitive, simple custom
 * hash function whichs "returns" a vector of the characters 'A'-'Z'. We overload
 * the [] operator in order to access the characters.
 *
 * The class may throw a SipiError!
 */
class SipiFilenameHash {
private:
    const int seed = 137;   //!< an arbitrary seed value (no special meaning...)
    const int numchars = 26;
    const unsigned int modval = 308915776; // (26^6)
    const int hash_len = 6;
    std::vector<char> *hash; //!< Pointer to a vector of chars


public:
    /*!
     * Constructor for hash vector
     *
     * @param name Filename of file for which the "hash" should be calculated
     * @param hash_len Number of directory levels. Must be in the range (inklusive) of [1..6]!
     */
    SipiFilenameHash(const std::string &name);

    /*!
     * Copy constructor (makes deep copy)
     *
     * @param other Reference to SipiFilenameHash instance
     */
    SipiFilenameHash(const SipiFilenameHash& other);

    /*!
     * Assignment copy operator (deep copy)
     *
     * @param other Reference to SipiFilenameHash instance
     * @return
     */
    SipiFilenameHash& operator=(const SipiFilenameHash& other);

    /*!
     * Destructor which frees all allocated memory (pointer to vector)
     */
    virtual ~SipiFilenameHash();


    /*!
     * Index operator to access hash character
     *
     * @param index Number between [0..5] inclusive
     *
     * @return Character in the range 'A' to 'Z'
     */
    char &operator[] (int index);

    static int check_levels(const std::string &path);

    static void migrateToLevels(const std::string &imgdir, int levels);
};


#endif //SIPI_SIPIFILENAMEHASH_H
