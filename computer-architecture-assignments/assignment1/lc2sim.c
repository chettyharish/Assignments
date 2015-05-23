#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMMEMORY  						65536 /* maximum number of data words in memory */
#define NUMREGS  						8 /* number of machine registers */

#define ADD  							0
#define NAND  							1
#define LW  							2
#define SW  							3
#define BEQ  							4
#define MULT 							5
#define HALT 							6
#define NOOP 							7
#define NOOPINSTRUCTION 				0x1c00000

/*Hazard Detection Logic*/
#define EMPTY							999
#define NO_HAZARD 						1000
#define RA1					 			1001
#define RB1					 			1002
#define RA2								1003
#define RB2								1004
#define RA3 							1005
#define RB3								1006
#define RA1RB1 							1007
#define RA1RB2				 			1008
#define RA1RB3				 			1009
#define RA2RB1							1010
#define RA2RB2							1011
#define RA2RB3 							1012
#define RA3RB1							1013
#define RA3RB2							1014
#define RA3RB3							1015
#define LWINSTRUCTION	 				1016

/*Branch Prediction Logic*/
#define BRANCH_TRUE 					0
#define BRANCH_FALSE 					1
#define STRONGLY_NOT_TAKEN				0
#define WEAKLY_NOT_TAKEN				1
#define WEAKLY_TAKEN					2
#define STRONGLY_TAKEN					3

/*Prediction logic chooser*/
#define LSB_HISTORY_MAPPING 0
#define COUNTER_ONE_TO_ONE 1
#define LOCAL_HISTORY 2
#define PREDICTOR LSB_HISTORY_MAPPING	/*Set the prediction algorithm here!!*/
#define MOREOUTPUT 1	/*Set to 0 for additional printouts*/

typedef struct IFIDStruct {
	int instr;
	int pcPlus1;
} IFIDType;

typedef struct IDEXStruct {
	int instr;
	int pcPlus1;
	int readRegA;
	int readRegB;
	int offset;
	int currHazard;
} IDEXType;

typedef struct EXMEMStruct {
	int instr;
	int branchTarget;
	int aluResult;
	int readRegB;
	int currHazard;
} EXMEMType;

typedef struct MEMWBStruct {
	int instr;
	int writeData;
	int currHazard;
} MEMWBType;

typedef struct WBENDStruct {
	int instr;
	int writeData;
	int currHazard;
} WBENDType;

typedef struct stateStruct {
	int pc;
	int instrMem[NUMMEMORY];
	int dataMem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
	int cycles; /* number of cycles run so far */
	int fetched; /*# of instructions fetched*/
	int retired; /*# of instruction completed*/
	int branches;/*# of branches executed (i.e., resolved)*/
	int mispred; /*# of branches incorrectly predicted*/

} stateType;

typedef struct hazardStruct {
	int first;
	int second;
	int third;
	int load;
} hazardType;

/* Global structs*/
stateType state;
stateType newState;
hazardType newhazardState;
hazardType oldhazardState;

/*Functions*/
int field0(int instruction) {
	return ((instruction >> 19) & 0x7);
}

int field1(int instruction) {
	return ((instruction >> 16) & 0x7);
}

int field2(int instruction) {

	if (((instruction & 0x0000FFFF) >> 15) == 1) {
		/*Negative offset*/
		instruction = -((~(instruction & 0xFFFF) & 0xFFFF) + 1);
	} else {
		instruction = (instruction & 0x0000FFFF);
	}

	return instruction;
}

int opcode(int instruction) {
	return (instruction >> 22);
}

/*One to One Saturation Counter Prediction Algorithm Here*/
#if PREDICTOR == COUNTER_ONE_TO_ONE
typedef struct branchStruct {
	/* 0: instruction 1: target 2: state */
	int BTB[4][3];
	int curPos;
}branchType;

branchType branchBuffer;

int modifyBTB(int pc, int target, int branchTaken) {
	int i = 0;
	int oldstate = EMPTY;

	if (branchTaken == BRANCH_TRUE) {
		for (i = 0; i < 4; i++) {
			if (pc == branchBuffer.BTB[i][0]) {
				oldstate = branchBuffer.BTB[i][2];
				if (branchBuffer.BTB[i][2] != STRONGLY_TAKEN) {
					branchBuffer.BTB[i][2] += 1;
				}
				/*If return is 0, the branch was correctly predicted or else not*/
				if (oldstate == STRONGLY_TAKEN || oldstate == WEAKLY_TAKEN)
				return 0;
				else
				return 1;
			}
		}
		/*address not present*/
		branchBuffer.BTB[branchBuffer.curPos][0] = pc;
		branchBuffer.BTB[branchBuffer.curPos][1] = target;
		branchBuffer.BTB[branchBuffer.curPos][2] = WEAKLY_TAKEN;
		branchBuffer.curPos = (branchBuffer.curPos + 1) % 4;
		return 1;
	} else {
		/*If branch predicted by BTB, and not taken , then penalize*/
		for (i = 0; i < 4; i++) {
			if (pc == branchBuffer.BTB[i][0]) {
				oldstate = branchBuffer.BTB[i][2];
				if (branchBuffer.BTB[i][2] != STRONGLY_NOT_TAKEN) {
					branchBuffer.BTB[i][2] -= 1;
				}
				/*If return is 0 , then the branch was incorrectly predicted*/
				if (oldstate == STRONGLY_TAKEN || oldstate == WEAKLY_TAKEN)
				return 1;
				else
				return 0;
			}
		}
		return 0;
	}
}

int* predictBTB(int pc, int instr) {
	int i = 0;
	int *results = malloc(sizeof(int) * 2);
	if (opcode(instr) != BEQ) {
		results[0] = BRANCH_FALSE;
		return results;
	}
	for (i = 0; i < 4; i++) {
		if (pc == branchBuffer.BTB[i][0]) {
			if (branchBuffer.BTB[i][2] == STRONGLY_TAKEN
					|| branchBuffer.BTB[i][2] == WEAKLY_TAKEN) {
				results[0] = BRANCH_TRUE;
				results[1] = branchBuffer.BTB[i][1];
				return results;
			} else {
				results[0] = BRANCH_FALSE;
				return results;
			}
		}
	}
	results[0] = BRANCH_FALSE;
	return results;
}

/*Local History Prediciton Algorithm Here*/
#elif PREDICTOR == LOCAL_HISTORY

typedef struct branchStruct {
	/* 0: instruction	 1: target	 state is in branchHistoryTable stuct	*/
	int BTB[4][2];
	int curPos;
}branchType;

