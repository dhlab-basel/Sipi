//
// Created by Lukas Rosenthaler on 21/07/16.
//

#include "SipiCommon.h"

namespace Sipi {

    void memcpy(void *to, const void *from, size_t len) {
        register char *toptr = (char *) to;
        register char *fromptr = (char *) from;
        while (toptr < (char *) to + len) {
            *toptr++ = *fromptr++;
        }
    }
}
