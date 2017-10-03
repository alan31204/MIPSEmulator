CSC 252 Project 3 MIPS Emulator README
Po-Chun Chiu & Yujie Liu
Email: pchiu4@u.rochester.edu, yliu134@u.rochester.edu

COMPILE STEPS:
Compile the program using makefile and and run the program using commands under Linux or Mac Terminal

Sample Compile Steps:
cd 252Project3
cd code
make
./eMIPS tests/asm_tier1/arith 100
./eMIPS tests/asm_tier2_new/BinarySearch 1000
./eMIPS tests/cpp/hello 100000


Modify everything in the for loop under PROC.c

How we approach the problem:
We implement all the test cases following the InstructionSubSet.pdf and the book MIPSInstruction Reference.
Since everything is implemented as a 32 bit length word, we need to decode the instructions from this 32 bit word.
We follow the ideas from the MIPSInstruction book on what each instruction does using bitwise operations, and decide to use switch statement for this project.
We switch for opCode, func, and also rt.
Thus, the code for the project is several nested switch statement in the for loop for every instructions. That is, the code will keep running until it finished the MaxInst or reach to the syscall instruction and terminates.
