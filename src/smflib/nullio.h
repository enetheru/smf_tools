//
// Created by nicho on 20/11/2023.
//

#ifndef NULLIO_H
#define NULLIO_H

#include "smfiobase.h"

namespace smflib {

/**
 * \brief NullWriter performs no work, and does nothing.
 */
class NullWriter final : public SMFIOBase {
public:
    void read( std::ifstream& file ) override {}
    size_t write() override { return 0; }
};

}

#endif //NULLIO_H
