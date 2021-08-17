#ifndef SYSYBACKEND_PEEPHOLEOPTIMIZATION_H
#define SYSYBACKEND_PEEPHOLEOPTIMIZATION_H

#include <utility>

#include "Instruction.h"

Instruction::InstructionStream DoScan(Instruction::InstructionStream RefStream) {

    Instruction::InstructionStream instruction = std::move(RefStream);

    std::map<std::string, size_t> instructionNameSet;
    instructionNameSet[typeid(Instruction::LabelInstruction).name()] = 0;   // Label Name
    instructionNameSet[typeid(Instruction::LoadInstruction).name()] = 1;    // Load Name
    instructionNameSet[typeid(Instruction::SaveInstruction).name()] = 2;    // Save Name
    instructionNameSet[typeid(Instruction::MoveInstruction).name()] = 3;    // Save Name'
    instructionNameSet[typeid(Instruction::BranchInstruction).name()] = 4;  // Branch Name

    for (int i = 0; i < 1; i++) {
        for (auto it = instruction.begin(); it != instruction.end(); it++) {
            std::string nowInstructionName = typeid(**it).name();

            if (!instructionNameSet.count(nowInstructionName))
                continue;
            else {
                switch (instructionNameSet[nowInstructionName]) {
                    case 2: {
                        std::string nextInstructionName = typeid(**(it + 1)).name();
                        if (instructionNameSet.count(nextInstructionName) &&
                            instructionNameSet[nextInstructionName] == 1) {

                            std::shared_ptr<Instruction::SaveInstruction> nowInstruction = std::dynamic_pointer_cast<Instruction::SaveInstruction>(
                                    *it);
                            Instruction::Operands::Register nowTarget = nowInstruction->getTarget();
                            Instruction::Operands::LoadSaveOperand nowSource = nowInstruction->getSource();

                            std::shared_ptr<Instruction::LoadInstruction> nextInstruction = std::dynamic_pointer_cast<Instruction::LoadInstruction>(
                                    *(it + 1));
                            Instruction::Operands::Register nextTarget = nextInstruction->getTarget();
                            Instruction::Operands::LoadSaveOperand nextSource = nextInstruction->getSource();

                            //std::cout << nowTarget << "" << nextTarget << std::endl;

                            if (nowTarget == nextTarget && nowSource == nextSource) {
                                instruction.erase(it + 1);
                                continue;
                            } else if (nowSource == nextSource) {
                                std::string followedInstructionName = typeid(**(it + 2)).name();
                                if (instructionNameSet.count(followedInstructionName) &&
                                    instructionNameSet[followedInstructionName] == 3) {
                                    std::shared_ptr<Instruction::MoveInstruction> followedInstruction = std::dynamic_pointer_cast<Instruction::MoveInstruction>(
                                            *(it + 2));
                                    Instruction::Operands::Register followedTarget = followedInstruction->getTarget();
                                    Instruction::Operands::Operand2 followedSource = followedInstruction->getSource();

                                    if (followedTarget == nowTarget && followedSource == nextTarget) {
                                        int flag = 0;
                                        for (int i = 3; i <= 50; i++) {
                                            followedInstructionName = typeid(**(it + i)).name();
                                            if (!instructionNameSet.count(followedInstructionName)) {
                                                flag = 1;
                                                break;
                                            }
                                            switch (instructionNameSet[followedInstructionName]) {
                                                case 1: {
                                                    std::shared_ptr<Instruction::LoadInstruction> findInstruction = std::dynamic_pointer_cast<Instruction::LoadInstruction>(
                                                            *(it + i));
                                                    Instruction::Operands::Register findTarget = findInstruction->getTarget();
                                                    Instruction::Operands::LoadSaveOperand findSource = findInstruction->getSource();

                                                    if (findTarget == nextTarget)
                                                        flag = 1;
                                                    break;
                                                }
                                                case 3: {
                                                    std::shared_ptr<Instruction::MoveInstruction> findInstruction = std::dynamic_pointer_cast<Instruction::MoveInstruction>(
                                                            *(it + i));
                                                    Instruction::Operands::Register findTarget = findInstruction->getTarget();
                                                    Instruction::Operands::Operand2 findSource = findInstruction->getSource();

                                                    if (findTarget == nextTarget)
                                                        flag = 1;
                                                    break;
                                                }
                                            }
                                            if (flag == 1) break;
                                        }
                                        if (flag == 1) {
                                            it = instruction.erase(it + 1) - 1;
                                            it = instruction.erase(it + 1) - 1;
                                            continue;
                                        }
                                    }

                                }
                            }
                        }
                    }
                    case 4: {
                        /*
                         * b    xxx
                         * xxx:
                         *
                         * xxx:
                         * */
                        // TODOPee
                    }
                }
            }
        }
    }
    return instruction;

}


#endif //SYSYBACKEND_PEEPHOLEOPTIMIZATION_H
