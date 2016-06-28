#ifndef PRINTINGPOLICY_SINGLETON_H
#define PRINTINGPOLICY_SINGLETON_H

#include <clang/AST/PrettyPrinter.h>

namespace backport { namespace helper {
    class PrintingPolicySingleton
    {
    public:
        static clang::PrintingPolicy& instance() {
            static PrintingPolicySingleton s;
            return s.Policy;
        }
    private:
        PrintingPolicySingleton() : Policy( { /*Instantiates a LangOptions object*/ } ) {
            Policy.LangOpts.CPlusPlus = true;
            Policy.Bool = true;
            Policy.SuppressUnwrittenScope = true;
            Policy.PolishForDeclaration = true;
        }

        PrintingPolicySingleton(PrintingPolicySingleton const&) = delete;
        PrintingPolicySingleton(PrintingPolicySingleton const&&) = delete;
        void operator=(PrintingPolicySingleton const&) = delete;

        clang::PrintingPolicy Policy;
    };

} /*namespace helper*/ } /*namespace backport*/

#endif // PRINTINGPOLICY_SINGLETON_H
