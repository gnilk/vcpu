//
// Created by gnilk on 16.05.24.
//

#ifndef VCPU_BITMASKFLAGOPERATORS_H
#define VCPU_BITMASKFLAGOPERATORS_H

#include <type_traits>

namespace gnilk {
    namespace vcpu {

        //
        // Defines a template for generic flag operators for enum class types
        //
        // Define your enum class like:
        // enum class MyFlags {
        //    flag_a = 0x01,        // must be bit's
        //    flag_b = 0x02,
        // };
        //
        // then:
        //
        //        template<>
        //        struct EnableBitmaskOperators<MyFlags> {
        //            static const bool enable = true;
        //        };
        //
        // the operators '|' and '&' will be defined..
        //

        template<typename E>
        struct EnableBitmaskOperators {
            static const bool enable = false;
        };

        template<typename E>
        typename std::enable_if<EnableBitmaskOperators<E>::enable, E>::type
        inline constexpr operator|(E lhs, E rhs) {
            typedef typename std::underlying_type<E>::type underlying;
            return static_cast<E>(
                    static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
        }

        template<typename E>
        typename std::enable_if<EnableBitmaskOperators<E>::enable, bool>::type
        inline constexpr operator&(E lhs, E rhs) {
            typedef typename std::underlying_type<E>::type underlying;
            return (static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
        }
    }

}

#endif //VCPU_BITMASKFLAGOPERATORS_H
