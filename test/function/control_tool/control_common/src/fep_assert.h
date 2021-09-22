/**
 * @copyright
 * @verbatim
 * Copyright @ 2021 VW Group. All rights reserved.
 *
 *     This Source Code Form is subject to the terms of the Mozilla
 *     Public License, v. 2.0. If a copy of the MPL was not distributed
 *     with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * If it is not possible or desirable to put the notice in a particular file, then
 * You may include the notice in a location (such as a LICENSE file in a
 * relevant directory) where a recipient would be likely to look for such a notice.
 *
 * You may add additional accurate notices of copyright ownership.
 *
 * @endverbatim
 */

#ifndef FEP_ASSERT_H
#define FEP_ASSERT_H

#include <gtest/gtest.h>

#define FEP_ASSERT_NO_THROW(statement)                                                             \
    GTEST_AMBIGUOUS_ELSE_BLOCKER_                                                                  \
    {                                                                                              \
        std::string what;                                                                          \
        if (::testing::internal::AlwaysTrue()) {                                                   \
            try {                                                                                  \
                GTEST_SUPPRESS_UNREACHABLE_CODE_WARNING_BELOW_(statement);                         \
            }                                                                                      \
            catch (const std::runtime_error& exception) {                                          \
                what = exception.what();                                                           \
                goto GTEST_CONCAT_TOKEN_(gtest_label_testnothrow_, __LINE__);                      \
            }                                                                                      \
            catch (...) {                                                                          \
                goto GTEST_CONCAT_TOKEN_(gtest_label_testnothrow_, __LINE__);                      \
            }                                                                                      \
        }                                                                                          \
        else {                                                                                     \
            GTEST_CONCAT_TOKEN_(gtest_label_testnothrow_, __LINE__)                                \
                : what = " Expected: " #statement " doesn't throw an exception.\n"                  \
                         "  Actual: it throws.\n"                                                  \
                         "  What: " +                                                              \
                         what;                                                                     \
           return testing::AssertionFailure() << __FILE__ << __LINE__ << what;                     \
        }                                                                                          \
    }                                                                               

#define FEP_RETURN_ON_FAIL(statement)               \
    {                                               \
    testing::AssertionResult assertRes = statement; \
    if (!assertRes)                                 \
    {                                               \
        return assertRes;                           \
    }                                               \
    }                                               

#endif
