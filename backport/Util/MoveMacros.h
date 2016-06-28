#ifndef BACKPORT_MOVE_MACROS
#define BACKPORT_MOVE_MACROS

#include <map>
#include <string>

enum move_keywords { movable_and_copyable_macro, moveable_but_not_copyable_macro, std_move, rvalue_ref_macro, assignment_op_macro, one_time_include };
enum implementation{ boost, cxxomfort, backport_impl };

static implementation const implementationKind = backport_impl;

static std::map<implementation, std::map<move_keywords, std::string> > get_move_replace_to() {
    std::map<implementation, std::map<move_keywords, std::string> > result;

    result[boost][movable_and_copyable_macro] = "BOOST_COPYABLE_AND_MOVABLE";
    result[boost][moveable_but_not_copyable_macro] = "BOOST_MOVABLE_BUT_NOT_COPYABLE";
    result[boost][std_move] = "::boost::move";
    result[boost][rvalue_ref_macro] = "BOOST_RV_REF";
    result[boost][assignment_op_macro] = "BOOST_COPY_ASSIGN_REF";
    //result[boost][return_move_macro] = "BOOST_MOVE_RET";
    result[boost][one_time_include] = "\n#define BOOST_NO_CXX11_DELETED_FUNCTIONS\n\n#define BOOST_NO_CXX11_RVALUE_REFERENCES\n\n#include <boost/move/core.hpp>\n#include <boost/move/move.hpp>\n#include <boost/move/algorithm.hpp>\n";

    result[cxxomfort][movable_and_copyable_macro] = "CXXO_COPYABLE_MOVABLE";
    result[cxxomfort][moveable_but_not_copyable_macro] = "CXXO_NONCOPYABLE_MOVABLE";
    result[cxxomfort][std_move] = "::std::move";
    result[cxxomfort][rvalue_ref_macro] = "CXXO_RV_REF";
    result[cxxomfort][assignment_op_macro] = "CXXO_COPY_ASSIGN_REF";
    result[cxxomfort][one_time_include] = "\n#include <backport/test/cxxomfort/cxxomfort.hpp>\n";
    
    result[backport_impl][movable_and_copyable_macro] = "";
    result[backport_impl][moveable_but_not_copyable_macro] = "";
    result[backport_impl][std_move] = "::backport::move";
    result[backport_impl][rvalue_ref_macro] = "";
    result[backport_impl][assignment_op_macro] = "";
    result[backport_impl][one_time_include] = "\n/*BACKPORT MOVE TOOLS BEGIN*/\n"
        "\n#ifndef BACKPORT_MOVE_TOOLS_DEFINED\n"
        "\n#define BACKPORT_MOVE_TOOLS_DEFINED\n"
        "namespace backport {\n"
        /*"template<bool cond, class T1, class T2> struct if_c {typedef T1 value;};\n"
        "template<class T1, class T2> struct if_c<false, T1, T2> {typedef T2 value;};\n"
        "template<class T> class nat {typedef T _____backport_move_nat_typedef_differentiator; public: operator T &() {return *reinterpret_cast<T*>(this);} operator T const &() const {return *reinterpret_cast<T const*>(this);} };\n"*/
        "\ntemplate<class T> class rv {\n"     // class rv
        "\nprivate:\n"
        "\nT _placeholder;\n"
        "\nrv();\n"
        "\n~rv();\n"
        "\nrv(rv const&);\n"
        "\nvoid operator=(rv const&);\n"
        "\n};\n"
        "\ntemplate<class T>\n"     // move func
        "\nrv<T>& move(T const &x)\n"
        "\n{ return *reinterpret_cast<rv<T> *>(reinterpret_cast<T*>(&const_cast<char &>(reinterpret_cast<const volatile char &>(static_cast<T const &>(x))))); }\n"
        "\ntemplate<class T>\n"
        "\nrv<T>& move(rv<T> &x)\n"
        "\n{ return x; }\n"
        /*"\n#define BACKPORT_RV_REF(TYPE) \\\n" // macros
        "::backport::rv<TYPE>\n"
        "\n#define BACKPORT_COPY_ASSIGN_REF(TYPE) \\\n"
        "::backport::rv<TYPE> const & \n"
        "\n#define BACKPORT_COPYABLE_AND_MOVABLE(TYPE) \\\n"
        "public: \\\n"
        "TYPE& operator=(TYPE &t) \\\n"
        "{ this->operator=(static_cast<const ::backport::rv<TYPE> &>(const_cast<const TYPE &>(t))); return *this; } \\\n"
        "operator ::backport::rv<TYPE>&() \\\n"
        "{ return *static_cast<::backport::rv<TYPE> *>(this); } \\\n"
        "operator ::backport::rv<TYPE>&() const \\\n"
        "{ return *static_cast<::backport::rv<TYPE> const *>(this); } \\\n"
        "private: \n"
        "\n#define BACKPORT_MOVABLE_BUT_NOT_COPYABLE(TYPE) \\\n"
        "private: \\\n"
        "TYPE(TYPE &);\\\n"
        "TYPE &operator=(TYPE &); \\\n"
        "public: \\\n"
        "operator ::backport::rv<TYPE>&() \\\n"
        "{ return *static_cast<::backport::rv<TYPE> *>(this); } \\\n"
        "operator ::backport::rv<TYPE>&() const \\\n"
        "{ return *static_cast<::backport::rv<TYPE> const *>(this); } \\\n"
        "private: \n"*/
        "\n}\n"
        //"namespace std {template<class T> T &move(T &x) { return x; }}"
        "\n#endif // not defined BACKPORT_MOVE_TOOLS_DEFINED\n"
        "\n\n"
        "\n/*BACKPORT MOVE TOOLS END*/\n";


    return result;
}

static std::map<implementation, std::map<move_keywords, std::string> > move_replace_to = get_move_replace_to();

static std::string getMoveMacro(move_keywords id) {
    return move_replace_to[implementationKind][id];
}


#endif
