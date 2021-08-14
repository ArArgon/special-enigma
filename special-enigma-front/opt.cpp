#include "IRTypes.h"
#include <iostream>

#include "opt.h"
using namespace IntermediateRepresentation;

int optimization(IRProgram *programIR)
{
//    create_cfg(programIR);
//    create_gen_kill(programIR);
//    create_in_out(programIR);
    create_use_def(programIR);
    constant_propagation(programIR);
    return 0;
}

int create_cfg(IRProgram *programIR)
{
    if(programIR->getGlobal().size()==1)
    {
        auto it=programIR->getGlobal().begin();
        it->next.push_back(1);
    }
    else
    {
        for(auto it=programIR->getGlobal().begin();it!=programIR->getGlobal().end();++it)
        {
            if(it==programIR->getGlobal().begin())
            {
                it->next.push_back(1);
            }
            else if(it==programIR->getGlobal().end()-1)
            {
                it->pre.push_back(programIR->getGlobal().size()-2);
            }
            else
            {
                it->pre.push_back(it-programIR->getGlobal().begin()-1);
                it->next.push_back(it-programIR->getGlobal().begin()+1);
            }
        }
    }


    for(auto it_f=programIR->getFunctions().begin();it_f!=programIR->getFunctions().end();++it_f)
    {
        for(auto it_stmt=it_f->getStatements().begin();it_stmt!=it_f->getStatements().end();++it_stmt)
        {
            if(it_stmt->getStmtType()==BR)
            {
                if(it_stmt->getOps().size()==1) //single jump
                {
                    for(auto it_find_label=it_f->getStatements().begin();
                    it_find_label!=it_f->getStatements().end();++it_find_label)
                    {
                        if(it_find_label->getStmtType()==LABEL && it_find_label->getOps()[0].getStrValue()==it_stmt->getOps()[0].getStrValue())
                        {
                            it_stmt->next.push_back(it_find_label-it_f->getStatements().begin());
                            it_find_label->pre.push_back(it_stmt-it_f->getStatements().begin());
                        }
                    }
                }
                else if(it_stmt->getOps().size()==3) //condition jump
                {
                    for(auto it_find_label=it_f->getStatements().begin();
                    it_find_label!=it_f->getStatements().end();++it_find_label)
                    {
                        if(it_find_label->getStmtType()==LABEL)
                        {
                            if(it_find_label->getOps()[0].getStrValue()==it_stmt->getOps()[1].getStrValue()
                            || it_find_label->getOps()[0].getStrValue()==it_stmt->getOps()[2].getStrValue())
                            {
                                it_stmt->next.push_back(it_find_label-it_f->getStatements().begin());
                                it_find_label->pre.push_back(it_stmt-it_f->getStatements().begin());
                            }
                        }
                    }
                }
            }
            else
            {
                if(it_stmt==it_f->getStatements().begin())
                {
                    it_stmt->next.push_back(1);
                }
                else if(it_stmt==it_f->getStatements().end()-1)
                {
                    it_stmt->pre.push_back(it_f->getStatements().size()-2);
                }
                else
                {
                    it_stmt->pre.push_back(it_stmt-it_f->getStatements().begin()-1);
                    it_stmt->next.push_back(it_stmt-it_f->getStatements().begin()+1);
                }
            }
            auto pre = std::unique(it_stmt->pre.begin(), it_stmt->pre.end());
            it_stmt->pre.erase(pre,it_stmt->pre.end());
            auto next = std::unique(it_stmt->next.begin(), it_stmt->next.end());
            it_stmt->next.erase(next,it_stmt->next.end());
        }
    }
    return 0;
}

int create_gen_kill(IRProgram *programIR)
{
    for(auto it=programIR->getGlobal().begin();it!=programIR->getGlobal().end();++it)
    {
        if(it->getStmtType() == GLB_VAR)
        {
            it->gen.push_back(it-programIR->getGlobal().begin());
        }
    }

    for(auto it_f=programIR->getFunctions().begin();it_f!=programIR->getFunctions().end();++it_f)
    {
        for(auto it_stmt=it_f->getStatements().begin();it_stmt!=it_f->getStatements().end();++it_stmt)
        {
            if(it_stmt->getStmtType() == ADD
            || it_stmt->getStmtType() == MUL
            || it_stmt->getStmtType() == DIV
            || it_stmt->getStmtType() == MOD
            || it_stmt->getStmtType() == SUB
            || it_stmt->getStmtType() == MOV)
            {
                it_stmt->gen.push_back(it_stmt-it_f->getStatements().begin());
            }
        }
    }
    return 0;
}

