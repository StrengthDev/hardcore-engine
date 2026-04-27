
#pragma once

template<const unsigned I, typename... Args>
struct ArgGetter;

template<typename Arg, typename... Args>
struct ArgGetter<0, Arg, Args...> {
    using Type = Arg;
};

template<const unsigned I, typename Arg, typename... Args>
struct ArgGetter<I, Arg, Args...> {
    using Type = ArgGetter<I - 1, Args...>::Type;
};

template<typename T>
struct FunctionSignature;

template<typename R, typename... Args>
struct FunctionSignature<R(*)(Args...)> {
    template<const unsigned I>
    using Arg = ArgGetter<I, Args...>::Type;

    using Return = R;
};

template<typename R, typename... Args>
struct FunctionSignature<R(**)(Args...)> {
    template<const unsigned I>
    using Arg = ArgGetter<I, Args...>::Type;

    using Return = R;
};

template<typename O, typename R, typename... Args>
struct FunctionSignature<R(O::*)(Args...) noexcept> {
    template<const unsigned I>
    using Arg = ArgGetter<I, Args...>::Type;

    using Return = R;

    using Owner = O;
};

template<typename O, typename R, typename... Args>
struct FunctionSignature<R(*O::*)(Args...)> {
    template<const unsigned I>
    using Arg = ArgGetter<I, Args...>::Type;

    using Return = R;

    using Owner = O;
};
