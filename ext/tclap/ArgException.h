// -*- Mode: c++; c-basic-offset: 4; tab-width: 4; -*-

/******************************************************************************
 *
 *  file:  ArgException.h
 *
 *  Copyright (c) 2003, Michael E. Smoot .
 *  Copyright (c) 2017 Google LLC
 *  All rights reserved.
 *
 *  See the file COPYING in the top directory of this distribution for
 *  more information.
 *
 *  THE SOFTWARE IS PROVIDED _AS IS_, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#ifndef TCLAP_ARG_EXCEPTION_H
#define TCLAP_ARG_EXCEPTION_H

#include <exception>
#include <string>
#include <utility>

namespace TCLAP {

/**
 * A simple class that defines and argument exception.  Should be caught
 * whenever a CmdLine is created and parsed.
 */
class ArgException : public std::exception {
public:
    /**
     * Constructor.
     * \param text - The text of the exception.
     * \param id - The text identifying the argument source.
     * \param td - Text describing the type of ArgException it is.
     * of the exception.
     */
    explicit ArgException(std::string text = "undefined exception",
                          std::string id   = "undefined",
                          std::string td   = "Generic ArgException")
        : _errorText(std::move(text)),
          _argId(std::move(id)),
          _typeDescription(std::move(td)) {}

    /**
     * Destructor.
     */
    ~ArgException() noexcept override = default;

    /**
     * Returns the error text.
     */
    [[nodiscard]] std::string error() const { return _errorText; }

    /**
     * Returns the argument id.
     */
    [[nodiscard]] std::string argId() const {
     return _argId == "undefined" ? " " : "Argument: " + _argId;
    }

    /**
     * Returns the arg id and error text.
     */
    [[nodiscard]] const char *what() const noexcept override {
        static std::string ex;
        ex = _argId + " -- " + _errorText;
        return ex.c_str();
    }

    /**
     * Returns the type of the exception.  Used to explain and distinguish
     * between different child exceptions.
     */
    [[nodiscard]] std::string typeDescription() const { return _typeDescription; }

private:
    /**
     * The text of the exception message.
     */
    std::string _errorText;

    /**
     * The argument related to this exception.
     */
    std::string _argId;

    /**
     * Describes the type of the exception.  Used to distinguish
     * between different child exceptions.
     */
    std::string _typeDescription;
};

/**
 * Thrown from within the child Arg classes when it fails to properly
 * parse the argument it has been passed.
 */
class ArgParseException final : public ArgException {
public:
    /**
     * Constructor.
     * \param text - The text of the exception.
     * \param id - The text identifying the argument source
     * of the exception.
     */
    explicit ArgParseException(const std::string &text = "undefined exception",
                               const std::string &id   = "undefined")
        : ArgException(text, id,std::string("Exception found while parsing the value the Arg has been passed.")) {}
};

/**
 * Thrown from CmdLine when the arguments on the command line are not
 * properly specified, e.g. too many arguments, required argument missing, etc.
 */
class CmdLineParseException final : public ArgException {
public:
    /**
     * Constructor.
     * \param text - The text of the exception.
     * \param id - The text identifying the argument source
     * of the exception.
     */
    explicit CmdLineParseException(const std::string &text = "undefined exception",
                                   const std::string &id   = "undefined")
        : ArgException(text, id,std::string("Exception found when the values on the command line do not meet the requirements of the defined Args.")) {}
};

/**
 * Thrown from Arg and CmdLine when an Arg is improperly specified, e.g.
 * same flag as another Arg, same name, etc.
 */
class SpecificationException final : public ArgException {
public:
    /**
     * Constructor.
     * \param text - The text of the exception.
     * \param id - The text identifying the argument source
     * of the exception.
     */
    explicit SpecificationException(const std::string &text = "undefined exception",
                                    const std::string &id   = "undefined")
        : ArgException(text, id,std::string("Exception found when an Arg object is improperly defined by the developer.")) {}
};

/**
 * Thrown when TCLAP thinks the program should exit.
 *
 * For example after parse error this exception will be thrown (and
 * normally caught). This allows any resource to be clened properly
 * before exit.
 *
 * If exception handling is disabled (CmdLine::setExceptionHandling),
 * this exception will propagate to the call site, allowing the
 * program to catch it and avoid program termination, or do it's own
 * cleanup. See for example, https://sourceforge.net/p/tclap/bugs/29.
 */
class ExitException final : std::exception {
public:
    explicit ExitException(const int estat) : _estat(estat) {}

    [[nodiscard]] int getExitStatus() const { return _estat; }

private:
    int _estat;
};

}  // namespace TCLAP

#endif  // TCLAP_ARG_EXCEPTION_H
