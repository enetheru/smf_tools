#ifndef ARGS_H
#define ARGS_H

#include <tclap/ArgGroup.h>
#include <tclap/MultiSwitchArg.h>
#include <tclap/ValueArg.h>


#include <fmt/core.h>
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

using Path = std::filesystem::path;

using TCLAP::ArgGroup;
using TCLAP::ValueArg;
using TCLAP::SwitchArg;
using TCLAP::MultiSwitchArg;

using TCLAP::Constraint;
using TCLAP::CheckResult;

class NamedGroup final : public ArgGroup {
public:
    NamedGroup() = default;
    explicit NamedGroup( const std::string& name ) : ArgGroup( name ) {}

    bool validate() override {
        return false; /* All good */
    }

    [[nodiscard]] bool isExclusive() const override { return false; }
    [[nodiscard]] bool isRequired() const override { return false; }

    [[nodiscard]] std::string getName() const override { return _name; }
};

template< class T >
class RangeConstraint final : public Constraint< T > {
    using RetVal = typename Constraint< T >::RetVal;
    std::string _name;
    CheckResult _failureMode = CheckResult::HARD_FAILURE;
    T _lowerBound, _upperBound, _modulo;

public:
    RangeConstraint( const T lower_bound, const T upper_bound, const T modulo = 1, const bool soft = false )
        : _name( __func__ ), _failureMode( soft ? CheckResult::SOFT_FAILURE : CheckResult::HARD_FAILURE ),
          _lowerBound( lower_bound ), _upperBound( upper_bound ), _modulo( modulo ) {}

    [[nodiscard]] std::string description() const override {
        return fmt::format(
                           "{} - values must be within ({} -> {}), and a multiple of {}",
                           _name, _lowerBound, _upperBound, _modulo );
    }

    [[nodiscard]] RetVal check( const ValueArg< T >& arg ) const override {
        std::string msg;
        bool fail = false;
        if( arg.getValue() < _lowerBound ) {
            fail = true;
            msg += ", lower bound violation";
        }
        if( arg.getValue() > _upperBound ) {
            fail = true;
            msg += ", upper bound violation";
        }
        if( static_cast< long long >(arg.getValue()) % static_cast< long long >(_modulo) ) {
            fail = true;
            msg += ", modulo violation";
        }
        msg += "\n" + description();
        return { fail ? _failureMode : CheckResult::SUCCESS, msg };
    }
};

class FileConstraint final : public Constraint<Path> {
public:
    [[nodiscard]] std::string description() const override {
        return "FileConsraint - tests whether a file exists";
    };
    [[nodiscard]] RetVal check( const ValueArg<std::filesystem::path>& arg ) const override {
        if( exists( arg.getValue() ) ) {
            SPDLOG_TRACE("FileConstraint Success, file exists: {}", arg.getValue().string() );
            return {CheckResult::SUCCESS, "OK" };
        }

        return { CheckResult::HARD_FAILURE, "File does not exist" };
    }
};

class IsFile_SMF final : public Constraint<Path> {
public:
    [[nodiscard]] std::string description() const override {
        return "FileConsraint - tests whether a file exists";
    };
    [[nodiscard]] RetVal check( const ValueArg<std::filesystem::path>& arg ) const override {
        if(! exists( arg.getValue() ) ) {
            return {CheckResult::HARD_FAILURE, "File does not exist"};
        }
        SPDLOG_TRACE("IsFile_SMF: File exists: {}", arg.getValue().string() );

        std::ifstream file( arg.getValue() );
        if( file.fail() ) {
            return {CheckResult::HARD_FAILURE, "Unable to open file for reading"};
        }
        std::array<char,16> magic{};
        file.read( magic.data(), 16 );
        file.close();
        if( strcmp( magic.data(), "spring map file" ) != 0 ){
            return {CheckResult::HARD_FAILURE, "file is not a 'spring map file'"};
        }
        return { CheckResult::SUCCESS, "OK" };
    }
};

struct ImageSpec {
    int width,height,channels,pixelFormat;
};



#endif //ARGS_H
