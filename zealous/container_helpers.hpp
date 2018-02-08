#pragma once

#include <algorithm>

namespace zealous {
    template<typename _ContainerType, typename _ValueType>
    inline bool Contains( const _ContainerType& container, const _ValueType& value ) {
        const auto start = container.begin();
        const auto end = container.end();
        const auto iter = std::find( start, end, value );
        return iter != end;
    }

    template<typename _ContainerType, typename _Pred>
    inline bool ContainsIf( const _ContainerType& container, _Pred predicate ) {
        const auto start = container.begin();
        const auto end = container.end();
        const auto iter = std::find_if( start, end, predicate );
        return iter != end;
    }

    template<typename _ContainerType, typename _Pred>
    inline bool ContainsIfNot( const _ContainerType& container, _Pred predicate ) {
        const auto start = container.begin();
        const auto end = container.end();
        const auto iter = std::find_if_not( start, end, predicate );
        return iter != end;
    }
}
