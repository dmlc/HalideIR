#ifndef HALIDE_ERROR_H
#define HALIDE_ERROR_H

#include <sstream>
#include <stdexcept>

#include "Debug.h"

namespace Halide {

/** Query whether Halide was compiled with exceptions. */
EXPORT bool exceptions_enabled();

/** A base class for Halide errors. */
struct Error : public std::runtime_error {
    // Give each class a non-inlined constructor so that the type
    // doesn't get separately instantiated in each compilation unit.
    EXPORT Error(const std::string &msg);
};

/** An error that occurs while running a JIT-compiled Halide pipeline. */
struct RuntimeError : public Error {
    EXPORT RuntimeError(const std::string &msg);
};

/** An error that occurs while compiling a Halide pipeline that Halide
 * attributes to a user error. */
struct CompileError : public Error {
    EXPORT CompileError(const std::string &msg);
};

/** An error that occurs while compiling a Halide pipeline that Halide
 * attributes to an internal compiler bug, or to an invalid use of
 * Halide's internals. */
struct InternalError : public Error {
    EXPORT InternalError(const std::string &msg);
};

/** CompileTimeErrorReporter is used at compile time (*not* runtime) when
 * an error or warning is generated by Halide. Note that error() is called
 * a fatal error has occurred, and returning to Halide may cause a crash;
 * implementations of CompileTimeErrorReporter::error() should never return.
 * (Implementations of CompileTimeErrorReporter::warning() may return but
 * may also abort(), exit(), etc.)
 */
class CompileTimeErrorReporter {
public:
    virtual ~CompileTimeErrorReporter() {}
    virtual void warning(const char* msg) = 0;
    virtual void error(const char* msg) = 0;
};

/** The default error reporter logs to stderr, then throws an exception
 * (if WITH_EXCEPTIONS) or calls abort (if not). This allows customization
 * of that behavior if a more gentle response to error reporting is desired.
 * Note that error_reporter is expected to remain valid across all Halide usage;
 * it is up to the caller to ensure that this is the case (and to do any
 * cleanup necessary).
 */
EXPORT void set_custom_compile_time_error_reporter(CompileTimeErrorReporter* error_reporter);

namespace Internal {


struct ErrorReport {
    std::ostringstream *msg;
    const char *file;
    const char *condition_string;
    int line;
    bool condition;
    bool user;
    bool warning;
    bool runtime;

    ErrorReport(const char *f, int l, const char *cs, bool c, bool u, bool w, bool r) :
        msg(nullptr), file(f), condition_string(cs), line(l), condition(c), user(u), warning(w), runtime(r) {
        if (condition) return;
        msg = new std::ostringstream;
        const std::string source_loc;

        if (user) {
            // Only mention where inside of libHalide the error tripped if we have debug level > 0
            debug(1) << "User error triggered at " << f << ":" << l << "\n";
            if (condition_string) {
                debug(1) << "Condition failed: " << condition_string << "\n";
            }
            if (warning) {
                (*msg) << "Warning";
            } else {
                (*msg) << "Error";
            }
            if (source_loc.empty()) {
                (*msg) << ":\n";
            } else {
                (*msg) << " at " << source_loc << ":\n";
            }

        } else {
            (*msg) << "Internal ";
            if (warning) {
                (*msg) << "warning";
            } else {
                (*msg) << "error";
            }
            (*msg) << " at " << f << ":" << l;
            if (!source_loc.empty()) {
                (*msg) << " triggered by user code at " << source_loc << ":\n";
            } else {
                (*msg) << "\n";
            }
            if (condition_string) {
                (*msg) << "Condition failed: " << condition_string << "\n";
            }
        }
    }

    template<typename T>
    ErrorReport &operator<<(T x) {
        if (condition) return *this;
        (*msg) << x;
        return *this;
    }

    /** When you're done using << on the object, and let it fall out of
     * scope, this errors out, or throws an exception if they are
     * enabled. This is a little dangerous because the destructor will
     * also be called if there's an exception in flight due to an
     * error in one of the arguments passed to operator<<. We handle
     * this by only actually throwing if there isn't an exception in
     * flight already.
     */
#if __cplusplus >= 201100 || _MSC_VER >= 1900
    ~ErrorReport() noexcept(false) {
#else
    ~ErrorReport() {
#endif
        if (condition) return;
        explode();
    }

    EXPORT void explode();
};

#define internal_error            Halide::Internal::ErrorReport(__FILE__, __LINE__, nullptr, false, false, false, false)
#define internal_assert(c)        Halide::Internal::ErrorReport(__FILE__, __LINE__, #c,   c,     false, false, false)
#define user_error                Halide::Internal::ErrorReport(__FILE__, __LINE__, nullptr, false, true, false, false)
#define user_assert(c)            Halide::Internal::ErrorReport(__FILE__, __LINE__, #c,   c,     true, false, false)
#define user_warning              Halide::Internal::ErrorReport(__FILE__, __LINE__, nullptr, false, true, true, false)
#define halide_runtime_error      Halide::Internal::ErrorReport(__FILE__, __LINE__, nullptr, false, true, false, true)

// The nicely named versions get cleaned up at the end of Halide.h,
// but user code might want to do halide-style user_asserts (e.g. the
// Extern macros introduce calls to user_assert), so for that purpose
// we define an equivalent macro that can be used outside of Halide.h
#define _halide_user_assert(c)     Halide::Internal::ErrorReport(__FILE__, __LINE__, #c, c, true, false, false)

// N.B. Any function that might throw a user_assert or user_error may
// not be inlined into the user's code, or the line number will be
// misattributed to Halide.h. Either make such functions internal to
// libHalide, or mark them as NO_INLINE.

}

}

#endif
