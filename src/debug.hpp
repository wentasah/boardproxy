// Copyright (C) 2021 Michal Sojka <michal.sojka@cvut.cz>
// 
// This file is part of boardproxy.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