typedef struct branchHistoryStruct {
	int branchHistory[4];
}branchHistoryStruct;

typedef struct patternHistoryStruct {
	int patternHistory[4];
}patternHistoryStruct;

branchType branchBuffer;
branchHistoryStruct branchHistoryTable;
patternHistoryStruct patternHistoryTable;

int lsbBits(int pc) {
	return (pc & 0x03);
}

int modifyBTB(int pc, int target, int branchTaken) {

	int i = 0;
	int oldstate = EMPTY;
	if (branchTaken == BRANCH_TRUE) {

		/*If return is 0, the branch was correctly predicted at IFID
		 * and the target was already fetched
		 *If return is 1, the branch was not predicted at IFID
		 * and the instructions have to be flushed*/

		for (i = 0; i < 4; i++) {
			if (pc == branchBuffer.BTB[i][0]) {
				/*Old state has current priority before updating*/
				oldstate =
				patternHistoryTable.patternHistory[branchHistoryTable.branchHistory[lsbBits(
						branchBuffer.BTB[i][0])]];

				if (oldstate != STRONGLY_TAKEN) {
					/*Increment only if not already STRONGLY_TAKEN*/
					patternHistoryTable.patternHistory[branchHistoryTable.branchHistory[lsbBits(
							branchBuffer.BTB[i][0])]]++;
				}

				/*LEFT SHIFTING AND ADDING NEW INFORMATION*/
				branchHistoryTable.branchHistory[lsbBits(branchBuffer.BTB[i][0])] =
				(((branchHistoryTable.branchHistory[lsbBits(
												branchBuffer.BTB[i][0])]) << 1) + 1) & 0x03;

				if (oldstate == STRONGLY_TAKEN || oldstate == WEAKLY_TAKEN)
				return 0;
				else
				return 1;
			}
		}
		/*If branch not already present, then insert it into BTB
		 * and reset the corresponding branch history to 1 (as the branch was taken for first time)
		 * and the patternHistory to WEAKLY_TAKEN*/
		branchBuffer.BTB[branchBuffer.curPos][0] = pc;
		branchBuffer.BTB[branchBuffer.curPos][1] = target;
		branchHistoryTable.branchHistory[lsbBits(
				branchBuffer.BTB[branchBuffer.curPos][0])] = 1;
		patternHistoryTable.patternHistory[branchHistoryTable.branchHistory[lsbBits(
				branchBuffer.BTB[branchBuffer.curPos][0])]] = WEAKLY_TAKEN;
		branchBuffer.curPos = (branchBuffer.curPos + 1) % 4;
		return 1;
	} else {
		/*If return is 0, the branch was correctly predicted as NOT taken at IFID
		 * and the PC+1 instruction was fetched
		 *If return is 1, the branch was predicted as TRUE at IFID
		 * and the instructions have to be flushed*/

		/*If branch predicted by BTB, and not taken , then penalize*/
		for (i = 0; i < 4; i++) {
			if (pc == branchBuffer.BTB[i][0]) {
				oldstate =
				patternHistoryTable.patternHistory[branchHistoryTable.branchHistory[lsbBits(
						branchBuffer.BTB[i][0])]];
				if (oldstate != STRONGLY_NOT_TAKEN) {
					patternHistoryTable.patternHistory[branchHistoryTable.branchHistory[lsbBits(
							branchBuffer.BTB[i][0])]]--;
				}

				/*LEFT SHIFTING AND ADDING NEW INFORMATION*/
				branchHistoryTable.branchHistory[lsbBits(branchBuffer.BTB[i][0])] =
				(((branchHistoryTable.branchHistory[lsbBits(
												branchBuffer.BTB[i][0])]) << 1) + 0) & 0x03;

				/*If return is 1 , then the branch was incorrectly predicted*/
				if (oldstate == STRONGLY_TAKEN || oldstate == WEAKLY_TAKEN)
				return 1;
				else
				return 0;
			}
		}

		/*If branch not already present, then mis-prediction is
		 * not possible!*/
		return 0;
	}
}

int* predictBTB(int pc, int instr) {
	/*results[0] states that branch is predicted/not predicted
	 *results[1] holds target address for positive branch prediction else EMPTY
	 * */
	int i = 0;
	int *results = malloc(sizeof(int) * 2);
	results[0] = BRANCH_FALSE;
	results[1] = EMPTY;

	if (opcode(instr) != BEQ) {
		return results;
	}

	for (i = 0; i < 4; i++) {
		if (pc == branchBuffer.BTB[i][0]) {
			if (patternHistoryTable.patternHistory[branchHistoryTable.branchHistory[lsbBits(
							branchBuffer.BTB[i][0])]] == STRONGLY_TAKEN
					|| patternHistoryTable.patternHistory[branchHistoryTable.branchHistory[lsbBits(
							branchBuffer.BTB[i][0])]] == WEAKLY_TAKEN) {
				/*The Branch History Table predicting branch taken
				 * We have no modify the PC+1 to PC+1+OFFSET*/
				results[0] = BRANCH_TRUE;
				results[1] = branchBuffer.BTB[i][1];

			} else {
				/*Predicting as Branch will not be taken*/
			}
		}
	}
	return results;
}
/*LSB to Branch History Table Prediction ALgorithm Here*/
#else
typedef struct branchHistoryStruct {
	int branchHistory[4];
}branchHistoryStruct;

typedef struct branchStruct {
	/* 0: instruction	 1: target	 state is in branchHistoryTable stuct	*/
	int BTB[4][2];
	int curPos;
}branchType;

branchType branchBuffer;
branchHistoryStruct branchHistoryTable;

int lsbBits(int pc) {
	return (pc & 0x03);
}

