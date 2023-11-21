//
// Created by nicho on 20/11/2023.
//

#ifndef ARGS_OIIO_H
#define ARGS_OIIO_H

#include <OpenImageIO/imageio.h>


class ImageConstraint final : public Constraint<Path> {
    std::string _name;
    CheckResult _failureMode = CheckResult::HARD_FAILURE;
    const OIIO::ImageSpec &_spec;
public:
    explicit ImageConstraint( const OIIO::ImageSpec *spec )
    : _name(__func__), _spec( *spec )  {}

    [[nodiscard]] std::string description() const override{
        return fmt::format( "{} - {}x{}:{} {}",
            _name,  _spec.width, _spec.height, _spec.nchannels, _spec.format );
    }

    [[nodiscard]] RetVal check( const ValueArg<Path>& arg ) const override {
        // Open Image
        // Test against values
        bool fail{};
        std::string msg;
        return { fail ? _failureMode : CheckResult::SUCCESS, msg };
    }
};

#endif //ARGS_OIIO_H
