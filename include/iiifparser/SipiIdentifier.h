//
// Created by Lukas Rosenthaler on 2019-05-23.
//

#ifndef SIPI_SIPIIDENTIFIER_H
#define SIPI_SIPIIDENTIFIER_H

#include <string>

namespace Sipi {
    class SipiIdentifier {
    private:
        std::string identifier;
        int page;
    public:
        inline SipiIdentifier() {
            page = 0;
        }

        SipiIdentifier(const std::string &str);

        const std::string &getIdentifier() const {
            return identifier;
        }

        int getPage() const {
            return page;
        }
    };
}
#endif //SIPI_SIPIIDENTIFIER_H