int modifyBTB(int pc, int target, int branchTaken) {

	int i = 0;
	int oldstate = EMPTY;
	if (branchTaken == BRANCH_TRUE) {

		/*If return is 0, the branch was correctly predicted at IFID
		 * and the target was already fetched
		 *If return is 1, the branch was not predicted at IFID
		 * and the instructions have to be flushed*/

		for (i = 0; i < 4; i++) {
			if (pc == branchBuffer.BTB[i][0]) {
				/*Old state has current priority before updating*/
				oldstate = branchHistoryTable.branchHistory[lsbBits(
						branchBuffer.BTB[i][0])];

				if (oldstate != STRONGLY_TAKEN) {
					/*Increment only if not already STRONGLY_TAKEN*/
					branchHistoryTable.branchHistory[lsbBits(
							branchBuffer.BTB[i][0])]++;
				}

				if (oldstate == STRONGLY_TAKEN || oldstate == WEAKLY_TAKEN)
				return 0;
				else
				return 1;
			}
		}
		/*If branch not already present, then insert it into BTB
		 * and reset the corresponding state to WEAKLY_TAKEN*/
		branchBuffer.BTB[branchBuffer.curPos][0] = pc;
		branchBuffer.BTB[branchBuffer.curPos][1] = target;
		branchHistoryTable.branchHistory[lsbBits(
				branchBuffer.BTB[branchBuffer.curPos][0])] = WEAKLY_TAKEN;
		branchBuffer.curPos = (branchBuffer.curPos + 1) % 4;
		return 1;
	} else {
		/*If return is 0, the branch was correctly predicted as NOT taken at IFID
		 * and the PC+1 instruction was fetched
		 *If return is 1, the branch was predicted as TRUE at IFID
		 * and the instructions have to be flushed*/

		/*If branch predicted by BTB, and not taken , then penalize*/
		for (i = 0; i < 4; i++) {
			if (pc == branchBuffer.BTB[i][0]) {
				oldstate = branchHistoryTable.branchHistory[lsbBits(
						branchBuffer.BTB[i][0])];
				if (oldstate != STRONGLY_NOT_TAKEN) {
					branchHistoryTable.branchHistory[lsbBits(
							branchBuffer.BTB[i][0])]--;
				}

				/*If return is 1 , then the branch was incorrectly predicted*/
				if (oldstate == STRONGLY_TAKEN || oldstate == WEAKLY_TAKEN)
				return 1;
				else
				return 0;
			}
		}

		/*If branch not already present, then mis-prediction is
		 * not possible!*/
		return 0;
	}
}

int* predictBTB(int pc, int instr) {
	/*results[0] states that branch is predicted/not predicted
	 *results[1] holds target address for positive branch prediction else EMPTY
	 * */
	int i = 0;
	int *results = malloc(sizeof(int) * 2);
	results[0] = BRANCH_FALSE;
	results[1] = EMPTY;

	if (opcode(instr) != BEQ) {
		return results;
	}

	for (i = 0; i < 4; i++) {
		if (pc == branchBuffer.BTB[i][0]) {
			if (branchHistoryTable.branchHistory[lsbBits(branchBuffer.BTB[i][0])]
					== STRONGLY_TAKEN
					|| branchHistoryTable.branchHistory[lsbBits(
							branchBuffer.BTB[i][0])] == WEAKLY_TAKEN) {
				/*The Branch History Table predicting branch taken
				 * We have to modify the PC+1 to PC+1+OFFSET*/
				results[0] = BRANCH_TRUE;
				results[1] = branchBuffer.BTB[i][1];

			} else {
				/*Predicting as Branch will not be taken*/
			}
		}
	}
	return results;
}
#endif

int hazardLocation(int regA, int regB) {
	int loc1 = -1, loc2 = -1;
	if ((regA ^ oldhazardState.first) == 0)
		loc1 = (oldhazardState.load == LWINSTRUCTION) ? LWINSTRUCTION : 1;
	else if ((regA ^ oldhazardState.second) == 0)
		loc1 = 2;
	else if ((regA ^ oldhazardState.third) == 0)
		loc1 = 3;

	if ((regB ^ oldhazardState.first) == 0)
		loc2 = (oldhazardState.load == LWINSTRUCTION) ? LWINSTRUCTION : 1;
	else if ((regB ^ oldhazardState.second) == 0)
		loc2 = 2;
	else if ((regB ^ oldhazardState.third) == 0)
		loc2 = 3;

	if (loc1 == LWINSTRUCTION || loc2 == LWINSTRUCTION)
		return LWINSTRUCTION;
	else if (loc1 == -1 && loc2 == -1)
		return NO_HAZARD;
	else if (loc1 == -1 && loc2 == 1)
		return RB1;
	else if (loc1 == -1 && loc2 == 2)
		return RB2;
	else if (loc1 == -1 && loc2 == 3)
		return RB3;
	else if (loc1 == 1 && loc2 == -1)
		return RA1;
	else if (loc1 == 1 && loc2 == 1)
		return RA1RB1;
	else if (loc1 == 1 && loc2 == 2)
		return RA1RB2;
	else if (loc1 == 1 && loc2 == 3)
		return RA1RB3;
	else if (loc1 == 2 && loc2 == -1)
		return RA2;
	else if (loc1 == 2 && loc2 == 1)
		return RA2RB1;
	else if (loc1 == 2 && loc2 == 2)
		return RA2RB2;
	else if (loc1 == 2 && loc2 == 3)
		return RA2RB3;
	else if (loc1 == 3 && loc2 == -1)
		return RA3;
	else if (loc1 == 3 && loc2 == 1)
		return RA3RB1;
	else if (loc1 == 3 && loc2 == 2)
		return RA3RB2;
	else if (loc1 == 3 && loc2 == 3)
		return RA3RB3;

	/* Must never reach here!*/
	return 999;
}

void dataForward(int regA, int regB, int hazard, int *forwardValues) {

	switch (hazard) {

	case NO_HAZARD:
		forwardValues[0] = regA;
		forwardValues[1] = regB;
		break;

	case RA1:
		forwardValues[0] = state.EXMEM.aluResult;
		forwardValues[1] = regB;
		break;

	case RB1:
		forwardValues[0] = regA;
		forwardValues[1] = state.EXMEM.aluResult;
		break;

	case RA2:
		forwardValues[0] = state.MEMWB.writeData;
		forwardValues[1] = regB;
		break;

	case RB2:
		forwardValues[0] = regA;
		forwardValues[1] = state.MEMWB.writeData;
		break;

	case RA3:
		forwardValues[0] = state.WBEND.writeData;
		forwardValues[1] = regB;
		break;

	case RB3:
		forwardValues[0] = regA;
		forwardValues[1] = state.WBEND.writeData;
		break;

	case RA1RB1:
		forwardValues[0] = state.EXMEM.aluResult;
		forwardValues[1] = state.EXMEM.aluResult;
		break;

	case RA1RB2:
		forwardValues[0] = state.EXMEM.aluResult;
		forwardValues[1] = state.MEMWB.writeData;
		break;

	case RA1RB3:
		forwardValues[0] = state.EXMEM.aluResult;
		forwardValues[1] = state.WBEND.writeData;
		break;

	case RA2RB1:
		forwardValues[0] = state.MEMWB.writeData;
		forwardValues[1] = state.EXMEM.aluResult;
		break;

	case RA2RB2:
		forwardValues[0] = state.MEMWB.writeData;
		forwardValues[1] = state.MEMWB.writeData;
		break;

	case RA2RB3:
		forwardValues[0] = state.MEMWB.writeData;
		forwardValues[1] = state.WBEND.writeData;
		break;

	case RA3RB1:
		forwardValues[0] = state.WBEND.writeData;
		forwardValues[1] = state.EXMEM.aluResult;
		break;

	case RA3RB2:
		forwardValues[0] = state.WBEND.writeData;
		forwardValues[1] = state.MEMWB.writeData;
		break;

	case RA3RB3:
		forwardValues[0] = state.WBEND.writeData;
		forwardValues[1] = state.WBEND.writeData;
		break;
	}

	return;
}

