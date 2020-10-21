#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <system_error>
#include <string>

#define XSTR(a) STR(a)
#define STR(a) #a

/// Throw std::system_error with function name and stringified expr as
/// error message if expr evaluates to -1

#define CHECK(expr)							\
	({								\
		auto ret = (expr);					\
		if (ret == -1)						\
			throw std::system_error(			\
                                errno, std::generic_category(),		\
                                __FILE__ ":" XSTR(__LINE__) ": " #expr);	\
		ret;							\
	})

/// Throw std::system_error with given message if expr evaluates to -1
#define CHECK_MSG(expr, message)                                                                   \
    ({                                                                                             \
        auto ret = (expr);                                                                         \
        if (ret == -1)                                                                             \
            throw std::system_error(errno, std::generic_category(), message);                      \
        ret;                                                                                       \
    })

#endif
