//
// Created by 廖治平 on 5/18/21.
//

#ifndef SYSYBACKEND_IRTYPES_H
#define SYSYBACKEND_IRTYPES_H

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace IntermediateRepresentation {

    class Identifier {
        std::string identifierName;
        IdentifierType identifierType;

    public:
        enum IdentifierType {
            GLOBAL, LOCAL;
        };


    };

    class enum IRType {
        i4, i8, i16, i32, i64,
        f32, f64,
        str,
        t_void
    };

    class BaseStatement {

    public:

    };

    typedef std::vector<std::shared_ptr<BaseStatement>> IRSequence;

    class Function : public BaseStatement {
        Identifier functionIdentifier;
        IRSequence sequence;


    public:

    };


}
#endif //SYSYBACKEND_IRTYPES_H