int create_in_out(IRProgram *programIR)
{
    //solve first element, only OUT, no need IN
    auto it_first = programIR->getGlobal().begin();
    std::sort(it_first->IN.begin(), it_first->IN.end());
    std::sort(it_first->kill.begin(), it_first->kill.end());
    std::set_difference(it_first->IN.begin(), it_first->IN.end(),
                        it_first->kill.begin(), it_first->kill.end(), std::inserter(it_first->OUT,it_first->OUT.begin()));

    std::sort(it_first->gen.begin(), it_first->gen.end());
    std::sort(it_first->OUT.begin(), it_first->OUT.end());
    set_union(it_first->gen.begin(), it_first->gen.end(),
              it_first->OUT.begin(), it_first->OUT.end(), std::inserter(it_first->OUT,it_first->OUT.begin()));


    for(auto it=programIR->getGlobal().begin()+1;it!=programIR->getGlobal().end();++it)
    {
        //solve IN
        set_union((it-1)->OUT.begin(), (it-1)->OUT.end(),
                  it->IN.begin(), it->IN.end(), std::inserter(it->IN, it->IN.begin()));

        //solve OUT
        std::sort(it->IN.begin(), it->IN.end());
        std::sort(it->kill.begin(), it->kill.end());
        std::set_difference(it->IN.begin(), it->IN.end(),
                            it->kill.begin(), it->kill.end(), std::inserter(it->OUT,it->OUT.begin()));
        std::sort(it->gen.begin(), it->gen.end());
        std::sort(it->OUT.begin(), it->OUT.end());
        set_union(it->gen.begin(), it->gen.end(),
                  it->OUT.begin(), it->OUT.end(), std::inserter(it->OUT, it->OUT.begin()));
    }

    for(auto it_f=programIR->getFunctions().begin();it_f!=programIR->getFunctions().end();++it_f)
    {
        for(auto it_stmt=it_f->getStatements().begin();it_stmt!=it_f->getStatements().end();++it_stmt)
        {
            //solve IN
            for(auto pre : it_stmt->pre)
            {
                std::sort((it_f->getStatements().begin()+pre)->OUT.begin(), (it_f->getStatements().begin()+pre)->OUT.end());
                std::sort(it_stmt->IN.begin(), it_stmt->IN.end());
                set_union((it_f->getStatements().begin()+pre)->OUT.begin(), (it_f->getStatements().begin()+pre)->OUT.end(),
                          it_stmt->IN.begin(), it_stmt->IN.end(), std::inserter(it_stmt->IN, it_stmt->IN.begin()));
            }
            //solve OUT
            std::sort(it_stmt->IN.begin(), it_stmt->IN.end());
            std::sort(it_stmt->kill.begin(), it_stmt->kill.end());
            std::set_difference(it_stmt->IN.begin(), it_stmt->IN.end(),
                                it_stmt->kill.begin(), it_stmt->kill.end(), std::inserter(it_stmt->OUT, it_stmt->OUT.begin()));
            std::sort(it_stmt->gen.begin(), it_stmt->gen.end());
            std::sort(it_stmt->OUT.begin(), it_stmt->OUT.end());
            set_union(it_stmt->gen.begin(), it_stmt->gen.end(),
                      it_stmt->OUT.begin(), it_stmt->OUT.end(), std::inserter(it_stmt->OUT, it_stmt->OUT.begin()));
        }
    }
    return 0;
}

int clear_use_def(IRProgram *programIR)
{
    for(auto it=programIR->getGlobal().begin();it!=programIR->getGlobal().end();++it)
    {
        for(auto it_op=it->getOps().begin();it_op!=it->getOps().end();++it_op)
        {
            it_op->use_scope.clear();
            it_op->use.clear();
            it_op->def_scope=-2;
            it_op->def=-2;
        }
    }
    for(auto it_f=programIR->getFunctions().begin();it_f!=programIR->getFunctions().end();++it_f)
    {
        for(auto it_stmt=it_f->getStatements().begin();it_stmt!=it_f->getStatements().end();++it_stmt)
        {
            for(auto it_op=it_stmt->getOps().begin();it_op!=it_stmt->getOps().end();++it_op)
            {
                it_op->use_scope.clear();
                it_op->use.clear();
                it_op->def_scope=-2;
                it_op->def=-2;
            }
        }
    }

    return 0;
}

