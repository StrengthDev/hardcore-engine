
#pragma once

template<class... Ts>
struct VariantVisitor : Ts... {
    using Ts::operator()...;
};
