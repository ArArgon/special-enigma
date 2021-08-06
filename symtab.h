#ifndef _SYMTAB_H_

#define _SYMTAB_H_
//symbal table
#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <utility>
#include <algorithm>

struct symbalTableMember 
{
    enum symTabType
    {
        INT, ARRAY, FUNC
    };
    
    std::string ssa_name;
    std::string original_name;
    symTabType type; //"INT" "ARRAY" "FUNC"
    int label_name;// "global" ..."param - 0"
    bool initialized = 0;
    std::vector<int> value;//initialized =1,value.empty()=1,-->all value =0 ;
                           // func param  int var 1, arr 0
    std::vector<int> arrayIndex; //[0] -

    void init(std::string in_ssa_name, std::string in_original_name, symTabType in_type, int in_label_name)
    {
        ssa_name = in_ssa_name;
        original_name = in_original_name;
        type = in_type;
        label_name = in_label_name;

    }
};

class symbalTable
{
    std::vector<symbalTableMember> globalvar;
    std::vector<symbalTableMember> func;
    std::vector<symbalTableMember> localvar;

public:
    symbalTable() = default;

    void addGlobalVar(const symbalTableMember& symTabMember)
    {
        globalvar.push_back(symTabMember);
    }

    void addFunc(const symbalTableMember& symTabMember)
    {
        func.push_back(symTabMember);
    }

    void addLocalVar(const symbalTableMember& symTabMember)
    {
        localvar.push_back(symTabMember);
    }

    void clearLocalVar()
    {
        localvar.clear();
    }

    int findInGlobal(std::string name, symbalTableMember::symTabType in_type)
    {
        if(globalvar.empty()) //!
            return -1;
        int count = globalvar.size();
        for (int i = count-1; i >= 0; i--)
        {
            if((globalvar[i].original_name == name) && (globalvar[i].type==in_type))
                return i;
        }

        return -1;
    }

    int findInLocal(std::string name, int label, symbalTableMember::symTabType in_type)
    {
        if(localvar.empty())
            return -1;
        int count = localvar.size();
        for (int i = count-1; i >= 0; i--)
        {
            if((localvar[i].original_name == name) && (localvar[i].label_name==label) && (localvar[i].type==in_type))
                return i;
        }
        return -1;
    }

    int findFunc(std::string name)
    {
        if(func.empty()) //!
            return -1;
        int count = func.size();
        for (int i = count-1; i >= 0; i--)
        {
            if(func[i].original_name == name)
                return i;
        }

        return -1;
    }

    symbalTableMember getFunc(int i)
    {
        return func[i];
    }

    void addFunc_value(int i, std::vector<int> in_value)
    {
        func[i].value = in_value;
    }

    symbalTableMember getGlobalVar(int i)
    {
        return globalvar[i];
    }

    symbalTableMember getLocalVar(int i)
    {
        return localvar[i];
    }
    
    symbalTableMember find(std::string in_name, symbalTableMember::symTabType in_type)
    {
        int i = findInLocal(in_name, 0, in_type);
        if(i >= 0)
        {
            return getLocalVar(i);
        }
        else
        {
            int j = findInGlobal(in_name, in_type);
            if(j >= 0)
            {
                return getGlobalVar(j);
            }
            else
            {
                std::cout << "error at find " << in_name << std::endl;
                exit(-1);
            }
        }
    }

    std::string find_name(std::string name, symbalTableMember::symTabType in_type)
    {
        return find(name, in_type).ssa_name;
    }

};

struct whileLable
{
    std::string cond;
    std::string stmt;
    std::string after;
};
#endif