int create_use_def(IRProgram *programIR)
{
    for(auto it=programIR->getGlobal().begin();it!=programIR->getGlobal().end();++it)
    {
        auto& target = it->getOps()[0];
        for(auto it_f=programIR->getFunctions().begin();it_f!=programIR->getFunctions().end();++it_f)
        {
            for(auto it_stmt=it_f->getStatements().begin();it_stmt!=it_f->getStatements().end();++it_stmt)
            {
                for(auto it_op=it_stmt->getOps().begin();it_op!=it_stmt->getOps().end();++it_op)
                {
                    if(it_op->getIrOpType()==Var && it_op->getVarName()==target.getVarName())
                    {
                        target.use_scope.push_back(it_f-programIR->getFunctions().begin());
                        target.use.push_back(std::make_pair(it_stmt-it_f->getStatements().begin(), it_op-it_stmt->getOps().begin()));
                        it_op->def_scope=-1;
                        it_op->def=it-programIR->getGlobal().begin();
                    }
                }
            }
        }
    }
    for(auto it_f=programIR->getFunctions().begin();it_f!=programIR->getFunctions().end();++it_f)
    {
        for(auto it_stmt=it_f->getStatements().begin();it_stmt!=it_f->getStatements().end();++it_stmt)
        {
            auto& target = it_stmt->getOps()[0];
            for(auto it_f_1=programIR->getFunctions().begin();it_f_1!=programIR->getFunctions().end();++it_f_1)
            {
                for(auto it_stmt_1=it_f_1->getStatements().begin();it_stmt_1!=it_f_1->getStatements().end();++it_stmt_1)
                {
                    for(auto it_op=it_stmt_1->getOps().begin();it_op!=it_stmt_1->getOps().end();++it_op)
                    {
                        if(it_op->getIrOpType()==Var && it_op->getVarName()==target.getVarName())
                        {
                            target.use_scope.push_back(it_f_1-programIR->getFunctions().begin());
                            target.use.push_back(std::make_pair(it_stmt_1-it_f_1->getStatements().begin(), it_op-it_stmt_1->getOps().begin()));
                            it_op->def_scope=it_f-programIR->getFunctions().begin();
                            it_op->def=it_stmt-it_f->getStatements().begin();
                        }
                    }
                }
            }
        }
    }

    return 0;
}

int constant_propagation(IRProgram *programIR)
{
    for(auto it=programIR->getGlobal().begin();it!=programIR->getGlobal().end();)
    {
        if(it->getStmtType()==GLB_VAR && it->getOps()[1].getIrOpType()==ImmVal)
        {
            //go through use table and change
            for(int i=0;i<it->getOps()[0].use.size();i++)
            {
                auto it_f=programIR->getFunctions().begin()+it->getOps()[0].use_scope[i];
                auto it_stmt=it_f->getStatements().begin()+it->getOps()[0].use[i].first;
                auto new_ops = it_stmt->getOps();
                auto it_use=new_ops.begin()+it->getOps()[0].use[i].second;
                it_use->setIrOpType(ImmVal);
                it_use->setValue(it->getOps()[1].getValue());
                it_use->def_scope=-2;
                it_use->def=-2;
                it_stmt->setOps(new_ops);
            }
            //delete original stmt
            programIR->getGlobal().erase(it);
            //update use def table
            clear_use_def(programIR);
            create_use_def(programIR);
        }
        else{
            ++it;
        }
    }

    for(auto it_f=programIR->getFunctions().begin();it_f!=programIR->getFunctions().end();++it_f)
    {
        for(auto it_stmt=it_f->getStatements().begin();it_stmt!=it_f->getStatements().end();)
        {
            if(it_stmt->getStmtType()==MOV && it_stmt->getOps()[1].getIrOpType()==ImmVal) // v<---c form
            {
                //go through use table and change
                for(int i=0;i<it_stmt->getOps()[0].use.size();i++)
                {
                    auto it_stmt_use=it_f->getStatements().begin()+it_stmt->getOps()[0].use[i].first;
                    auto& it_use = it_stmt_use->getOps()[it_stmt->getOps()[0].use[i].second];
                    it_use.setIrOpType(ImmVal);
                    it_use.setValue(it_stmt->getOps()[1].getValue());
                    it_use.def_scope=-2;
                    it_use.def=-2;
                }
                //delete original stmt
                it_f->getStatements().erase(it_stmt);
                //update use def table
                clear_use_def(programIR);
                create_use_def(programIR);
            }
            else{
                ++it_stmt;
            }
        }
    }
    return 0;
}

int debug_opt(IRProgram *programIR)
{
    //debug_cfg(programIR);
    //debug_gen_kill(programIR);
    //debug_in_out(programIR);
    debug_use_def(programIR);
    return 0;
}

