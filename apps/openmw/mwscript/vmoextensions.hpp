#ifndef MWSCRIPT_VMOEXTENSIONS_HPP_
#define MWSCRIPT_VMOEXTENSIONS_HPP_

namespace Compiler
{
    class Extensions;
}

namespace Interpreter
{
    class Interpreter;
}

namespace MWScript
{
    namespace VMO
    {
        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif /* MWSCRIPT_VMOEXTENSIONS_HPP_ */
