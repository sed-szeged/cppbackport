#ifndef BACKPORT_MODIFY_VARIADIC_UTIL_H
#define BACKPORT_MODIFY_VARIADIC_UTIL_H

#include <string>

// The following is a templated list structure that is used when transforming variadic functions
const std::string variadicInclude =
R"varInc(
//-----------------------------------------------------------------
#ifndef VARIADIC_CONVERSION_UTIL
#define VARIADIC_CONVERSION_UTIL
template <typename T, typename Next> struct VariadicTemplateList;
struct VariadicTemplateListEmpty;

template <typename VariadicTemplateList, typename U>
struct variadicTemplateList_result;

template <typename U>
struct variadicTemplateList_result<VariadicTemplateListEmpty, U> {
    typedef VariadicTemplateList<U, VariadicTemplateListEmpty> type;
};

template <typename T, typename U>
struct variadicTemplateList_result<VariadicTemplateList<T, VariadicTemplateListEmpty>, U> {
    typedef VariadicTemplateList<T, VariadicTemplateList<U, VariadicTemplateListEmpty> > type;
};

template <typename T, typename Next, typename U>
struct variadicTemplateList_result<VariadicTemplateList<T, Next>, U> {
    typedef VariadicTemplateList<T, typename variadicTemplateList_result<Next, U>::type> type;
};

template <typename T, typename Next>
struct VariadicTemplateList {
    VariadicTemplateList(T t, Next n) : value(t), next(n) {}

    T value;
    Next next;

    template <typename U>
    typename variadicTemplateList_result<VariadicTemplateList, U>::type operator()(U u) {
        typedef typename variadicTemplateList_result<VariadicTemplateList, U>::type Result;
        return Result(value, next(u));
    }
};

struct VariadicTemplateListEmpty {
    template <typename U>
    VariadicTemplateList<U, VariadicTemplateListEmpty> operator()(U u) {
        return VariadicTemplateList<U, VariadicTemplateListEmpty>(u, VariadicTemplateListEmpty());
    }
};

template <typename T>
VariadicTemplateList<T, VariadicTemplateListEmpty> variadicTemplateList(T t) {
    return VariadicTemplateList<T, VariadicTemplateListEmpty>(t, VariadicTemplateListEmpty());
}
#endif // VARIADIC_CONVERSION_UTIL
//-----------------------------------------------------------------
)varInc";

#endif // BACKPORT_MODIFY_VARIADIC_UTIL_H
