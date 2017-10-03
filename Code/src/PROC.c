
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "RegFile.h"
#include "Syscall.h"
#include "utils/heap.h"
#include "elf_reader/elf_reader.h"

//Stats

uint32_t DynInstCount = 0;

void write_initialization_vector(uint32_t sp, uint32_t gp, uint32_t start) {
        printf("\n ----- BOOT Sequence ----- \n");
        printf("Initializing sp=0x%08x; gp=0x%08x; start=0x%08x\n", sp, gp, start);
        RegFile[28] = gp;
        RegFile[29] = sp;
        RegFile[31] = start;
        printRegFile();

    }


int main(int argc, char * argv[]) {

    int MaxInst = 0;
    int status = 0;
    int jump = 0;   //initialize jump for jump condition
    uint32_t i;
    uint32_t PC,newPC;
    uint32_t CurrentInstruction;

    if (argc < 2) {
      printf("Input argument missing \n");
      return -1;
    }
    sscanf (argv[2], "%d", &MaxInst);

    //Open file pointers & initialize Heap & Regsiters
    initHeap();
    initFDT();
    initRegFile(0);

    //Load required code portions into Emulator Memory
    status =  LoadOSMemory(argv[1]);
    if(status <0) { return status; }

    //set Global & Stack Pointers for the Emulator
    // & provide startAddress of Program in Memory to Processor
    write_initialization_vector(exec.GSP, exec.GP, exec.GPC_START);

    // printf("\n ----- Execute Program ----- \n");
    // printf("Max Instruction to run = %d \n",MaxInst);
    PC = exec.GPC_START;

    for(i=0; i<MaxInst ; i++) {

      i--;
      label:
      i++;

        DynInstCount++;
        CurrentInstruction = readWord(PC,false);
       printRegFile();
    uint32_t opCode = CurrentInstruction >> 26; //get the opCode
    uint32_t func = CurrentInstruction & 63; // mask for Func
    uint32_t shamt = (CurrentInstruction & (31 << 6)) >> 6;
    uint32_t rs = (CurrentInstruction & (31 << 21)) >> 21;
    uint32_t rt = (CurrentInstruction & (31 << 16)) >> 16;
    uint32_t rd = (CurrentInstruction & (31 << 11)) >> 11;
    int32_t immediate = CurrentInstruction & 65535; // 2^16-1 to calculate for immediate, note that immediate is signed bit
    immediate = (immediate << 16) >> 16; //get the sign extension of the bit for proper immediate value
    RegFile[0] = 0; //Register 0 hardwired to 0

    // printf("%d\n\n\n\n\n", opCode);

    switch(opCode){
      case 0: {
      switch(func){
        case 32: // add
          RegFile[rd]= RegFile[rs]+RegFile[rt];
          break;
        case 33: // addu
          RegFile[rd]= RegFile[rs]+RegFile[rt]; // the same as add
          break;
        case 34: // sub
          RegFile[rd]= RegFile[rs]-RegFile[rt];
          break;
        case 35: // subu
          RegFile[rd]= (uint32_t)(RegFile[rs])-(uint32_t)(RegFile[rt]);  // same as sub
          break;
        case 26: // div
          RegFile[32] = RegFile[rs] % RegFile[rt]; //Store the remainder in high
          RegFile[33] = RegFile[rs] / RegFile[rt]; //Store the quotient in low
          break;
        case 27: // divu
          RegFile[32] = (uint32_t)(RegFile[rs]) % (uint32_t)(RegFile[rt]); //Store the remainder in high
          RegFile[33] = (uint32_t)(RegFile[rs]) / (uint32_t)(RegFile[rt]); //Store the quotient in low
          break;
        case 24: // mult
        { int64_t answer = (int64_t)(RegFile[rs]) * (int64_t)(RegFile[rt]);
          int64_t mask = 4294967295; // 2^32 - 1
          RegFile[32] = answer & (mask << 32); // store the upper 32 bits
          RegFile[33] = answer & mask; // store the lower 32 bits
          break;
        }
        case 25: // multu
        { uint64_t answer = (uint64_t)((int64_t)(RegFile[rs])) * (uint64_t)((int64_t)(RegFile[rt]));
          uint64_t mask = 4294967295; // 2^32 - 1
          RegFile[32] = answer & (mask << 32); // store the upper 32 bits
          RegFile[33] = answer & mask; // store the lower 32 bits
          break;
        }
        case 16: // mfhi
          RegFile[rd]=RegFile[32];
          break;
        case 18: // mflo
          RegFile[rd]=RegFile[33];
          break;
        case 17: // mthi
          RegFile[32]=RegFile[rs];
          break;
        case 19: // mtlo
          RegFile[33]=RegFile[rs];
          break;
        case 36: // and
          RegFile[rd]=RegFile[rs] & RegFile[rt];
          break;
        case 38: // xor
          RegFile[rd] = RegFile[rs] ^ RegFile[rt];
          break;
        case 39: // nor
          RegFile[rd] = ~(RegFile[rs] | RegFile[rt]);
          break;
        case 37: // or
          RegFile[rd] = RegFile[rs] | RegFile[rt];
          break;
          case 0: // sll or nop
            // if(func == 0){
            //   break;
            // }
            RegFile[rd]=RegFile[rt] << shamt;
            break;
          case 4: // sllv
            RegFile[rd] = RegFile[rt] << RegFile[rs];
            break;
          case 42: // slt
            RegFile[rd] = (RegFile[rs] < RegFile[rt]);
            // printf("----------------slt%d\n\n",RegFile[rd]);
            break;
          case 43: // sltu
            RegFile[rd] = ((uint32_t)RegFile[rs] < (uint32_t)RegFile[rt]);
            break;
          case 3: // sra
            RegFile[rd] = RegFile[rt] >> shamt;
            break;
          case 7: // srav
            RegFile[rd] = RegFile[rt] >> RegFile[rs];
            break;
          case 2: // srl
            RegFile[rd] = (int32_t)(((uint32_t)RegFile[rt]) >> shamt);
            break;
          case 6: // srlv
            RegFile[rd] = (int32_t)(((uint32_t)RegFile[rt]) >> RegFile[rs]);
            break;


          // Jump when opCode=0
          // include jalr & jr
          case 9: // jalr
            jump=1;
            // RegFile[rd] = PC + 8;
            RegFile[31] = PC + 8; // set it to ra instead of rd since no two registers will get executed
            newPC = RegFile[rs];
            PC+=4;
            goto label;
            break;
          //not sure
          case 8: // jr
            jump=1;
            // RegFile[rd] = PC + 8;
            newPC = RegFile[rs];
            PC+=4;
            goto label;
            break;
          case 12:
            SyscallExe(RegFile[2]);
            break;

      }

       break;  //break for opCode = 0
}
      case 8: // addi
        RegFile[rt]= RegFile[rs]+immediate;
        break;
      case 9: //addiu
      {
        // uint32_t a = RegFile[rs];
        uint32_t b = (uint32_t)RegFile[rs] + immediate;
        RegFile[rt] = b;
        // printf("%d + %d == %d\n\n\n", a, immediate, RegFile[rt]);

        break;
      }
      case 12: //andi
      { uint32_t mask = immediate;
        mask = (mask << 16) >> 16;
        RegFile[rt]= RegFile[rs] & mask;
        break;
      }
      case 14: //xori
      { uint32_t mask = immediate;
        mask = (mask << 16) >> 16;
        RegFile[rt]= RegFile[rs] ^ mask;
        break;
      }
      case 13: //ori
      { uint32_t mask = immediate;
        mask = (mask << 16) >> 16;
        RegFile[rt]= (RegFile[rs] | mask);
      // }
        break;
      }
      case 10: //slti
        RegFile[rt] = (RegFile[rs] < immediate);
        break;
      case 11: //sltiu
        RegFile[rt] = ((uint32_t)RegFile[rs] < (uint32_t)immediate);
        break;

      //Branch Condition

      case 4: // beq
      { jump = 1;
        int32_t offset = (immediate << 16) >> 14; // shifts left by 2 = *4, using sign extension
        if(RegFile[rs] == RegFile[rt]){
          newPC = (PC + 4)+ offset; //not sure
        }
        else{
          newPC = PC + 8; //not sure; execute the instruction in delay slot
        }
        PC+=4;
        goto label;
        break;
      }

      case 1: { //REGIMM
      switch(rt){
        case 1: // bgez
          { jump = 1;
            int32_t offset = ((int32_t)immediate << 16) >> 14; // shifts left by 2 = *4, using sign extension
            if(RegFile[rs] >= 0){
              newPC = (PC + 4)+ offset; //not sure
            }
            else{
              newPC = PC + 8; //not sure, probably +8
            }
            PC+=4;
            goto label;
            break;
          }

        case 17:  // bgezal
        { jump = 1;
          int32_t offset = (int32_t)(immediate << 16) >> 14; // shifts left by 2 = *4, using sign extension
          if(RegFile[rs] >= 0){
            newPC = (PC + 4)+ offset; //not sure
          }
          else{
            newPC = PC + 8; //not sure;
          }
          RegFile[31] = PC + 8;
          PC+=4;
          goto label;
          break;
        }


        case 0 :  // bltz
        { jump = 1;
          int32_t offset = ((int32_t)immediate << 16) >> 14; // shifts left by 2 = *4, using sign extension
          if(RegFile[rs] < 0){
            newPC = (PC + 4)+ offset; //not sure
          }
          else{
            newPC = PC + 8; //not sure; execute the instruction in delay slot
          }
          PC+=4;
          goto label;
          break;
        }

        case 16:  //bltzal
        { jump = 1;
          int32_t offset = ((int32_t)immediate << 16) >> 14; // shifts left by 2 = *4, using sign extension
          if(RegFile[rs] < 0){
            newPC = (PC + 4)+ offset; //not sure
          }
          else{
            newPC = PC + 8; //not sure; execute the instruction in delay slot
          }
          RegFile[31] = PC + 8; // set it to ra instead of rd since no two registers will get executed
          PC+=4;
          goto label;
          break;
        }

      }
      break;
}

      case 7: // bgtz
      { jump = 1;
        int32_t offset = (int32_t)(immediate << 16) >> 14; // shifts left by 2 = *4, using sign extension
        if(RegFile[rs] > 0){
          newPC = (PC + 4)+ offset; //not sure
        }
        else{
          newPC = PC + 8; //not sure; execute the instruction in delay slot
        }
        PC+=4;
        goto label;
        break;
      }

      case 6: // blez
      { jump = 1;
        int32_t offset = (int32_t)(immediate << 16) >> 14; // shifts left by 2 = *4, using sign extension
        if(RegFile[rs] <= 0){
          newPC = (PC + 4)+ offset; //not sure
        }
        else{
          newPC = PC + 8; //not sure; execute the instruction in delay slot
        }
        PC+=4;
        goto label;
        break;
      }


      case 5: // bne
      { jump = 1;
        // int32_t offset = (immediate << 16) >> 14; // shifts left by 2 = *4, using sign extension
        int32_t offset = immediate << 2;
        if(RegFile[rs] != RegFile[rt]){
          newPC = (PC + 4)+ offset; //not sure
        }
        else{
          newPC = PC + 8; //not sure; execute the instruction in delay slot
        }
        PC+=4;
        goto label;
        break;
      }



      //Jump Condition
      case 2: // j
      { jump = 1;
        uint32_t offset = ((CurrentInstruction << 6) >> 4) | ((PC + 4) & (4 << 28));  //268435455
        newPC = offset;
        PC += 4;
        // printf("---------------jump\n\n");
        goto label;
        break;
      }

      case 3: // jal
      { jump = 1;
        uint32_t offset = ((CurrentInstruction << 6) >> 4) | ((PC + 4) & (4 << 28));  //268435455
        newPC = offset;
        RegFile[31] = PC + 8; // set $ra to the next
        PC += 4;
        goto label;
        break;
      }

      // load and store condition

      case 32: // lb
      { uint32_t address = RegFile[rs] + immediate; // rs = base
        RegFile[rt] = (int32_t) readByte(address, false);
        break;
      }
      case 36: // lbu
      { uint32_t address = RegFile[rs] + immediate; // rs = base
        RegFile[rt] = readByte(address, false);
        break;
      }
      case 33: // lh
      { uint32_t address = RegFile[rs] + immediate; // rs = base
        int32_t a = (int32_t)readWord(address, false);
        a = a & (0x0000ffff << 16);
        RegFile[rt] = a >> 16;  // sign extension
        break;
      }

      case 37: // lhu
      { uint32_t address = RegFile[rs] + immediate; // rs = base
        uint32_t a = (uint32_t)readWord(address, false);
        a = a & (0x0000ffff << 16);
        RegFile[rt] = (int32_t)(a >> 16);  // no sign extension
        break;
      }

      case 15: // lui
        // printf("%d\n", rt);
        RegFile[rt] = ((immediate) << 16);
        break;

      case 35: // lw
      { uint32_t address = RegFile[rs] + immediate; // rs = base
        RegFile[rt] = (int32_t) readWord(address, false);
        break;
      }


      case 34: // lwl
      {
        uint32_t address = RegFile[rs] + immediate;
        uint32_t start = address - address%4;
        uint32_t left = ((int32_t) readWord(start, false)) << (address - start);
        RegFile[rt] = RegFile[rt] | left;
        break;
      }

      case 38: // lwr
      {
        uint32_t address = RegFile[rs] + immediate;
        uint32_t start = address - address%4;
        uint32_t right = ((int32_t) readWord(start, false)) >> (3 - address + start);
        RegFile[rt] = RegFile[rt] | right;
        break;
      }

      case 40: // sb
      { uint32_t address = RegFile[rs] + immediate; // rs = base
        writeByte(address, RegFile[rt] & 0x000000ff, false);  // stands for 255 in decimal
        break;
      }


      case 41: // sh  store byte twice
      { uint32_t address = RegFile[rs] + immediate; // rs = base
        int32_t a = RegFile[rt] & 0x000000ff;  // stands for 255 in decimal
        int32_t b = RegFile[rt] & 0x0000ff00; // stands for 65280 in decimal
        writeByte(address+8, a , false);
        writeByte(address, b , false);
        break;
      }

      case 43: // sw
      { uint32_t address = (uint32_t)RegFile[rs] + immediate; // rs = base
        writeWord(address, RegFile[rt], false);
        break;
      }

      case 42: // swl
      {
        uint32_t address = RegFile[rs] + immediate;
        int j = 0;
        for(int i = address; i < (address - address%4 + 4); i++){
          writeByte(i, (uint8_t) (RegFile[rt] >> 8*(3 - j)) & 0x000000ff, false);
          j++;
        }
        break;
      }

       case 46: //swr
       {
         uint32_t address = RegFile[rs] + immediate;
         int j = 0;
         for(int i = address - address%4; i < address + 1; i++){
           writeByte(i, (uint8_t) (RegFile[rt] >> 8*(address - address%4 -1 - j)) & 0x000000ff, false);
           j++;
         }
       break;
      }

    }
    /********************************/
    //Add your implementation here
    /********************************/

    RegFile[0] = 0; //Register 0 hardwired to 0
    if(jump){ //when jump is true
      PC = newPC;
      jump = 0;
    }
    else {
      PC += 4;
    }
    // printf("%x\n", CurrentInstruction);
    // printf("i: %d\n", i);
  }
    //Close file pointers & free allocated Memory
    closeFDT();
    CleanUp();
    return 1;
}
