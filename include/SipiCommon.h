//
// Created by Lukas Rosenthaler on 21/07/16.
//

#ifndef SIPI_SIPICOMMON_H
#define SIPI_SIPICOMMON_H


#include <cstdlib>

namespace Sipi {

    /*!
     * We had to create a memcpy which will not dump a core if the data
     * is not aligned (problem with ubuntu!)
     *
     * @param to Adress where to copy the data to
     * @param from Adress from where the data is copied
     * @param len Number of bytes to copy
     */
    extern void memcpy(void *to, const void *from, size_t len);

}

#endif //SIPI_SIPICOMMON_H