int debug_cfg(IRProgram *programIR)
{
    std::cout<<"debug_cfg:"<<std::endl;
    for(auto it=programIR->getGlobal().begin();it!=programIR->getGlobal().end();++it)
    {
        std::cout<<"pre: ";
        for(auto pre : it->pre)
            std::cout<<pre<<" ";
        std::cout<<std::endl;

        std::cout<<"next: ";
        for(auto next : it->next)
            std::cout<<next<<" ";
        std::cout<<std::endl;
    }
    for(auto it_f=programIR->getFunctions().begin();it_f!=programIR->getFunctions().end();++it_f)
    {
        for(auto it_stmt=it_f->getStatements().begin();it_stmt!=it_f->getStatements().end();++it_stmt)
        {
            std::cout<<"pre: ";
            for(auto pre : it_stmt->pre)
                std::cout<<pre<<" ";
            std::cout<<std::endl;

            std::cout<<"next: ";
            for(auto next : it_stmt->next)
                std::cout<<next<<" ";
            std::cout<<std::endl;
        }
    }
    return 0;
}

int debug_gen_kill(IRProgram *programIR)
{
    std::cout<<"debug_gen_kill:"<<std::endl;
    for(auto it=programIR->getGlobal().begin();it!=programIR->getGlobal().end();++it)
    {
        std::cout<<"gen: ";
        for(auto gen : it->gen)
            std::cout<<gen<<" ";
        std::cout<<std::endl;

        std::cout<<"kill: ";
        for(auto kill : it->kill)
            std::cout<<kill<<" ";
        std::cout<<std::endl;
    }
    for(auto it_f=programIR->getFunctions().begin();it_f!=programIR->getFunctions().end();++it_f)
    {
        for(auto it_stmt=it_f->getStatements().begin();it_stmt!=it_f->getStatements().end();++it_stmt)
        {
            std::cout<<"gen: ";
            for(auto gen : it_stmt->gen)
                std::cout<<gen<<" ";
            std::cout<<std::endl;

            std::cout<<"kill: ";
            for(auto kill : it_stmt->kill)
                std::cout<<kill<<" ";
            std::cout<<std::endl;
        }
    }
    return 0;
}

int debug_in_out(IRProgram *programIR)
{
    std::cout<<"debug_IN_OUT:"<<std::endl;
    for(auto it=programIR->getGlobal().begin();it!=programIR->getGlobal().end();++it)
    {
        std::cout<<"IN: ";
        for(auto IN : it->IN)
            std::cout<<IN<<" ";
        std::cout<<std::endl;

        std::cout<<"OUT: ";
        for(auto OUT : it->OUT)
            std::cout<<OUT<<" ";
        std::cout<<std::endl;
    }
    for(auto it_f=programIR->getFunctions().begin();it_f!=programIR->getFunctions().end();++it_f)
    {
        for(auto it_stmt=it_f->getStatements().begin();it_stmt!=it_f->getStatements().end();++it_stmt)
        {
            std::cout<<"IN: ";
            for(auto IN : it_stmt->IN)
                std::cout<<IN<<" ";
            std::cout<<std::endl;

            std::cout<<"OUT: ";
            for(auto OUT : it_stmt->OUT)
                std::cout<<OUT<<" ";
            std::cout<<std::endl;
        }
    }
    return 0;
}

int debug_use_def(IRProgram *programIR)
{
    std::cout<<"debug_use_def:"<<std::endl;
    for(auto it=programIR->getGlobal().begin();it!=programIR->getGlobal().end();++it)
    {
        if(it->getOps()[0].use.size()==0)
            std::cout<<"null"<<" ";
        else{
            for(int i=0;i<it->getOps()[0].use.size();i++)
                std::cout<<it->getOps()[0].use_scope[i]<<","<<it->getOps()[0].use[i].first<<","<<it->getOps()[0].use[i].second<<" ";
        }
//        for(auto it1=it->getOps().begin()+1;it1!=it->getOps().end();++it1)
//            std::cout<<it1->def_scope<<","<<it1->def<<" ";
        std::cout<<std::endl;
    }
    std::cout<<"<------>"<<std::endl;
    for(auto it_f=programIR->getFunctions().begin();it_f!=programIR->getFunctions().end();++it_f)
    {
        for(auto it_stmt=it_f->getStatements().begin();it_stmt!=it_f->getStatements().end();++it_stmt)
        {
            if(it_stmt->getOps()[0].use.size()==0)
                std::cout<<"null"<<" ";
            else{
                for(int i=0;i<it_stmt->getOps()[0].use.size();i++)
                    std::cout<<it_stmt->getOps()[0].use_scope[i]<<","<<it_stmt->getOps()[0].use[i].first<<","<<it_stmt->getOps()[0].use[i].second<<" ";
            }
            for(auto it_right_ops=it_stmt->getOps().begin()+1;it_right_ops!=it_stmt->getOps().end();++it_right_ops)
                if(it_right_ops->getIrOpType()==Var)
                    std::cout<<it_right_ops->def_scope<<","<<it_right_ops->def<<" ";
            std::cout<<std::endl;
        }
    }
    return 0;
}