void printInstruction(int instr) {
	char opcodeString[10];
	if (opcode(instr) == ADD) {
		strcpy(opcodeString, "add");
	} else if (opcode(instr) == NAND) {
		strcpy(opcodeString, "nand");
	} else if (opcode(instr) == LW) {
		strcpy(opcodeString, "lw");
	} else if (opcode(instr) == MULT) {
		strcpy(opcodeString, "mult");
	} else if (opcode(instr) == SW) {
		strcpy(opcodeString, "sw");
	} else if (opcode(instr) == BEQ) {
		strcpy(opcodeString, "beq");
	} else if (opcode(instr) == HALT) {
		strcpy(opcodeString, "halt");
	} else if (opcode(instr) == NOOP) {
		strcpy(opcodeString, "noop");
	} else {
		strcpy(opcodeString, "data");
	}

	printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
			field2(instr));
}

void printState(stateType *statePtr) {
	int i;
	printf("\n@@@\nstate before cycle %d starts\n", statePtr->cycles);
	printf("\tpc %d\n", statePtr->pc);

	printf("\tdata memory:\n");
	for (i = 0; i < statePtr->numMemory; i++) {
		printf("\t\tdataMem[ %d ] %d\n", i, statePtr->dataMem[i]);
	}
	printf("\tregisters:\n");
	for (i = 0; i < NUMREGS; i++) {
		printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	}
	printf("\tIFID:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IFID.instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IFID.pcPlus1);
	printf("\tIDEX:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IDEX.instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IDEX.pcPlus1);
	printf("\t\treadRegA %d\n", statePtr->IDEX.readRegA);
	printf("\t\treadRegB %d\n", statePtr->IDEX.readRegB);
	printf("\t\toffset %d\n", statePtr->IDEX.offset);
#if MOREOUTPUT == 0
	printf("\t\thazard %d\n", statePtr->IDEX.currHazard);
#endif

	printf("\tEXMEM:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->EXMEM.instr);
	printf("\t\tbranchTarget %d\n", statePtr->EXMEM.branchTarget);
	printf("\t\taluResult %d\n", statePtr->EXMEM.aluResult);
	printf("\t\treadRegB %d\n", statePtr->EXMEM.readRegB);
#if MOREOUTPUT == 0
	printf("\t\thazard %d\n", statePtr->EXMEM.currHazard);
#endif

	printf("\tMEMWB:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->MEMWB.instr);
	printf("\t\twriteData %d\n", statePtr->MEMWB.writeData);
#if MOREOUTPUT == 0
	printf("\t\thazard %d\n", statePtr->MEMWB.currHazard);
#endif

	printf("\tWBEND:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->WBEND.instr);
	printf("\t\twriteData %d\n", statePtr->WBEND.writeData);
#if MOREOUTPUT == 0
	printf("\t\thazard %d\n", statePtr->WBEND.currHazard);
#endif

#if MOREOUTPUT == 0
#if PREDICTOR == LSB_HISTORY_MAPPING
	printf("\t BTB:\n");
	printf("\t\t%-10d\t%-10d\t\n", branchBuffer.BTB[0][0],
			branchBuffer.BTB[0][1]);
	printf("\t\t%-10d\t%-10d\t\n", branchBuffer.BTB[1][0],
			branchBuffer.BTB[1][1]);
	printf("\t\t%-10d\t%-10d\t\n", branchBuffer.BTB[2][0],
			branchBuffer.BTB[2][1]);
	printf("\t\t%-10d\t%-10d\t\n", branchBuffer.BTB[3][0],
			branchBuffer.BTB[3][1]);

	printf("\t BHT:\n");
	printf("\t\t%-10d\n", branchHistoryTable.branchHistory[0]);
	printf("\t\t%-10d\n", branchHistoryTable.branchHistory[1]);
	printf("\t\t%-10d\n", branchHistoryTable.branchHistory[2]);
	printf("\t\t%-10d\n", branchHistoryTable.branchHistory[3]);

	printf("\t HAZARD:\n");
	printf("\t\t%-10d\t%-10d\t%-10d\t\n", oldhazardState.first,
			oldhazardState.second, oldhazardState.third);
	printf("\t\t%-10d\t\n", oldhazardState.load);

	printf("\t COUNTERS:\n");
	printf("\t\t%-10s: %d\n", "CYCLES", state.cycles);
	printf("\t\t%-10s: %d\n", "FETCHED", state.fetched);
	printf("\t\t%-10s: %d\n", "RETIRED", state.retired);
	printf("\t\t%-10s: %d\n", "BRANCHES", state.branches);
	printf("\t\t%-10s: %d\n", "MISPRED", state.mispred);

#endif/*LSB_HISTORY_MAPPING*/

#if PREDICTOR == COUNTER_ONE_TO_ONE
	printf("\t BTB:\n");
	printf("\t\t%-10d\t%-10d\t\n", branchBuffer.BTB[0][0],
			branchBuffer.BTB[0][1]);
	printf("\t\t%-10d\t%-10d\t\n", branchBuffer.BTB[1][0],
			branchBuffer.BTB[1][1]);
	printf("\t\t%-10d\t%-10d\t\n", branchBuffer.BTB[2][0],
			branchBuffer.BTB[2][1]);
	printf("\t\t%-10d\t%-10d\t\n", branchBuffer.BTB[3][0],
			branchBuffer.BTB[3][1]);

	printf("\t BHT:\n");
	printf("\t\t%-10d\n", branchHistoryTable.branchHistory[0]);
	printf("\t\t%-10d\n", branchHistoryTable.branchHistory[1]);
	printf("\t\t%-10d\n", branchHistoryTable.branchHistory[2]);
	printf("\t\t%-10d\n", branchHistoryTable.branchHistory[3]);

	printf("\t HAZARD:\n");
	printf("\t\t%-10d\t%-10d\t%-10d\t\n", oldhazardState.first,
			oldhazardState.second, oldhazardState.third);
	printf("\t\t%-10d\t\n", oldhazardState.load);

	printf("\t COUNTERS:\n");
	printf("\t\t%-10s: %d\n", "CYCLES", state.cycles);
	printf("\t\t%-10s: %d\n", "FETCHED", state.fetched);
	printf("\t\t%-10s: %d\n", "RETIRED", state.retired);
	printf("\t\t%-10s: %d\n", "BRANCHES", state.branches);
	printf("\t\t%-10s: %d\n", "MISPRED", state.mispred);
#endif/*COUNTER_ONE_TO_ONE*/

#if PREDICTOR == LOCAL_HISTORY
	printf("\t BTB:\n");
	printf("\t\t%-10d\t%-10d\t\n", branchBuffer.BTB[0][0],
			branchBuffer.BTB[0][1]);
	printf("\t\t%-10d\t%-10d\t\n", branchBuffer.BTB[1][0],
			branchBuffer.BTB[1][1]);
	printf("\t\t%-10d\t%-10d\t\n", branchBuffer.BTB[2][0],
			branchBuffer.BTB[2][1]);
	printf("\t\t%-10d\t%-10d\t\n", branchBuffer.BTB[3][0],
			branchBuffer.BTB[3][1]);

	printf("\t BHT:\n");
	printf("\t\t%-10d\n", branchHistoryTable.branchHistory[0]);
	printf("\t\t%-10d\n", branchHistoryTable.branchHistory[1]);
	printf("\t\t%-10d\n", branchHistoryTable.branchHistory[2]);
	printf("\t\t%-10d\n", branchHistoryTable.branchHistory[3]);

	printf("\t PHT:\n");
	printf("\t\t%-10d\n", patternHistoryTable.patternHistory[0]);
	printf("\t\t%-10d\n", patternHistoryTable.patternHistory[1]);
	printf("\t\t%-10d\n", patternHistoryTable.patternHistory[2]);
	printf("\t\t%-10d\n", patternHistoryTable.patternHistory[3]);

	printf("\t HAZARD:\n");
	printf("\t\t%-10d\t%-10d\t%-10d\t\n", oldhazardState.first,
			oldhazardState.second, oldhazardState.third);
	printf("\t\t%-10d\t\n", oldhazardState.load);

	printf("\t COUNTERS:\n");
	printf("\t\t%-10s: %d\n", "CYCLES", state.cycles);
	printf("\t\t%-10s: %d\n", "FETCHED", state.fetched);
	printf("\t\t%-10s: %d\n", "RETIRED", state.retired);
	printf("\t\t%-10s: %d\n", "BRANCHES", state.branches);
	printf("\t\t%-10s: %d\n", "MISPRED", state.mispred);
#endif/*LOCAL_HISTORY*/

#endif /*MORE OUTPUT*/

}

void run() {
	int regA, regB, val1, val2, result, predictedPath, hazard;
	int *forwardValues = malloc(sizeof(int) * 2);
	while (1) {
		printState(&state);
		/* check for halt */
		if (opcode(state.MEMWB.instr) == HALT) {
			return;
		}

		newState = state;
		newhazardState = oldhazardState;
		newState.cycles++;

		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		/* --------------------- IF stage --------------------- */
		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */

		newState.IFID.instr = state.instrMem[state.pc];

		int *results = predictBTB(state.pc, state.instrMem[state.pc]);
		newState.IFID.pcPlus1 = state.pc + 1;

		if (!(opcode(state.instrMem[state.pc]) > 7
				|| opcode(state.instrMem[state.pc]) < 0
				|| (opcode(state.instrMem[state.pc]) == ADD
						&& field0(state.instrMem[state.pc]) == 0
						&& field1(state.instrMem[state.pc]) == 0)))
			newState.fetched++;
		else {
			/*Not a Fetched Instruction*/
		}

		if (results[0] == BRANCH_FALSE) {
			newState.pc = state.pc + 1;
		} else {
			newState.pc = results[1];
		}

		/* -------------------------------------------;----- */
		/* ---------------------------------------------------- */
		/* --------------------- ID stage --------------------- */
		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */

		switch (opcode(state.IFID.instr)) {

		case ADD:
			regA = field0(state.IFID.instr);
			regB = field1(state.IFID.instr);
			hazard = hazardLocation(regA, regB);

			if (hazard == LWINSTRUCTION) {
				newState.IDEX.instr = NOOPINSTRUCTION;
				newState.IDEX.offset = field2(state.IFID.instr);
				newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
				newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
				newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
				newhazardState.first = NO_HAZARD;

				newState.IFID.pcPlus1 = state.IFID.pcPlus1;
				newState.IFID.instr = state.IFID.instr;
				newState.pc = state.pc;
			} else {
				newState.IDEX.instr = state.IFID.instr;
				newState.IDEX.offset = field2(state.IFID.instr);
				newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
				newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
				newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
				newState.IDEX.currHazard = hazard;
				newhazardState.first = field2(state.IFID.instr);
			}
			break;

		case NAND:
			regA = field0(state.IFID.instr);
			regB = field1(state.IFID.instr);
			hazard = hazardLocation(regA, regB);

			if (hazard == LWINSTRUCTION) {
				newState.IDEX.instr = NOOPINSTRUCTION;
				newState.IDEX.offset = field2(state.IFID.instr);
				newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
				newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
				newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
				newhazardState.first = NO_HAZARD;

				newState.IFID.pcPlus1 = state.IFID.pcPlus1;
				newState.IFID.instr = state.IFID.instr;
				newState.pc = state.pc;
			} else {
				newState.IDEX.instr = state.IFID.instr;
				newState.IDEX.offset = field2(state.IFID.instr);
				newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
				newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
				newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
				newState.IDEX.currHazard = hazard;
				newhazardState.first = field2(state.IFID.instr);
			}
			break;

		case MULT:
			regA = field0(state.IFID.instr);
			regB = field1(state.IFID.instr);
			hazard = hazardLocation(regA, regB);

			if (hazard == LWINSTRUCTION) {
				newState.IDEX.instr = NOOPINSTRUCTION;
				newState.IDEX.offset = field2(state.IFID.instr);
				newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
				newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
				newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
				newhazardState.first = NO_HAZARD;

				newState.IFID.pcPlus1 = state.IFID.pcPlus1;
				newState.IFID.instr = state.IFID.instr;
				newState.pc = state.pc;
			} else {
				newState.IDEX.instr = state.IFID.instr;
				newState.IDEX.offset = field2(state.IFID.instr);
				newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
				newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
				newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
				newState.IDEX.currHazard = hazard;
				newhazardState.first = field2(state.IFID.instr);
			}
			break;

		case LW:

			regA = field0(state.IFID.instr);
			regB = field1(state.IFID.instr);
			hazard = hazardLocation(regA, -1);

			if (hazard == LWINSTRUCTION) {
				newState.IDEX.instr = NOOPINSTRUCTION;
				newState.IDEX.offset = field2(state.IFID.instr);
				newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
				newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
				newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
				newhazardState.first = NO_HAZARD;

				newState.IFID.pcPlus1 = state.IFID.pcPlus1;
				newState.IFID.instr = state.IFID.instr;
				newState.pc = state.pc;
			} else {
				newState.IDEX.instr = state.IFID.instr;
				newState.IDEX.offset = field2(state.IFID.instr);
				newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
				newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
				newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
				newState.IDEX.currHazard = hazard;
				newhazardState.load = LWINSTRUCTION;
				newhazardState.first = field1(state.IFID.instr);
			}
			break;

		case SW:

			regA = field0(state.IFID.instr);
			regB = field1(state.IFID.instr);
			hazard = hazardLocation(regA, regB);

			if (hazard == LWINSTRUCTION) {
				newState.IDEX.instr = NOOPINSTRUCTION;
				newState.IDEX.offset = field2(state.IFID.instr);
				newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
				newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
				newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
				newhazardState.first = NO_HAZARD;

				newState.IFID.pcPlus1 = state.IFID.pcPlus1;
				newState.IFID.instr = state.IFID.instr;
				newState.pc = state.pc;
			} else {
				newState.IDEX.instr = state.IFID.instr;
				newState.IDEX.offset = field2(state.IFID.instr);
				newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
				newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
				newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
				newState.IDEX.currHazard = hazard;
				newhazardState.first = NO_HAZARD;
			}
			break;

		case BEQ:
			regA = field0(state.IFID.instr);
			regB = field1(state.IFID.instr);
			hazard = hazardLocation(regA, regB);

			if (hazard == LWINSTRUCTION) {
				newState.IDEX.instr = NOOPINSTRUCTION;
				newState.IDEX.offset = field2(state.IFID.instr);
				newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
				newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
				newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
				newhazardState.first = NO_HAZARD;

				newState.IFID.pcPlus1 = state.IFID.pcPlus1;
				newState.IFID.instr = state.IFID.instr;
				newState.pc = state.pc;
			} else {
				newState.IDEX.instr = state.IFID.instr;
				newState.IDEX.offset = field2(state.IFID.instr);
				newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
				newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
				newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
				newState.IDEX.currHazard = hazard;
				newhazardState.first = NO_HAZARD;
			}
			break;

		case HALT:
			newState.IDEX.instr = state.IFID.instr;
			newState.IDEX.offset = field2(state.IFID.instr);
			newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
			newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
			newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
			newhazardState.first = NO_HAZARD;

			break;

		case NOOP:
			newState.IDEX.instr = state.IFID.instr;
			newState.IDEX.offset = field2(state.IFID.instr);
			newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
			newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
			newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
			newhazardState.first = NO_HAZARD;
			break;

		default:
			newState.IDEX.instr = state.IFID.instr;
			newState.IDEX.offset = field2(state.IFID.instr);
			newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
			newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
			newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
			newhazardState.first = NO_HAZARD;

		}

		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		/* --------------------- EX stage --------------------- */
		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		switch (opcode(state.IDEX.instr)) {
		case ADD:
			val1 = state.IDEX.readRegA;
			val2 = state.IDEX.readRegB;
			dataForward(val1, val2, state.IDEX.currHazard, forwardValues);
			val1 = forwardValues[0];
			val2 = forwardValues[1];

			result = val1 + val2;
			newState.EXMEM.aluResult = result;
			newState.EXMEM.currHazard = state.IDEX.currHazard;
			newState.EXMEM.instr = state.IDEX.instr;
			newState.EXMEM.readRegB = val2;
			newState.EXMEM.branchTarget = state.IDEX.offset
					+ state.IDEX.pcPlus1;
			newhazardState.second = oldhazardState.first;
			break;

		case NAND:
			val1 = state.IDEX.readRegA;
			val2 = state.IDEX.readRegB;
			dataForward(val1, val2, state.IDEX.currHazard, forwardValues);
			val1 = forwardValues[0];
			val2 = forwardValues[1];

			result = ~(val1 & val2);
			newState.EXMEM.aluResult = result;
			newState.EXMEM.currHazard = state.IDEX.currHazard;
			newState.EXMEM.instr = state.IDEX.instr;
			newState.EXMEM.readRegB = val2;
			newState.EXMEM.branchTarget = state.IDEX.offset
					+ state.IDEX.pcPlus1;
			newhazardState.second = oldhazardState.first;

			break;

		case MULT:
			val1 = state.IDEX.readRegA;
			val2 = state.IDEX.readRegB;
			dataForward(val1, val2, state.IDEX.currHazard, forwardValues);
			val1 = forwardValues[0];
			val2 = forwardValues[1];

			result = val1 * val2;
			newState.EXMEM.aluResult = result;
			newState.EXMEM.currHazard = state.IDEX.currHazard;
			newState.EXMEM.instr = state.IDEX.instr;
			newState.EXMEM.readRegB = val2;
			newState.EXMEM.branchTarget = state.IDEX.offset
					+ state.IDEX.pcPlus1;
			newhazardState.second = oldhazardState.first;

			break;

		case LW:
			val1 = state.IDEX.readRegA;
			val2 = state.IDEX.readRegB;
			dataForward(val1, val2, state.IDEX.currHazard, forwardValues);
			val1 = forwardValues[0];
			val2 = forwardValues[1];
			result = val1 + state.IDEX.offset;
			newState.EXMEM.aluResult = result;
			newState.EXMEM.currHazard = state.IDEX.currHazard;
			newState.EXMEM.instr = state.IDEX.instr;
			newState.EXMEM.readRegB = val2;
			newState.EXMEM.branchTarget = state.IDEX.offset
					+ state.IDEX.pcPlus1;
			newhazardState.second = oldhazardState.first;

			/*For clearing load bit*/
			if (opcode(newState.IDEX.instr) != LW)
				newhazardState.load = NO_HAZARD;
			break;

		case SW:
			val1 = state.IDEX.readRegA;
			val2 = state.IDEX.readRegB;
			dataForward(val1, val2, state.IDEX.currHazard, forwardValues);
			val1 = forwardValues[0];
			val2 = forwardValues[1];

			result = val1 + state.IDEX.offset;
			newState.EXMEM.aluResult = result;
			newState.EXMEM.currHazard = state.IDEX.currHazard;
			newState.EXMEM.instr = state.IDEX.instr;
			newState.EXMEM.readRegB = val2;
			newState.EXMEM.branchTarget = state.IDEX.offset
					+ state.IDEX.pcPlus1;
			newhazardState.second = oldhazardState.first;

			break;

		case BEQ:

			val1 = state.IDEX.readRegA;
			val2 = state.IDEX.readRegB;
			dataForward(val1, val2, state.IDEX.currHazard, forwardValues);
			val1 = forwardValues[0];
			val2 = forwardValues[1];

			result = val1 ^ val2;
			newState.EXMEM.aluResult = result;
			newState.EXMEM.currHazard = state.IDEX.currHazard;
			newState.EXMEM.instr = state.IDEX.instr;
			newState.EXMEM.readRegB = val2;
			newState.EXMEM.branchTarget = state.IDEX.offset
					+ state.IDEX.pcPlus1;
			newhazardState.second = oldhazardState.first;

			break;

		case HALT:
			val1 = state.IDEX.readRegA;
			val2 = state.IDEX.readRegB;
			dataForward(val1, val2, state.IDEX.currHazard, forwardValues);
			val1 = forwardValues[0];
			val2 = forwardValues[1];
			newState.EXMEM.aluResult = result;
			newState.EXMEM.currHazard = state.IDEX.currHazard;
			newState.EXMEM.instr = state.IDEX.instr;
			newState.EXMEM.readRegB = val2;
			newState.EXMEM.branchTarget = state.IDEX.offset
					+ state.IDEX.pcPlus1;
			newhazardState.second = oldhazardState.first;
			break;

		case NOOP:
			val1 = state.IDEX.readRegA;
			val2 = state.IDEX.readRegB;
			dataForward(val1, val2, state.IDEX.currHazard, forwardValues);
			val1 = forwardValues[0];
			val2 = forwardValues[1];
			newState.EXMEM.aluResult = result;
			newState.EXMEM.currHazard = state.IDEX.currHazard;
			newState.EXMEM.instr = state.IDEX.instr;
			newState.EXMEM.readRegB = val2;
			newState.EXMEM.branchTarget = state.IDEX.offset
					+ state.IDEX.pcPlus1;
			newhazardState.second = oldhazardState.first;
			break;

		default:
			val1 = state.IDEX.readRegA;
			val2 = state.IDEX.readRegB;
			dataForward(val1, val2, state.IDEX.currHazard, forwardValues);
			val1 = forwardValues[0];
			val2 = forwardValues[1];
			newState.EXMEM.aluResult = result;
			newState.EXMEM.currHazard = state.IDEX.currHazard;
			newState.EXMEM.instr = state.IDEX.instr;
			newState.EXMEM.readRegB = val2;
			newState.EXMEM.branchTarget = state.IDEX.offset
					+ state.IDEX.pcPlus1;
			newhazardState.second = oldhazardState.first;
			break;

		}

		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		/* --------------------- MEM stage --------------------- */
		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */

		switch (opcode(state.EXMEM.instr)) {

		case ADD:
			newState.MEMWB.instr = state.EXMEM.instr;
			newState.MEMWB.writeData = state.EXMEM.aluResult;
			newState.MEMWB.currHazard = state.EXMEM.currHazard;
			newhazardState.third = oldhazardState.second;
			if (!(field0(state.EXMEM.instr) == 0
					&& field1(state.EXMEM.instr) == 0))
				newState.retired++;
			break;

		case NAND:
			newState.MEMWB.instr = state.EXMEM.instr;
			newState.MEMWB.writeData = state.EXMEM.aluResult;
			newState.MEMWB.currHazard = state.EXMEM.currHazard;
			newhazardState.third = oldhazardState.second;
			newState.retired++;
			break;

		case MULT:
			newState.MEMWB.instr = state.EXMEM.instr;
			newState.MEMWB.writeData = state.EXMEM.aluResult;
			newState.MEMWB.currHazard = state.EXMEM.currHazard;
			newhazardState.third = oldhazardState.second;
			newState.retired++;
			break;

		case LW:
			newState.MEMWB.instr = state.EXMEM.instr;
			newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
			newState.MEMWB.currHazard = state.EXMEM.currHazard;
			newhazardState.third = oldhazardState.second;
			newState.retired++;

			break;

		case SW:
			newState.MEMWB.currHazard = state.EXMEM.currHazard;
			newState.MEMWB.instr = state.EXMEM.instr;
			newState.dataMem[state.EXMEM.aluResult] = state.EXMEM.readRegB;
			newState.MEMWB.writeData = state.EXMEM.aluResult;
			newhazardState.third = oldhazardState.second;
			newState.retired++;
			break;

		case BEQ:
			/*branching logic implemented here*/
			newState.branches++; /*branch was executed and not flushed!*/
			if (state.EXMEM.aluResult == 0) {
				/*branching TRUE*/

				predictedPath = modifyBTB(
						state.EXMEM.branchTarget - field2(state.EXMEM.instr)
								- 1, state.EXMEM.branchTarget, BRANCH_TRUE);

				if (predictedPath == 1) {
					/*Branching was not predicted at IFID stage
					 * Need to flush out all the instructions
					 * after the branch*/
					newState.IFID.instr = NOOPINSTRUCTION;
					newState.IDEX.instr = NOOPINSTRUCTION;
					newState.EXMEM.instr = NOOPINSTRUCTION;

					newhazardState.first = EMPTY;
					newhazardState.second = EMPTY;
					newhazardState.load = NO_HAZARD;

					newState.pc = state.EXMEM.branchTarget;
				} else {
					/*Branch was predicted at IFID stage*/
				}

			} else {
				/*branching FALSE*/
				predictedPath = modifyBTB(
						state.EXMEM.branchTarget - field2(state.EXMEM.instr)
								- 1, state.EXMEM.branchTarget,
						BRANCH_FALSE);

				if (predictedPath == 1) {
					/*Branch was predicted at IFID but not TAKEN!
					 * MISPREDICTION by the branch predictor
					 * Need to flush out all the instructions
					 * after the branch*/
					newState.mispred++;
					newState.IFID.instr = NOOPINSTRUCTION;
					newState.IDEX.instr = NOOPINSTRUCTION;
					newState.EXMEM.instr = NOOPINSTRUCTION;

					newhazardState.first = EMPTY;
					newhazardState.second = EMPTY;
					newhazardState.load = NO_HAZARD;

					newState.pc = state.EXMEM.branchTarget
							- field2(state.EXMEM.instr);
				} else {
					/*Not Predicted/ Correct prediction!*/
				}
			}

			newState.MEMWB.currHazard = state.EXMEM.currHazard;
			newState.MEMWB.instr = state.EXMEM.instr;
			newState.MEMWB.writeData = state.EXMEM.aluResult;
			newhazardState.third = oldhazardState.second;
			newState.retired++;
			break;

		case HALT:
			/*do nothing;*/
			newState.MEMWB.instr = state.EXMEM.instr;
			newState.MEMWB.writeData = state.EXMEM.aluResult;
			newState.MEMWB.currHazard = state.EXMEM.currHazard;
			newhazardState.third = oldhazardState.second;
			newState.retired++;
			break;

		case NOOP:
			/*do nothing;*/
			newState.MEMWB.instr = state.EXMEM.instr;
			newState.MEMWB.writeData = state.EXMEM.aluResult;
			newState.MEMWB.currHazard = state.EXMEM.currHazard;
			newhazardState.third = oldhazardState.second;
			break;

		default:
			/*do nothing;*/
			newState.MEMWB.instr = state.EXMEM.instr;
			newState.MEMWB.writeData = state.EXMEM.aluResult;
			newState.MEMWB.currHazard = state.EXMEM.currHazard;
			newhazardState.third = oldhazardState.second;
			break;
		}

		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		/* --------------------- WB stage --------------------- */
		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */

		switch (opcode(state.MEMWB.instr)) {

		case ADD:
			newState.WBEND.instr = state.MEMWB.instr;
			newState.WBEND.writeData = state.MEMWB.writeData;
			newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
			newState.WBEND.currHazard = state.MEMWB.currHazard;

			break;

		case NAND:
			newState.WBEND.instr = state.MEMWB.instr;
			newState.WBEND.writeData = state.MEMWB.writeData;
			newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
			newState.WBEND.currHazard = state.MEMWB.currHazard;

			break;

		case MULT:
			newState.WBEND.instr = state.MEMWB.instr;
			newState.WBEND.writeData = state.MEMWB.writeData;
			newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
			newState.WBEND.currHazard = state.MEMWB.currHazard;
			break;

		case LW:
			newState.WBEND.instr = state.MEMWB.instr;
			newState.WBEND.writeData = state.MEMWB.writeData;
			newState.reg[field1(state.MEMWB.instr)] = state.MEMWB.writeData;
			newState.WBEND.currHazard = state.MEMWB.currHazard;

			break;

		case SW:
			/*do nothing;*/
			newState.WBEND.instr = state.MEMWB.instr;
			newState.WBEND.writeData = state.MEMWB.writeData;
			newState.WBEND.currHazard = state.MEMWB.currHazard;
			break;

		case BEQ:
			/*do nothing;*/
			newState.WBEND.instr = state.MEMWB.instr;
			newState.WBEND.writeData = state.MEMWB.writeData;
			newState.WBEND.currHazard = state.MEMWB.currHazard;
			break;

		case HALT:
			/*do nothing;*/
			newState.WBEND.instr = state.MEMWB.instr;
			newState.WBEND.writeData = state.MEMWB.writeData;
			newState.WBEND.currHazard = state.MEMWB.currHazard;
			break;

		case NOOP:
			/*do nothing;*/
			newState.WBEND.instr = state.MEMWB.instr;
			newState.WBEND.writeData = state.MEMWB.writeData;
			newState.WBEND.currHazard = state.MEMWB.currHazard;
			break;

		default:
			/*do nothing;*/
			newState.WBEND.instr = state.MEMWB.instr;
			newState.WBEND.writeData = state.MEMWB.writeData;
			newState.WBEND.currHazard = state.MEMWB.currHazard;
			break;
		}

		oldhazardState = newhazardState;
		state = newState; /* this is the last statement before end of the loop.
		 It marks the end of the cycle and updates the
		 current state with the values calculated in this
		 cycle */
	}
}

int main(int argc, char **argv) {
	int i = 0, j = 0, count = 0;

	/* --------------------- Get the instructions --------------------- */
	FILE *fp = fopen(argv[1], "r");
	while (fscanf(fp, "%d\n", &i) == 1) {
		state.instrMem[count] = i;
		state.dataMem[count] = i;
		count++;
	}
	fclose(fp);

	/* --------------------- Initializing state --------------------- */
	state.pc = 0;
	state.IFID.pcPlus1 = 0;
	state.cycles = 0;
	state.fetched = 0;
	state.retired = 0;
	state.branches = 0;
	state.mispred = 0;

	state.numMemory = count;
	state.IFID.instr = NOOPINSTRUCTION;
	state.IDEX.instr = NOOPINSTRUCTION;
	state.EXMEM.instr = NOOPINSTRUCTION;
	state.MEMWB.instr = NOOPINSTRUCTION;
	state.WBEND.instr = NOOPINSTRUCTION;
	state.IDEX.currHazard = NO_HAZARD;
	state.EXMEM.currHazard = NO_HAZARD;
	state.MEMWB.currHazard = NO_HAZARD;
	state.WBEND.currHazard = NO_HAZARD;

	oldhazardState.first = EMPTY;
	oldhazardState.second = EMPTY;
	oldhazardState.third = EMPTY;
	oldhazardState.load = NO_HAZARD;

	newhazardState = oldhazardState;
	for (i = 0; i < 8; i++)
		state.reg[i] = 0;

#if PREDICTOR == COUNTER_ONE_TO_ONE

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 3; j++) {
			branchBuffer.BTB[i][j] = EMPTY;
		}
	}

#elif PREDICTOR == LOCAL_HISTORY

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 2; j++) {
			branchBuffer.BTB[i][j] = EMPTY;
		}
	}

	for (i = 0; i < 4; i++) {
		branchHistoryTable.branchHistory[i] = 0;
	}

	for (i = 0; i < 4; i++) {
		patternHistoryTable.patternHistory[i] = WEAKLY_NOT_TAKEN;
	}
#else

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 2; j++) {
			branchBuffer.BTB[i][j] = EMPTY;
		}
	}

	for (i = 0; i < 4; i++) {
		branchHistoryTable.branchHistory[i] = WEAKLY_NOT_TAKEN;
	}

#endif
	/* --------------------- Initializing done --------------------- */

	run();

	printf("machine halted\n");
	printf("total of %d cycles executed\n", state.cycles);
	printf("%-10s: %d\n", "CYCLES", state.cycles);
	printf("%-10s: %d\n", "FETCHED", state.fetched);
	printf("%-10s: %d\n", "RETIRED", state.retired);
	printf("%-10s: %d\n", "BRANCHES", state.branches);
	printf("%-10s: %d\n", "MISPRED", state.mispred);

	exit(0);
}

