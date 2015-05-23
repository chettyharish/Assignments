#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define NUMMEMORY 				65536
#define NUMREGS 				8
#define NUMROB	 				16

#define ADD 					0
#define NAND 					1
#define LW 						2
#define SW 						3
#define BEQ 					4
#define MULT 					5
#define HALT 					6
#define NOOP 					7
#define NOINSTRUCTION			-1
#define NOOPINSTRUCTION 		0x1c00000

#define TRUE					0
#define FALSE					1
#define VALID					0
#define INVALID					1
#define NOTCALCULATED			0
#define CALCULATED				1
#define COMMITTED				0
#define NOTCOMMITTED			1
#define EMPTY					-1

/*Branch Prediction Logic*/
#define BRANCH_TRUE 			0
#define BRANCH_FALSE 			1
#define STRONGLY_NOT_TAKEN 		0
#define WEAKLY_NOT_TAKEN 		1
#define WEAKLY_TAKEN 			2
#define STRONGLY_TAKEN 			3
#define EMPTY					-1

#define MOREOUTPUT 				TRUE /*Set to TRUE for additional printouts*/

typedef struct ROBEntryStruct {
	int pc;
	int destReg;
	int destValue;
	int validValue;
	int operation;
	int offset;
	int instruction;
} ROBEntryStruct;

typedef struct ROBStruct {
	ROBEntryStruct ROBENTRY[NUMROB];
	int head;
	int tail;
	int full;
} ROBStruct;

typedef struct UnitRSStruct {
	int busy;
	int scheduled;
	int physicalSrc1;
	int physicalSrc2;
	int physicalDest;
	int readSrc1;
	int readSrc2;
	int validSrc1;
	int validSrc2;
	int offset;
	int operation;
	int cycleID;
} UnitRSStruct;

typedef struct RSStruct {
	UnitRSStruct MultiplyRS[3];
	UnitRSStruct IntegerRS[3];
	UnitRSStruct LoadRS[3];
} RSStruct;

typedef struct RTStruct {
	int renameTable[NUMREGS][4];
} RTStruct;

typedef struct MultStageStruct {
	int operation;
	int readSrc1;
	int readSrc2;
	int dest;
} MultStageStruct;

typedef struct IntStageStruct {
	int operation;
	int readSrc1;
	int readSrc2;
	int dest;
} IntStageStruct;

typedef struct LoadStageStruct {
	int operation;
	int readSrc1;
	int readSrc2;
	int offset;
	int dest;
} LoadStageStruct;

typedef struct MultStruct {
	MultStageStruct stage[6];
} MultStruct;

typedef struct IntStruct {
	IntStageStruct stage[1];
} IntStruct;

typedef struct LoadStruct {
	LoadStageStruct stage[3];
} LoadStruct;

typedef struct FetchStruct {
	int instr1;
	int pcinstr1;
	int instr2;
	int pcinstr2;
} FetchStruct;

typedef struct stateStruct {
	int pc;
	int instrMem[NUMMEMORY];
	int dataMem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
	FetchStruct Fetch;
	ROBStruct ROB;
	RSStruct RS;
	RTStruct RT;
	MultStruct multUnit;
	IntStruct intUnit;
	LoadStruct loadUnit;
	int cycles; /* number of cycles run so far */
	int fetched; /*# of instructions fetched*/
	int retired; /*# of instruction completed*/
	int branches;/*# of branches executed (i.e., resolved)*/
	int mispred; /*# of branches incorrectly predicted*/
} stateStruct;

typedef struct branchHistoryStruct {
	int branchHistory[64];
} branchHistoryStruct;

typedef struct branchStruct {
	/* 0: instruction 		1: target 		2: LRU tracker */
	int BTB[3][3];
	int curPos;
} branchStruct;

stateStruct state;
stateStruct newState;
branchStruct branchBuffer;
branchHistoryStruct branchHistoryTable;

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
	if (instruction == NOINSTRUCTION)
		return -1;
	return (instruction >> 22);
}

int lsbBits(int pc) {
	return (pc & 0x3F);
}

int modifyPos( position) {
	/*Returns the branch which was accessed the most cycles ago*/
	int i = 0;
	int newPos = position;

	for (i = 0; i < 3; i++) {
		if (branchBuffer.BTB[i][2] > branchBuffer.BTB[newPos][2])
			newPos = i;
	}
	return newPos;
}

void incHead() {
	newState.ROB.head = (newState.ROB.head + 1) % NUMROB;
	newState.ROB.full = FALSE;
}

void incTail() {
	newState.ROB.tail = (newState.ROB.tail + 1) % NUMROB;
	if (newState.ROB.tail == newState.ROB.head) {
		newState.ROB.full = TRUE;
	}
}

int* RSallot(int instruction) {
	/*results[0] states that RS is available or not
	 *results[1] states the RS index
	 * */
	int i = 0;
	int *results = malloc(sizeof(int) * 2);
	results[0] = FALSE;
	results[1] = 0;

	if (instruction == NOINSTRUCTION) {
		results[0] = TRUE;
		results[1] = 0;
		return results;
	}

	switch (opcode(instruction)) {
	case ADD:
	case NAND:
	case BEQ:
		for (i = 0; i < 3; i++) {
			if (newState.RS.IntegerRS[i].busy != TRUE) {
				results[0] = TRUE;
				results[1] = i;
				return results;
			}
		}
		break;

	case MULT:
		for (i = 0; i < 3; i++) {
			if (newState.RS.MultiplyRS[i].busy != TRUE) {
				results[0] = TRUE;
				results[1] = i;
				return results;
			}
		}
		break;

	case HALT:
	case NOOP:
		results[0] = TRUE;
		results[1] = 0;
		return results;
		break;

	case LW:
	case SW:
		for (i = 0; i < 3; i++) {
			if (newState.RS.LoadRS[i].busy != TRUE) {
				results[0] = TRUE;
				results[1] = i;
				return results;
			}
		}
		break;

	}
	return results;
}

void incrementLRU(int position) {
	/*Sets the most recently accessed branch as 1*/
	int i = 0;
	for (i = 0; i < 3; i++) {
		if (position == i)
			branchBuffer.BTB[i][2] = 1;
		else
			branchBuffer.BTB[i][2]++;
	}
	branchBuffer.curPos = modifyPos(position);
}

int getPhysicalReg(int regNum) {
	if (newState.RT.renameTable[regNum][1] == VALID)
		return newState.RT.renameTable[regNum][0];
	else
		return -1;
}

int* readValue(int regNum) {
	/*Returns the register value and validity*/
	int *results = malloc(sizeof(int) * 2);
	if (newState.RT.renameTable[regNum][1] == INVALID) {
		results[0] = state.reg[regNum];
		results[1] = TRUE;
		return results;
	} else if (newState.ROB.ROBENTRY[newState.RT.renameTable[regNum][0]].validValue
			== CALCULATED) {
		results[0] =
				newState.ROB.ROBENTRY[newState.RT.renameTable[regNum][0]].destValue;
		results[1] = TRUE;
		return results;
	} else {
		results[0] =
				newState.ROB.ROBENTRY[newState.RT.renameTable[regNum][0]].destValue;
		results[1] = FALSE;
		return results;
	}
}

int modifyBTB(int pc, int target, int branchTaken) {
	int i = 0;
	int oldstate = EMPTY;
	if (branchTaken == BRANCH_TRUE) {
		/*If return is 0, the branch was correctly predicted at IF
		 * and the target was already fetched
		 *If return is 1, the branch was not predicted at IF
		 * and the ROB instructions have to be flushed*/
		for (i = 0; i < 3; i++) {
			if (pc == branchBuffer.BTB[i][0]) {
				/*Old state has current priority before updating*/
				oldstate = branchHistoryTable.branchHistory[lsbBits(
						branchBuffer.BTB[i][0])];
				if (oldstate != STRONGLY_TAKEN) {
					/*Increment only if not already STRONGLY_TAKEN*/
					branchHistoryTable.branchHistory[lsbBits(
							branchBuffer.BTB[i][0])]++;
				}

				incrementLRU(i);

				if (oldstate == STRONGLY_TAKEN || oldstate == WEAKLY_TAKEN)
					return 0;
				else {
					newState.mispred++;
					return 1;
				}
			}
		}
		/*If branch not already present, then insert it into BTB
		 * and reset the corresponding state to WEAKLY_TAKEN*/
		branchBuffer.BTB[branchBuffer.curPos][0] = pc;
		branchBuffer.BTB[branchBuffer.curPos][1] = target;
		oldstate = branchHistoryTable.branchHistory[lsbBits(
				branchBuffer.BTB[branchBuffer.curPos][0])];
		if(oldstate != STRONGLY_TAKEN){
			branchHistoryTable.branchHistory[lsbBits(
							branchBuffer.BTB[branchBuffer.curPos][0])]++;
		}
		incrementLRU(branchBuffer.curPos);
		return 1;
	} else {
		/*If return is 0, the branch was correctly predicted as NOT taken at IF
		 * and the PC+1 instruction was fetched
		 *If return is 1, the branch was predicted as TRUE at IF
		 * and the instructions have to be flushed*/
		/*If branch predicted by BTB, and not taken , then penalize*/
		for (i = 0; i < 3; i++) {
			if (pc == branchBuffer.BTB[i][0]) {
				oldstate = branchHistoryTable.branchHistory[lsbBits(
						branchBuffer.BTB[i][0])];
				if (oldstate != STRONGLY_NOT_TAKEN) {
					branchHistoryTable.branchHistory[lsbBits(
							branchBuffer.BTB[i][0])]--;
				}
				incrementLRU(i);
				/*If return is 1 , then the branch was incorrectly predicted*/
				if (oldstate == STRONGLY_TAKEN || oldstate == WEAKLY_TAKEN) {
					newState.mispred++;
					return 1;
				} else
					return 0;
			}
		}
		/*If branch not already present, then mis-prediction is
		 * not possible!*/
		branchBuffer.BTB[branchBuffer.curPos][0] = pc;
		branchBuffer.BTB[branchBuffer.curPos][1] = target;

		oldstate = branchHistoryTable.branchHistory[lsbBits(
				branchBuffer.BTB[branchBuffer.curPos][0])];
		if(oldstate != STRONGLY_NOT_TAKEN){
			branchHistoryTable.branchHistory[lsbBits(
							branchBuffer.BTB[branchBuffer.curPos][0])]--;
		}
		incrementLRU(branchBuffer.curPos);
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

	for (i = 0; i < 3; i++) {
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

void printOperation(int operation, int busy) {
	char opcodeString[10];
	if (busy == FALSE) {
		strcpy(opcodeString, "noins");
	} else if (operation == ADD) {
		strcpy(opcodeString, "add");
	} else if (operation == NAND) {
		strcpy(opcodeString, "nand");
	} else if (operation == LW) {
		strcpy(opcodeString, "lw");
	} else if (operation == MULT) {
		strcpy(opcodeString, "mult");
	} else if (operation == SW) {
		strcpy(opcodeString, "sw");
	} else if (operation == BEQ) {
		strcpy(opcodeString, "beq");
	} else if (operation == HALT) {
		strcpy(opcodeString, "halt");
	} else if (operation == NOOP) {
		strcpy(opcodeString, "noop");
	} else {
		strcpy(opcodeString, "data");
	}
	printf("%s ", opcodeString);
}

void printState() {
	int i;
	printf("\n@@@\nstate before cycle %d starts\n", state.cycles);
	printf("\tpc %d\n", state.pc);

#if MOREOUTPUT == TRUE
	printf("\n\tFetch:\n");
	printf("\t\tPC : %d\tINSTR1\t:", state.Fetch.pcinstr1);
	printInstruction(state.Fetch.instr1);
	printf("\t\tPC : %d\tINSTR2\t:", state.Fetch.pcinstr2);
	printInstruction(state.Fetch.instr2);

	printf("\tdata memory:\n");
	for (i = 0; i < state.numMemory; i++) {
		printf("\t\tdataMem[ %d ] %d\n", i, state.dataMem[i]);
	}
	printf("\n\tregisters:\n");
	for (i = 0; i < NUMREGS; i++) {
		printf("\t\treg[ %d ] %d\n", i, state.reg[i]);
	}


	printf("\t BTB:\n");
	printf("\t\tcurpos: %d\n", branchBuffer.curPos);
	printf("\t\t%-10d\t%-10d\t%-10d\t\n", branchBuffer.BTB[0][0],
			branchBuffer.BTB[0][1], branchBuffer.BTB[0][2]);
	printf("\t\t%-10d\t%-10d\t%-10d\t\n", branchBuffer.BTB[1][0],
			branchBuffer.BTB[1][1], branchBuffer.BTB[1][2]);
	printf("\t\t%-10d\t%-10d\t%-10d\t\n", branchBuffer.BTB[2][0],
			branchBuffer.BTB[2][1], branchBuffer.BTB[2][2]);

	printf("\t BHT:\n");
	for(i = 0;i<64;i++) {
		printf("\t\t%-10d\n", branchHistoryTable.branchHistory[i]);
	}

	printf("\n\tFetch:\n");
	printf("\t\tINSTR1\t:");
	printInstruction(state.Fetch.instr1);
	printf("\t\tINSTR2\t:");
	printInstruction(state.Fetch.instr2);

	printf("\n\tROB:\n");
	printf("\t\thead: %d\n", state.ROB.head);
	printf("\t\ttail: %d\n", state.ROB.tail);

	printf(
			"\t\t\t\toperation\t\tdestReg\t\tdestValue\t\tvalidValue\t\toffset\n");
	for (i = 0; i < NUMROB; i++) {
		printf("\t\tROB [%2d]\t%d", i, state.ROB.ROBENTRY[i].operation);
		printf("\t\t\t%d", state.ROB.ROBENTRY[i].destReg);
		printf("\t\t%d", state.ROB.ROBENTRY[i].destValue);
		printf("\t\t\t%d", state.ROB.ROBENTRY[i].validValue);
		printf("\t\t\t%d\n", state.ROB.ROBENTRY[i].offset);
	}

	printf("\n\tRT:\n\t\t");
	for (i = 0; i < NUMREGS; i++) {
		printf("\t\treg[%d]", i);
	}
	printf("\n\t\tpReg:");
	for (i = 0; i < NUMREGS; i++) {
		printf("\t\t%d", state.RT.renameTable[i][0]);
	}
	printf("\n\t\tvalid:");
	for (i = 0; i < NUMREGS; i++) {
		printf("\t\t%d", state.RT.renameTable[i][1]);
	}
	printf("\n\t\tvalue:");
	for (i = 0; i < NUMREGS; i++) {
		printf("\t\t%d", state.RT.renameTable[i][2]);
	}
	printf("\n\t\tcalc:");
	for (i = 0; i < NUMREGS; i++) {
		printf("\t\t%d", state.RT.renameTable[i][3]);
	}

	printf("\n\n\tRS:\n");
	printf("\t\tMULTIPLY\n");
	printf(
			"\t\t\toperation\tbusy\t\tsch\tphysicalDest\tphysicalSrc1\tphysicalSrc2");
	printf("\treadSrc1\treadSrc2\tvalidSrc1\tvalidSrc2\tcycleNo\n");
	for (i = 0; i < 3; i++) {
		printf("\t\t\t");
		printOperation(state.RS.MultiplyRS[i].operation,
				state.RS.MultiplyRS[i].busy);
		printf("\t\t%d", state.RS.MultiplyRS[i].busy);
		printf("\t\t%d", state.RS.MultiplyRS[i].scheduled);
		printf("\t\t%d", state.RS.MultiplyRS[i].physicalDest);
		printf("\t\t%d", state.RS.MultiplyRS[i].physicalSrc1);
		printf("\t\t%d", state.RS.MultiplyRS[i].physicalSrc2);
		printf("\t\t%d", state.RS.MultiplyRS[i].readSrc1);
		printf("\t\t%d", state.RS.MultiplyRS[i].readSrc2);
		printf("\t\t%d", state.RS.MultiplyRS[i].validSrc1);
		printf("\t\t%d", state.RS.MultiplyRS[i].validSrc2);
		printf("\t\t%d\n", state.RS.MultiplyRS[i].cycleID);
	}

	printf("\n\t\tADD/NAND/BEQ\n");
	printf(
			"\t\t\toperation\tbusy\t\tsch\tphysicalDest\tphysicalSrc1\tphysicalSrc2");
	printf("\treadSrc1\treadSrc2\tvalidSrc1\tvalidSrc2\tcycleNo\n");
	for (i = 0; i < 3; i++) {
		printf("\t\t\t");
		printOperation(state.RS.IntegerRS[i].operation,
				state.RS.IntegerRS[i].busy);
		printf("\t\t%d", state.RS.IntegerRS[i].busy);
		printf("\t\t%d", state.RS.IntegerRS[i].scheduled);
		printf("\t\t%d", state.RS.IntegerRS[i].physicalDest);
		printf("\t\t%d", state.RS.IntegerRS[i].physicalSrc1);
		printf("\t\t%d", state.RS.IntegerRS[i].physicalSrc2);
		printf("\t\t%d", state.RS.IntegerRS[i].readSrc1);
		printf("\t\t%d", state.RS.IntegerRS[i].readSrc2);
		printf("\t\t%d", state.RS.IntegerRS[i].validSrc1);
		printf("\t\t%d", state.RS.IntegerRS[i].validSrc2);
		printf("\t\t%d\n", state.RS.IntegerRS[i].cycleID);
	}

	printf("\n\t\tLOAD/STORE\n");
	printf(
			"\t\t\toperation\tbusy\t\tsch\tphysicalDest\tphysicalSrc1\tphysicalSrc2");
	printf("\treadSrc1\treadSrc2\toffset\t\tvalidSrc1\tvalidSrc2\tcycleNo\n");
	for (i = 0; i < 3; i++) {
		printf("\t\t\t");
		printOperation(state.RS.LoadRS[i].operation, state.RS.LoadRS[i].busy);
		printf("\t\t%d", state.RS.LoadRS[i].busy);
		printf("\t\t%d", state.RS.LoadRS[i].scheduled);
		printf("\t\t%d", state.RS.LoadRS[i].physicalDest);
		printf("\t\t%d", state.RS.LoadRS[i].physicalSrc1);
		printf("\t\t%d", state.RS.LoadRS[i].physicalSrc2);
		printf("\t\t%d", state.RS.LoadRS[i].readSrc1);
		printf("\t\t%d", state.RS.LoadRS[i].readSrc2);
		printf("\t\t%d", state.RS.LoadRS[i].offset);
		printf("\t\t%d", state.RS.LoadRS[i].validSrc1);
		printf("\t\t%d", state.RS.LoadRS[i].validSrc2);
		printf("\t\t%d\n", state.RS.LoadRS[i].cycleID);
	}

	printf("\n\n\tMULTIPLY UNIT:\n\t\t\t");
	for (i = 0; i < 6; i++) {
		printf("\tstage[%d]", i);
	}
	printf("\n\t\toperation:\t");
	for (i = 0; i < 6; i++) {
		printf("%d\t\t", state.multUnit.stage[i].operation);
	}
	printf("\n\t\treadSrc1:\t");
	for (i = 0; i < 6; i++) {
		printf("%d\t\t", state.multUnit.stage[i].readSrc1);
	}
	printf("\n\t\treadSrc2:\t");
	for (i = 0; i < 6; i++) {
		printf("%d\t\t", state.multUnit.stage[i].readSrc2);
	}
	printf("\n\t\tdest:\t\t");
	for (i = 0; i < 6; i++) {
		printf("%d\t\t", state.multUnit.stage[i].dest);
	}

	printf("\n\n\tINTEGER UNIT:\n\t\t\t");
	for (i = 0; i < 1; i++) {
		printf("\tstage[%d]", i);
	}
	printf("\n\t\toperation:\t");
	for (i = 0; i < 1; i++) {
		printf("%d\t\t", state.intUnit.stage[i].operation);
	}
	printf("\n\t\treadSrc1:\t");
	for (i = 0; i < 1; i++) {
		printf("%d\t\t", state.intUnit.stage[i].readSrc1);
	}
	printf("\n\t\treadSrc2:\t");
	for (i = 0; i < 1; i++) {
		printf("%d\t\t", state.intUnit.stage[i].readSrc2);
	}

	printf("\n\t\tdest:\t\t");
	for (i = 0; i < 1; i++) {
		printf("%d\t\t", state.intUnit.stage[i].dest);
	}

	printf("\n\n\tLOAD UNIT:\n\t\t\t");
	for (i = 0; i < 3; i++) {
		printf("\tstage[%d]", i);
	}
	printf("\n\t\toperation:\t");
	for (i = 0; i < 3; i++) {
		printf("%d\t\t", state.loadUnit.stage[i].operation);
	}
	printf("\n\t\treadSrc1:\t");
	for (i = 0; i < 3; i++) {
		printf("%d\t\t", state.loadUnit.stage[i].readSrc1);
	}
	printf("\n\t\treadSrc2:\t");
	for (i = 0; i < 3; i++) {
		printf("%d\t\t", state.loadUnit.stage[i].readSrc2);
	}

	printf("\n\t\tdest:\t\t");
	for (i = 0; i < 3; i++) {
		printf("%d\t\t", state.loadUnit.stage[i].dest);
	}

	printf("\n\t\toffset:\t\t");
	for (i = 0; i < 3; i++) {
		printf("%d\t\t", state.loadUnit.stage[i].offset);
	}
	printf("\n");
#endif

	printf("\n\tROB INSTRUCTIONS (NEW FIRST):\n");
	printf("\t\thead: %d\n", state.ROB.head);
	printf("\t\ttail: %d\n\n", state.ROB.tail);
	i = state.ROB.tail;
	printf("\t\tROB_index\t\tinstruction\n\n");
	while (i != state.ROB.head) {
		i--;
		if (i == -1)
			i = NUMROB - 1;
		printf("\t\t\t%d\t\t", i);

		printInstruction(state.ROB.ROBENTRY[i].instruction);

	}

}

void run() {
	int value, i, pos = -1, insCycle = INT_MAX, predictedPath = -1, brflag =
	FALSE;
	int *resultsBP1, *resultsBP2, *resultsRS, *resultsReadReg;
	while (1) {
		newState = state;
		//system("read x");
		newState.cycles++;

		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		/* ---------------------- IF stage -------------------- */
		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		newState.Fetch.instr1 = state.instrMem[state.pc];
		newState.Fetch.pcinstr1 = state.pc;
		newState.Fetch.instr2 = state.instrMem[state.pc + 1];
		newState.Fetch.pcinstr2 = state.pc + 1;
		resultsBP1 = predictBTB(state.pc, state.instrMem[state.pc]);
		resultsBP2 = predictBTB(state.pc + 1, state.instrMem[state.pc + 1]);

		printState();

		if (resultsBP1[0] == BRANCH_TRUE) {
			/*First instruction is a branch and is predicted true*/
			newState.Fetch.instr2 = NOINSTRUCTION;
			newState.Fetch.pcinstr2 = INVALID;
			newState.pc = state.pc + 1 + resultsBP1[1];
		} else if (opcode(state.instrMem[state.pc]) == BEQ
				&& opcode(state.instrMem[state.pc + 1]) == BEQ) {
			/*Both instructions are Branches*/
			newState.Fetch.instr2 = NOINSTRUCTION;
			newState.Fetch.pcinstr2 = INVALID;
			newState.pc = state.pc + 1;
		} else if (resultsBP2[0] == BRANCH_TRUE) {
			/*Second instruction is a branch and is predicted true*/
			newState.pc = state.pc + 1 + 1 + resultsBP2[1];
		} else {
			/*First instruction is predicted as a
			 * non-branching branch*/
			newState.pc = state.pc + 2;
		}

		if (!(opcode(newState.Fetch.instr1) > 7
				|| opcode(newState.Fetch.instr1) < 0
				|| (opcode(newState.Fetch.instr1) == ADD
						&& field0(newState.Fetch.instr1) == 0
						&& field1(newState.Fetch.instr1) == 0)))
			newState.fetched++;
		else {
			/*Not a Instruction*/
		}

		if (!(opcode(newState.Fetch.instr2) > 7
				|| opcode(newState.Fetch.instr2) < 0
				|| (opcode(newState.Fetch.instr2) == ADD
						&& field0(newState.Fetch.instr2) == 0
						&& field1(newState.Fetch.instr2) == 0)))
			newState.fetched++;
		else {
			/*Not a Instruction -->*/
		}
		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		/* --------------------- RENAME/ALLOC stage ----------- */
		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		resultsRS = RSallot(state.Fetch.instr1);
		if (newState.ROB.full != TRUE && resultsRS[0] == TRUE) {
			switch (opcode(state.Fetch.instr1)) {
			case ADD:
			case NAND:

				newState.ROB.ROBENTRY[newState.ROB.tail].destReg = field2(
						state.Fetch.instr1);
				newState.ROB.ROBENTRY[newState.ROB.tail].destValue = EMPTY;
				newState.ROB.ROBENTRY[newState.ROB.tail].operation = opcode(
						state.Fetch.instr1);
				newState.ROB.ROBENTRY[newState.ROB.tail].validValue =
				NOTCALCULATED;
				newState.ROB.ROBENTRY[newState.ROB.tail].pc =
						state.Fetch.pcinstr1;
				newState.ROB.ROBENTRY[newState.ROB.tail].instruction =
						state.Fetch.instr1;

				/*Storing logic in RS is:
				 * cycleNo * 10 + 1 for first instruction
				 * cycleNo * 10 + 2 for next instruction in same cycle.
				 * Helps to distinguish both inter and intracycle instruction */
				newState.RS.IntegerRS[resultsRS[1]].busy = TRUE;
				newState.RS.IntegerRS[resultsRS[1]].cycleID = state.cycles * 10
						+ 1;
				newState.RS.IntegerRS[resultsRS[1]].operation = opcode(
						state.Fetch.instr1);
				newState.RS.IntegerRS[resultsRS[1]].physicalSrc1 =
						getPhysicalReg(field0(state.Fetch.instr1));
				newState.RS.IntegerRS[resultsRS[1]].physicalSrc2 =
						getPhysicalReg(field1(state.Fetch.instr1));
				newState.RS.IntegerRS[resultsRS[1]].physicalDest =
						newState.ROB.tail;

				resultsReadReg = readValue(field0(state.Fetch.instr1));
				newState.RS.IntegerRS[resultsRS[1]].readSrc1 =
						resultsReadReg[0];
				newState.RS.IntegerRS[resultsRS[1]].validSrc1 =
						resultsReadReg[1];

				resultsReadReg = readValue(field1(state.Fetch.instr1));
				newState.RS.IntegerRS[resultsRS[1]].readSrc2 =
						resultsReadReg[0];
				newState.RS.IntegerRS[resultsRS[1]].validSrc2 =
						resultsReadReg[1];

				newState.RT.renameTable[field2(state.Fetch.instr1)][0] =
						newState.ROB.tail;
				newState.RT.renameTable[field2(state.Fetch.instr1)][1] = VALID;
				incTail();
				break;

			case BEQ:
				newState.ROB.ROBENTRY[newState.ROB.tail].offset = field2(
						state.Fetch.instr1);
				newState.ROB.ROBENTRY[newState.ROB.tail].destReg = EMPTY;
				newState.ROB.ROBENTRY[newState.ROB.tail].destValue = EMPTY;
				newState.ROB.ROBENTRY[newState.ROB.tail].operation = opcode(
						state.Fetch.instr1);
				newState.ROB.ROBENTRY[newState.ROB.tail].validValue =
				NOTCALCULATED;
				newState.ROB.ROBENTRY[newState.ROB.tail].pc =
						state.Fetch.pcinstr1;
				newState.ROB.ROBENTRY[newState.ROB.tail].instruction =
						state.Fetch.instr1;

				/*Adds no entry in Rename Table*/
				newState.RS.IntegerRS[resultsRS[1]].busy = TRUE;
				newState.RS.IntegerRS[resultsRS[1]].cycleID = state.cycles * 10
						+ 1;
				newState.RS.IntegerRS[resultsRS[1]].operation = opcode(
						state.Fetch.instr1);
				newState.RS.IntegerRS[resultsRS[1]].physicalSrc1 =
						getPhysicalReg(field0(state.Fetch.instr1));
				newState.RS.IntegerRS[resultsRS[1]].physicalSrc2 =
						getPhysicalReg(field1(state.Fetch.instr1));
				newState.RS.IntegerRS[resultsRS[1]].physicalDest =
						newState.ROB.tail;

				resultsReadReg = readValue(field0(state.Fetch.instr1));
				newState.RS.IntegerRS[resultsRS[1]].readSrc1 =
						resultsReadReg[0];
				newState.RS.IntegerRS[resultsRS[1]].validSrc1 =
						resultsReadReg[1];

				resultsReadReg = readValue(field1(state.Fetch.instr1));
				newState.RS.IntegerRS[resultsRS[1]].readSrc2 =
						resultsReadReg[0];
				newState.RS.IntegerRS[resultsRS[1]].validSrc2 =
						resultsReadReg[1];
				incTail();
				break;

			case MULT:
				newState.ROB.ROBENTRY[newState.ROB.tail].destReg = field2(
						state.Fetch.instr1);
				newState.ROB.ROBENTRY[newState.ROB.tail].destValue = EMPTY;
				newState.ROB.ROBENTRY[newState.ROB.tail].operation = opcode(
						state.Fetch.instr1);
				newState.ROB.ROBENTRY[newState.ROB.tail].validValue =
				NOTCALCULATED;
				newState.ROB.ROBENTRY[newState.ROB.tail].pc =
						state.Fetch.pcinstr1;
				newState.ROB.ROBENTRY[newState.ROB.tail].instruction =
						state.Fetch.instr1;

				/*Storing logic in RS is:
				 * cycleNo * 10 + 1 for first instruction
				 * cycleNo * 10 + 2 for next instruction in same cycle.
				 * Helps to distinguish both inter and intracycle instruction */

				newState.RS.MultiplyRS[resultsRS[1]].busy = TRUE;
				newState.RS.MultiplyRS[resultsRS[1]].cycleID = state.cycles * 10
						+ 1;
				newState.RS.MultiplyRS[resultsRS[1]].operation = opcode(
						state.Fetch.instr1);
				newState.RS.MultiplyRS[resultsRS[1]].physicalSrc1 =
						getPhysicalReg(field0(state.Fetch.instr1));
				newState.RS.MultiplyRS[resultsRS[1]].physicalSrc2 =
						getPhysicalReg(field1(state.Fetch.instr1));
				newState.RS.MultiplyRS[resultsRS[1]].physicalDest =
						newState.ROB.tail;

				resultsReadReg = readValue(field0(state.Fetch.instr1));
				newState.RS.MultiplyRS[resultsRS[1]].readSrc1 =
						resultsReadReg[0];
				newState.RS.MultiplyRS[resultsRS[1]].validSrc1 =
						resultsReadReg[1];

				resultsReadReg = readValue(field1(state.Fetch.instr1));
				newState.RS.MultiplyRS[resultsRS[1]].readSrc2 =
						resultsReadReg[0];
				newState.RS.MultiplyRS[resultsRS[1]].validSrc2 =
						resultsReadReg[1];

				newState.RT.renameTable[field2(state.Fetch.instr1)][0] =
						newState.ROB.tail;
				newState.RT.renameTable[field2(state.Fetch.instr1)][1] = VALID;
				incTail();
				break;

			case LW:
				newState.ROB.ROBENTRY[newState.ROB.tail].destReg = field1(
						state.Fetch.instr1);
				newState.ROB.ROBENTRY[newState.ROB.tail].destValue = EMPTY;
				newState.ROB.ROBENTRY[newState.ROB.tail].operation = opcode(
						state.Fetch.instr1);
				newState.ROB.ROBENTRY[newState.ROB.tail].validValue =
				NOTCALCULATED;
				newState.ROB.ROBENTRY[newState.ROB.tail].pc =
						state.Fetch.pcinstr1;
				newState.ROB.ROBENTRY[newState.ROB.tail].instruction =
						state.Fetch.instr1;

				newState.RS.LoadRS[resultsRS[1]].busy = TRUE;
				newState.RS.LoadRS[resultsRS[1]].cycleID = state.cycles * 10
						+ 1;
				newState.RS.LoadRS[resultsRS[1]].operation = opcode(
						state.Fetch.instr1);
				newState.RS.LoadRS[resultsRS[1]].physicalSrc1 = getPhysicalReg(
						field0(state.Fetch.instr1));
				newState.RS.LoadRS[resultsRS[1]].physicalSrc2 = EMPTY;
				newState.RS.LoadRS[resultsRS[1]].physicalDest =
						newState.ROB.tail;

				resultsReadReg = readValue(field0(state.Fetch.instr1));
				newState.RS.LoadRS[resultsRS[1]].readSrc1 = resultsReadReg[0];
				newState.RS.LoadRS[resultsRS[1]].validSrc1 = resultsReadReg[1];

				newState.RS.LoadRS[resultsRS[1]].readSrc2 = field2(
						state.Fetch.instr1);
				newState.RS.LoadRS[resultsRS[1]].validSrc2 = TRUE;

				newState.RS.LoadRS[resultsRS[1]].offset = field2(
						state.Fetch.instr1);

				newState.RT.renameTable[field1(state.Fetch.instr1)][0] =
						newState.ROB.tail;
				newState.RT.renameTable[field1(state.Fetch.instr1)][1] = VALID;
				incTail();
				break;

			case SW:
				newState.ROB.ROBENTRY[newState.ROB.tail].destReg = field1(
						state.Fetch.instr1);
				newState.ROB.ROBENTRY[newState.ROB.tail].destValue = EMPTY;
				newState.ROB.ROBENTRY[newState.ROB.tail].operation = opcode(
						state.Fetch.instr1);
				newState.ROB.ROBENTRY[newState.ROB.tail].validValue =
				NOTCALCULATED;
				newState.ROB.ROBENTRY[newState.ROB.tail].pc =
						state.Fetch.pcinstr1;
				newState.ROB.ROBENTRY[newState.ROB.tail].instruction =
						state.Fetch.instr1;

				/*Adds no entry in Rename Table*/
				newState.RS.LoadRS[resultsRS[1]].busy = TRUE;
				newState.RS.LoadRS[resultsRS[1]].cycleID = state.cycles * 10
						+ 1;
				newState.RS.LoadRS[resultsRS[1]].operation = opcode(
						state.Fetch.instr1);
				newState.RS.LoadRS[resultsRS[1]].physicalSrc1 = getPhysicalReg(
						field0(state.Fetch.instr1));
				newState.RS.LoadRS[resultsRS[1]].physicalSrc2 = getPhysicalReg(
						field1(state.Fetch.instr1));
				newState.RS.LoadRS[resultsRS[1]].physicalDest =
						newState.ROB.tail;

				resultsReadReg = readValue(field0(state.Fetch.instr1));
				newState.RS.LoadRS[resultsRS[1]].readSrc1 = resultsReadReg[0];
				newState.RS.LoadRS[resultsRS[1]].validSrc1 = resultsReadReg[1];

				resultsReadReg = readValue(field1(state.Fetch.instr1));
				newState.RS.LoadRS[resultsRS[1]].readSrc2 = resultsReadReg[0];
				newState.RS.LoadRS[resultsRS[1]].validSrc2 = resultsReadReg[1];

				newState.RS.LoadRS[resultsRS[1]].offset = field2(
						state.Fetch.instr1);
				incTail();
				break;

			case NOOP:
			case HALT:

				newState.ROB.ROBENTRY[newState.ROB.tail].destReg = EMPTY;
				newState.ROB.ROBENTRY[newState.ROB.tail].destValue = EMPTY;
				newState.ROB.ROBENTRY[newState.ROB.tail].operation = opcode(
						state.Fetch.instr1);
				newState.ROB.ROBENTRY[newState.ROB.tail].validValue =
				CALCULATED;
				newState.ROB.ROBENTRY[newState.ROB.tail].pc =
						state.Fetch.pcinstr1;
				newState.ROB.ROBENTRY[newState.ROB.tail].instruction =
						state.Fetch.instr1;
				/*Adds no entry in Rename Table*/
				/*Adds no entry in Reservation Station*/
				incTail();
				break;

			case NOINSTRUCTION:
				/*Does nothing!*/
				break;
			}

			/*Allocating FETCH2 Instruction*/
			resultsRS = RSallot(state.Fetch.instr2);
			if (newState.ROB.full != TRUE && resultsRS[0] == TRUE) {
				switch (opcode(state.Fetch.instr2)) {
				case ADD:
				case NAND:

					newState.ROB.ROBENTRY[newState.ROB.tail].destReg = field2(
							state.Fetch.instr2);
					newState.ROB.ROBENTRY[newState.ROB.tail].destValue = EMPTY;
					newState.ROB.ROBENTRY[newState.ROB.tail].operation = opcode(
							state.Fetch.instr2);
					newState.ROB.ROBENTRY[newState.ROB.tail].validValue =
					NOTCALCULATED;
					newState.ROB.ROBENTRY[newState.ROB.tail].pc =
							state.Fetch.pcinstr2;
					newState.ROB.ROBENTRY[newState.ROB.tail].instruction =
							state.Fetch.instr2;

					/*Storing logic in RS is:
					 * cycleNo * 10 + 2 for first instruction
					 * cycleNo * 10 + 2 for next instruction in same cycle.
					 * Helps to distinguish both inter and intracycle instruction */
					newState.RS.IntegerRS[resultsRS[1]].busy = TRUE;
					newState.RS.IntegerRS[resultsRS[1]].cycleID = state.cycles
							* 10 + 2;
					newState.RS.IntegerRS[resultsRS[1]].operation = opcode(
							state.Fetch.instr2);
					newState.RS.IntegerRS[resultsRS[1]].physicalSrc1 =
							getPhysicalReg(field0(state.Fetch.instr2));
					newState.RS.IntegerRS[resultsRS[1]].physicalSrc2 =
							getPhysicalReg(field1(state.Fetch.instr2));
					newState.RS.IntegerRS[resultsRS[1]].physicalDest =
							newState.ROB.tail;

					resultsReadReg = readValue(field0(state.Fetch.instr2));
					newState.RS.IntegerRS[resultsRS[1]].readSrc1 =
							resultsReadReg[0];
					newState.RS.IntegerRS[resultsRS[1]].validSrc1 =
							resultsReadReg[1];

					resultsReadReg = readValue(field1(state.Fetch.instr2));
					newState.RS.IntegerRS[resultsRS[1]].readSrc2 =
							resultsReadReg[0];
					newState.RS.IntegerRS[resultsRS[1]].validSrc2 =
							resultsReadReg[1];

					newState.RT.renameTable[field2(state.Fetch.instr2)][0] =
							newState.ROB.tail;
					newState.RT.renameTable[field2(state.Fetch.instr2)][1] =
					VALID;
					incTail();
					break;

				case BEQ:
					newState.ROB.ROBENTRY[newState.ROB.tail].offset = field2(
							state.Fetch.instr2);
					newState.ROB.ROBENTRY[newState.ROB.tail].destReg = EMPTY;
					newState.ROB.ROBENTRY[newState.ROB.tail].destValue = EMPTY;
					newState.ROB.ROBENTRY[newState.ROB.tail].operation = opcode(
							state.Fetch.instr2);
					newState.ROB.ROBENTRY[newState.ROB.tail].validValue =
					NOTCALCULATED;
					newState.ROB.ROBENTRY[newState.ROB.tail].pc =
							state.Fetch.pcinstr2;
					newState.ROB.ROBENTRY[newState.ROB.tail].instruction =
							state.Fetch.instr2;
					newState.RS.IntegerRS[resultsRS[1]].busy = TRUE;

					/*Adds no entry in Rename Table*/

					newState.RS.IntegerRS[resultsRS[1]].cycleID = state.cycles
							* 10 + 2;
					newState.RS.IntegerRS[resultsRS[1]].operation = opcode(
							state.Fetch.instr2);
					newState.RS.IntegerRS[resultsRS[1]].physicalSrc1 =
							getPhysicalReg(field0(state.Fetch.instr2));
					newState.RS.IntegerRS[resultsRS[1]].physicalSrc2 =
							getPhysicalReg(field1(state.Fetch.instr2));
					newState.RS.IntegerRS[resultsRS[1]].physicalDest =
							newState.ROB.tail;

					resultsReadReg = readValue(field0(state.Fetch.instr2));
					newState.RS.IntegerRS[resultsRS[1]].readSrc1 =
							resultsReadReg[0];
					newState.RS.IntegerRS[resultsRS[1]].validSrc1 =
							resultsReadReg[1];

					resultsReadReg = readValue(field1(state.Fetch.instr2));
					newState.RS.IntegerRS[resultsRS[1]].readSrc2 =
							resultsReadReg[0];
					newState.RS.IntegerRS[resultsRS[1]].validSrc2 =
							resultsReadReg[1];
					incTail();
					break;

				case MULT:
					newState.ROB.ROBENTRY[newState.ROB.tail].destReg = field2(
							state.Fetch.instr2);
					newState.ROB.ROBENTRY[newState.ROB.tail].destValue = EMPTY;
					newState.ROB.ROBENTRY[newState.ROB.tail].operation = opcode(
							state.Fetch.instr2);
					newState.ROB.ROBENTRY[newState.ROB.tail].validValue =
					NOTCALCULATED;
					newState.ROB.ROBENTRY[newState.ROB.tail].pc =
							state.Fetch.pcinstr2;
					newState.ROB.ROBENTRY[newState.ROB.tail].instruction =
							state.Fetch.instr2;

					/*Storing logic in RS is:
					 * cycleNo * 10 + 2 for first instruction
					 * cycleNo * 10 + 2 for next instruction in same cycle.
					 * Helps to distinguish both inter and intracycle instruction */

					newState.RS.MultiplyRS[resultsRS[1]].busy = TRUE;
					newState.RS.MultiplyRS[resultsRS[1]].cycleID = state.cycles
							* 10 + 2;
					newState.RS.MultiplyRS[resultsRS[1]].operation = opcode(
							state.Fetch.instr2);
					newState.RS.MultiplyRS[resultsRS[1]].physicalSrc1 =
							getPhysicalReg(field0(state.Fetch.instr2));
					newState.RS.MultiplyRS[resultsRS[1]].physicalSrc2 =
							getPhysicalReg(field1(state.Fetch.instr2));
					newState.RS.MultiplyRS[resultsRS[1]].physicalDest =
							newState.ROB.tail;

					resultsReadReg = readValue(field0(state.Fetch.instr2));
					newState.RS.MultiplyRS[resultsRS[1]].readSrc1 =
							resultsReadReg[0];
					newState.RS.MultiplyRS[resultsRS[1]].validSrc1 =
							resultsReadReg[1];

					resultsReadReg = readValue(field1(state.Fetch.instr2));
					newState.RS.MultiplyRS[resultsRS[1]].readSrc2 =
							resultsReadReg[0];
					newState.RS.MultiplyRS[resultsRS[1]].validSrc2 =
							resultsReadReg[1];

					newState.RT.renameTable[field2(state.Fetch.instr2)][0] =
							newState.ROB.tail;
					newState.RT.renameTable[field2(state.Fetch.instr2)][1] =
					VALID;
					incTail();
					break;

				case LW:
					newState.ROB.ROBENTRY[newState.ROB.tail].destReg = field1(
							state.Fetch.instr2);
					newState.ROB.ROBENTRY[newState.ROB.tail].destValue = EMPTY;
					newState.ROB.ROBENTRY[newState.ROB.tail].operation = opcode(
							state.Fetch.instr2);
					newState.ROB.ROBENTRY[newState.ROB.tail].validValue =
					NOTCALCULATED;
					newState.ROB.ROBENTRY[newState.ROB.tail].pc =
							state.Fetch.pcinstr2;
					newState.ROB.ROBENTRY[newState.ROB.tail].instruction =
							state.Fetch.instr2;

					newState.RS.LoadRS[resultsRS[1]].busy = TRUE;
					newState.RS.LoadRS[resultsRS[1]].cycleID = state.cycles * 10
							+ 2;
					newState.RS.LoadRS[resultsRS[1]].operation = opcode(
							state.Fetch.instr2);
					newState.RS.LoadRS[resultsRS[1]].physicalSrc1 =
							getPhysicalReg(field0(state.Fetch.instr2));
					newState.RS.LoadRS[resultsRS[1]].physicalSrc2 = EMPTY;
					newState.RS.LoadRS[resultsRS[1]].physicalDest =
							newState.ROB.tail;

					resultsReadReg = readValue(field0(state.Fetch.instr2));
					newState.RS.LoadRS[resultsRS[1]].readSrc1 =
							resultsReadReg[0];
					newState.RS.LoadRS[resultsRS[1]].validSrc1 =
							resultsReadReg[1];

					newState.RS.LoadRS[resultsRS[1]].readSrc2 = field2(
							state.Fetch.instr2);
					newState.RS.LoadRS[resultsRS[1]].validSrc2 = TRUE;

					newState.RS.LoadRS[resultsRS[1]].offset = field2(
							state.Fetch.instr2);

					newState.RT.renameTable[field1(state.Fetch.instr2)][0] =
							newState.ROB.tail;
					newState.RT.renameTable[field1(state.Fetch.instr2)][1] =
					VALID;
					incTail();
					break;

				case SW:
					newState.ROB.ROBENTRY[newState.ROB.tail].destReg = field1(
							state.Fetch.instr2);
					newState.ROB.ROBENTRY[newState.ROB.tail].destValue = EMPTY;
					newState.ROB.ROBENTRY[newState.ROB.tail].operation = opcode(
							state.Fetch.instr2);
					newState.ROB.ROBENTRY[newState.ROB.tail].validValue =
					NOTCALCULATED;
					newState.ROB.ROBENTRY[newState.ROB.tail].pc =
							state.Fetch.pcinstr2;
					newState.ROB.ROBENTRY[newState.ROB.tail].instruction =
							state.Fetch.instr2;

					/*Adds no entry in Rename Table*/
					newState.RS.LoadRS[resultsRS[1]].busy = TRUE;
					newState.RS.LoadRS[resultsRS[1]].cycleID = state.cycles * 10
							+ 2;
					newState.RS.LoadRS[resultsRS[1]].operation = opcode(
							state.Fetch.instr2);
					newState.RS.LoadRS[resultsRS[1]].physicalSrc1 =
							getPhysicalReg(field0(state.Fetch.instr2));
					newState.RS.LoadRS[resultsRS[1]].physicalSrc2 =
							getPhysicalReg(field1(state.Fetch.instr2));
					newState.RS.LoadRS[resultsRS[1]].physicalDest =
							newState.ROB.tail;

					resultsReadReg = readValue(field0(state.Fetch.instr2));
					newState.RS.LoadRS[resultsRS[1]].readSrc1 =
							resultsReadReg[0];
					newState.RS.LoadRS[resultsRS[1]].validSrc1 =
							resultsReadReg[1];

					resultsReadReg = readValue(field1(state.Fetch.instr2));
					newState.RS.LoadRS[resultsRS[1]].readSrc2 =
							resultsReadReg[0];
					newState.RS.LoadRS[resultsRS[1]].validSrc2 =
							resultsReadReg[1];

					newState.RS.LoadRS[resultsRS[1]].offset = field2(
							state.Fetch.instr2);
					incTail();
					break;

				case NOOP:
				case HALT:

					newState.ROB.ROBENTRY[newState.ROB.tail].destReg = EMPTY;
					newState.ROB.ROBENTRY[newState.ROB.tail].destValue = EMPTY;
					newState.ROB.ROBENTRY[newState.ROB.tail].operation = opcode(
							state.Fetch.instr2);
					newState.ROB.ROBENTRY[newState.ROB.tail].validValue =
					CALCULATED;
					newState.ROB.ROBENTRY[newState.ROB.tail].pc =
							state.Fetch.pcinstr2;
					newState.ROB.ROBENTRY[newState.ROB.tail].instruction =
							state.Fetch.instr2;
					/*Adds no entry in Rename Table*/
					/*Adds no entry in Reservation Station*/
					incTail();
					break;

				case NOINSTRUCTION:
					/*Does nothing!*/
					break;
				}
			} else {
				/*Only one instruction has allocated*/
				newState.pc = newState.Fetch.pcinstr1;
				newState.Fetch.instr1 = state.Fetch.instr2;
				newState.Fetch.pcinstr1 = state.Fetch.pcinstr2;
				newState.Fetch.instr2 = NOINSTRUCTION;
				newState.Fetch.pcinstr2 = EMPTY;

				/*WORKING CODE
				 if (state.Fetch.instr1 != NOINSTRUCTION
				 && state.Fetch.instr2 != NOINSTRUCTION) {
				 newState.pc = state.Fetch.pcinstr2;
				 newState.Fetch.instr1 = NOINSTRUCTION;
				 newState.Fetch.pcinstr1 = EMPTY;
				 newState.Fetch.instr2 = NOINSTRUCTION;
				 newState.Fetch.pcinstr2 = EMPTY;
				 }*/

			}
		} else {
			/*no space instructions don't move ahead of fetch stage */

			newState.pc = newState.Fetch.pcinstr1;
			newState.Fetch.instr1 = state.Fetch.instr1;
			newState.Fetch.pcinstr1 = state.Fetch.pcinstr1;
			newState.Fetch.instr2 = state.Fetch.instr2;
			newState.Fetch.pcinstr2 = state.Fetch.pcinstr2;

			/*WORKING CODE
			 if (state.Fetch.instr1 != NOINSTRUCTION
			 && state.Fetch.instr2 != NOINSTRUCTION) {
			 newState.pc = state.Fetch.pcinstr1;
			 newState.Fetch.instr1 = NOINSTRUCTION;
			 newState.Fetch.pcinstr1 = EMPTY;
			 newState.Fetch.instr2 = NOINSTRUCTION;
			 newState.Fetch.pcinstr2 = EMPTY;
			 }*/
		}

		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		/* --------------------- SC stage --------------------- */
		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */

		/*Integer Unit*/
		pos = -1;
		insCycle = INT_MAX;
		brflag = FALSE;
		for (i = 0; i < 3; i++) {
			if (state.RS.IntegerRS[i].busy == TRUE
					&& state.RS.IntegerRS[i].validSrc1 == TRUE
					&& state.RS.IntegerRS[i].validSrc2 == TRUE
					&& state.RS.IntegerRS[i].scheduled == FALSE
					&& state.RS.IntegerRS[i].cycleID < insCycle) {
				if (brflag == TRUE) {
					if (state.RS.IntegerRS[i].operation == BEQ) {
						pos = i;
						insCycle = state.RS.IntegerRS[i].cycleID;
					}
				} else {
					pos = i;
					insCycle = state.RS.IntegerRS[i].cycleID;
					if (state.RS.IntegerRS[i].operation == BEQ) {
						brflag = TRUE;
					}
				}
			}
		}

		if (pos != -1) {
			switch (state.RS.IntegerRS[pos].operation) {
			case ADD:
			case NAND:
				newState.intUnit.stage[0].dest =
						state.RS.IntegerRS[pos].physicalDest;
				newState.intUnit.stage[0].operation =
						state.RS.IntegerRS[pos].operation;
				newState.intUnit.stage[0].readSrc1 =
						state.RS.IntegerRS[pos].readSrc1;
				newState.intUnit.stage[0].readSrc2 =
						state.RS.IntegerRS[pos].readSrc2;
				newState.RS.IntegerRS[pos].scheduled = TRUE;
				break;
			case BEQ:
				newState.branches++;
				newState.intUnit.stage[0].dest =
						state.RS.IntegerRS[pos].physicalDest;
				newState.intUnit.stage[0].operation =
						state.RS.IntegerRS[pos].operation;
				newState.intUnit.stage[0].readSrc1 =
						state.RS.IntegerRS[pos].readSrc1;
				newState.intUnit.stage[0].readSrc2 =
						state.RS.IntegerRS[pos].readSrc2;
				newState.RS.IntegerRS[pos].scheduled = TRUE;
				break;
			}
		} else {
			/*No instructions present or ready to RUN*/
			newState.intUnit.stage[0].dest = EMPTY;
			newState.intUnit.stage[0].operation = NOINSTRUCTION;
			newState.intUnit.stage[0].readSrc1 = EMPTY;
			newState.intUnit.stage[0].readSrc2 = EMPTY;
		}

		/*Multiply Unit*/
		pos = -1;
		insCycle = INT_MAX;
		for (i = 0; i < 3; i++) {
			if (state.RS.MultiplyRS[i].busy == TRUE
					&& state.RS.MultiplyRS[i].validSrc1 == TRUE
					&& state.RS.MultiplyRS[i].validSrc2 == TRUE
					&& state.RS.MultiplyRS[i].cycleID
							< insCycle&& state.RS.MultiplyRS[i].scheduled == FALSE) {
				pos = i;
				insCycle = state.RS.MultiplyRS[i].cycleID;
			}
		}

		if (state.multUnit.stage[0].operation == NOINSTRUCTION
		//&& state.multUnit.stage[1].operation == NOINSTRUCTION
				&& pos != -1) {
			newState.multUnit.stage[0].operation =
					state.RS.MultiplyRS[pos].operation;
			newState.multUnit.stage[0].dest =
					state.RS.MultiplyRS[pos].physicalDest;
			newState.multUnit.stage[0].readSrc1 =
					state.RS.MultiplyRS[pos].readSrc1;
			newState.multUnit.stage[0].readSrc2 =
					state.RS.MultiplyRS[pos].readSrc2;
			newState.RS.MultiplyRS[pos].scheduled = TRUE;
		} else {
			/*Instructions NOT ready/ Unable to Pipeline
			 * Need to add NOINSTRUCTIONS to the pipeline*/
			newState.multUnit.stage[0].operation = NOINSTRUCTION;
			newState.multUnit.stage[0].dest = EMPTY;
			newState.multUnit.stage[0].readSrc1 = EMPTY;
			newState.multUnit.stage[0].readSrc2 = EMPTY;
		}

		/*Load Unit*/
		pos = -1;
		insCycle = INT_MAX;
		for (i = 0; i < 3; i++) {
			if (state.RS.LoadRS[i].busy == TRUE
					&& state.RS.LoadRS[i].validSrc1 == TRUE
					&& state.RS.LoadRS[i].validSrc2 == TRUE
					&& state.RS.LoadRS[i].cycleID
							< insCycle&& state.RS.LoadRS[i].scheduled == FALSE) {
				pos = i;
				insCycle = state.RS.LoadRS[i].cycleID;
			}
		}

		if (state.loadUnit.stage[0].operation == NOINSTRUCTION
				&& state.loadUnit.stage[1].operation == NOINSTRUCTION
				&& state.loadUnit.stage[2].operation == NOINSTRUCTION
				&& pos != -1) {

			newState.loadUnit.stage[0].operation =
					state.RS.LoadRS[pos].operation;
			newState.loadUnit.stage[0].dest = state.RS.LoadRS[pos].physicalDest;
			newState.loadUnit.stage[0].readSrc1 = state.RS.LoadRS[pos].readSrc1;
			newState.loadUnit.stage[0].readSrc2 = state.RS.LoadRS[pos].readSrc2;
			newState.loadUnit.stage[0].offset = state.RS.LoadRS[pos].offset;
			newState.RS.LoadRS[pos].scheduled = TRUE;
		} else {
			/*Instructions NOT ready/ Unable to Pipeline
			 * Need to add NOINSTRUCTIONS to the pipeline*/
			newState.loadUnit.stage[0].operation = NOINSTRUCTION;
			newState.loadUnit.stage[0].dest = EMPTY;
			newState.loadUnit.stage[0].readSrc1 = EMPTY;
			newState.loadUnit.stage[0].readSrc2 = EMPTY;
			newState.loadUnit.stage[0].offset = EMPTY;
		}

		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		/* --------------------- EX stage --------------------- */
		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */

		/*Integer Unit*/
		if (state.intUnit.stage[0].operation != -1) {
			switch (state.intUnit.stage[0].operation) {
			case ADD:
				/*Entry into ROB*/
				value = state.intUnit.stage[0].readSrc1
						+ state.intUnit.stage[0].readSrc2;
				newState.ROB.ROBENTRY[state.intUnit.stage[0].dest].destValue =
						value;
				newState.ROB.ROBENTRY[state.intUnit.stage[0].dest].validValue =
				CALCULATED;
				/*Entry into RS*/
				for (i = 0; i < 3; i++) {
					if (newState.RS.IntegerRS[i].busy == TRUE) {
						if (newState.RS.IntegerRS[i].physicalSrc1
								== state.intUnit.stage[0].dest&& newState.RS.IntegerRS[i].validSrc1 == FALSE) {
							newState.RS.IntegerRS[i].readSrc1 = value;
							newState.RS.IntegerRS[i].validSrc1 = TRUE;
						}

						if (newState.RS.IntegerRS[i].physicalSrc2
								== state.intUnit.stage[0].dest&& newState.RS.IntegerRS[i].validSrc2 == FALSE) {
							newState.RS.IntegerRS[i].readSrc2 = value;
							newState.RS.IntegerRS[i].validSrc2 = TRUE;
						}
					}

					if (newState.RS.MultiplyRS[i].busy == TRUE) {
						if (newState.RS.MultiplyRS[i].physicalSrc1
								== state.intUnit.stage[0].dest&& newState.RS.MultiplyRS[i].validSrc1 == FALSE) {
							newState.RS.MultiplyRS[i].readSrc1 = value;
							newState.RS.MultiplyRS[i].validSrc1 = TRUE;
						}

						if (newState.RS.MultiplyRS[i].physicalSrc2
								== state.intUnit.stage[0].dest&& newState.RS.MultiplyRS[i].validSrc2 == FALSE) {
							newState.RS.MultiplyRS[i].readSrc2 = value;
							newState.RS.MultiplyRS[i].validSrc2 = TRUE;
						}
					}

					if (newState.RS.LoadRS[i].busy == TRUE) {
						if (newState.RS.LoadRS[i].physicalSrc1
								== state.intUnit.stage[0].dest&& newState.RS.LoadRS[i].validSrc1 == FALSE) {
							newState.RS.LoadRS[i].readSrc1 = value;
							newState.RS.LoadRS[i].validSrc1 = TRUE;
						}

						if (newState.RS.LoadRS[i].physicalSrc2
								== state.intUnit.stage[0].dest&& newState.RS.LoadRS[i].validSrc2 == FALSE) {
							newState.RS.LoadRS[i].readSrc2 = value;
							newState.RS.LoadRS[i].validSrc2 = TRUE;
						}
					}
				}

				/*Delete entry from RS*/
				pos = -1;
				insCycle = INT_MAX;
				for (i = 0; i < 3; i++) {
					if (newState.RS.IntegerRS[i].operation
							== state.intUnit.stage[0].operation&& newState.RS.IntegerRS[i].scheduled == TRUE) {
						if (newState.RS.IntegerRS[i].cycleID < insCycle) {
							pos = i;
							insCycle = newState.RS.IntegerRS[i].cycleID;
						}
					}
				}

				if (pos != -1) {
					newState.RS.IntegerRS[pos].busy = FALSE;
					newState.RS.IntegerRS[pos].scheduled = FALSE;
					newState.RS.IntegerRS[pos].cycleID = 0;
					newState.RS.IntegerRS[pos].operation = 0;
					newState.RS.IntegerRS[pos].physicalDest = 0;
					newState.RS.IntegerRS[pos].physicalSrc1 = 0;
					newState.RS.IntegerRS[pos].physicalSrc2 = 0;
					newState.RS.IntegerRS[pos].readSrc1 = 0;
					newState.RS.IntegerRS[pos].readSrc2 = 0;
					newState.RS.IntegerRS[pos].validSrc1 = FALSE;
					newState.RS.IntegerRS[pos].validSrc2 = FALSE;
				}
				break;

			case NAND:
				/*Entry into ROB*/
				value = ~(state.intUnit.stage[0].readSrc1
						& state.intUnit.stage[0].readSrc2);
				newState.ROB.ROBENTRY[state.intUnit.stage[0].dest].destValue =
						value;
				newState.ROB.ROBENTRY[state.intUnit.stage[0].dest].validValue =
				CALCULATED;

				/*Entry into RS*/
				for (i = 0; i < 3; i++) {
					if (newState.RS.IntegerRS[i].busy == TRUE) {
						if (newState.RS.IntegerRS[i].physicalSrc1
								== state.intUnit.stage[0].dest&& newState.RS.IntegerRS[i].validSrc1 == FALSE) {
							newState.RS.IntegerRS[i].readSrc1 = value;
							newState.RS.IntegerRS[i].validSrc1 = TRUE;
						}

						if (newState.RS.IntegerRS[i].physicalSrc2
								== state.intUnit.stage[0].dest&& newState.RS.IntegerRS[i].validSrc2 == FALSE) {
							newState.RS.IntegerRS[i].readSrc2 = value;
							newState.RS.IntegerRS[i].validSrc2 = TRUE;
						}
					}

					if (newState.RS.MultiplyRS[i].busy == TRUE) {
						if (newState.RS.MultiplyRS[i].physicalSrc1
								== state.intUnit.stage[0].dest&& newState.RS.MultiplyRS[i].validSrc1 == FALSE) {
							newState.RS.MultiplyRS[i].readSrc1 = value;
							newState.RS.MultiplyRS[i].validSrc1 = TRUE;
						}

						if (newState.RS.MultiplyRS[i].physicalSrc2
								== state.intUnit.stage[0].dest&& newState.RS.MultiplyRS[i].validSrc2 == FALSE) {
							newState.RS.MultiplyRS[i].readSrc2 = value;
							newState.RS.MultiplyRS[i].validSrc2 = TRUE;
						}
					}

					if (newState.RS.LoadRS[i].busy == TRUE) {
						if (newState.RS.LoadRS[i].physicalSrc1
								== state.intUnit.stage[0].dest&& newState.RS.LoadRS[i].validSrc1 == FALSE) {
							newState.RS.LoadRS[i].readSrc1 = value;
							newState.RS.LoadRS[i].validSrc1 = TRUE;
						}

						if (newState.RS.LoadRS[i].physicalSrc2
								== state.intUnit.stage[0].dest&& newState.RS.LoadRS[i].validSrc2 == FALSE) {
							newState.RS.LoadRS[i].readSrc2 = value;
							newState.RS.LoadRS[i].validSrc2 = TRUE;
						}
					}
				}

				/*Delete entry from RS*/
				pos = -1;
				insCycle = INT_MAX;
				for (i = 0; i < 3; i++) {
					if (newState.RS.IntegerRS[i].operation
							== state.intUnit.stage[0].operation&& newState.RS.IntegerRS[i].scheduled == TRUE) {
						if (newState.RS.IntegerRS[i].cycleID < insCycle) {
							pos = i;
							insCycle = newState.RS.IntegerRS[i].cycleID;
						}
					}
				}

				if (pos != -1) {
					newState.RS.IntegerRS[pos].busy = FALSE;
					newState.RS.IntegerRS[pos].scheduled = FALSE;
					newState.RS.IntegerRS[pos].cycleID = 0;
					newState.RS.IntegerRS[pos].operation = 0;
					newState.RS.IntegerRS[pos].physicalDest = 0;
					newState.RS.IntegerRS[pos].physicalSrc1 = 0;
					newState.RS.IntegerRS[pos].physicalSrc2 = 0;
					newState.RS.IntegerRS[pos].readSrc1 = 0;
					newState.RS.IntegerRS[pos].readSrc2 = 0;
					newState.RS.IntegerRS[pos].validSrc1 = FALSE;
					newState.RS.IntegerRS[pos].validSrc2 = FALSE;
				}
				break;

			case BEQ:
				/*Entry into ROB*/
				value = state.intUnit.stage[0].readSrc1
						^ state.intUnit.stage[0].readSrc2;
				newState.ROB.ROBENTRY[state.intUnit.stage[0].dest].destValue =
						value;
				newState.ROB.ROBENTRY[state.intUnit.stage[0].dest].validValue =
				CALCULATED;

				/*Delete entry from RS*/
				pos = -1;
				insCycle = INT_MAX;
				for (i = 0; i < 3; i++) {
					if (newState.RS.IntegerRS[i].operation
							== state.intUnit.stage[0].operation&& newState.RS.IntegerRS[i].scheduled == TRUE) {
						if (newState.RS.IntegerRS[i].cycleID < insCycle) {
							pos = i;
							insCycle = newState.RS.IntegerRS[i].cycleID;
						}
					}
				}

				/*BEQ do not cause hazards!*/
				if (pos != -1) {
					newState.RS.IntegerRS[pos].busy = FALSE;
					newState.RS.IntegerRS[pos].scheduled = FALSE;
					newState.RS.IntegerRS[pos].cycleID = 0;
					newState.RS.IntegerRS[pos].operation = 0;
					newState.RS.IntegerRS[pos].physicalDest = 0;
					newState.RS.IntegerRS[pos].physicalSrc1 = 0;
					newState.RS.IntegerRS[pos].physicalSrc2 = 0;
					newState.RS.IntegerRS[pos].readSrc1 = 0;
					newState.RS.IntegerRS[pos].readSrc2 = 0;
					newState.RS.IntegerRS[pos].validSrc1 = FALSE;
					newState.RS.IntegerRS[pos].validSrc2 = FALSE;
				}
				break;
			} //switch ends
		} else {
			/*Integer Unit is empty*/
		}

		/*Move the first five instructions ahead and execute the sixth multiplication*/
		newState.multUnit.stage[1] = state.multUnit.stage[0];
		newState.multUnit.stage[2] = state.multUnit.stage[1];
		newState.multUnit.stage[3] = state.multUnit.stage[2];
		newState.multUnit.stage[4] = state.multUnit.stage[3];
		newState.multUnit.stage[5] = state.multUnit.stage[4];

		if (state.multUnit.stage[5].operation != -1) {
			/*Entry into ROB*/
			value = state.multUnit.stage[5].readSrc1
					* state.multUnit.stage[5].readSrc2;
			newState.ROB.ROBENTRY[state.multUnit.stage[5].dest].destValue =
					value;
			newState.ROB.ROBENTRY[state.multUnit.stage[5].dest].validValue =
			CALCULATED;

			/*Entry into RS*/
			for (i = 0; i < 3; i++) {
				if (newState.RS.IntegerRS[i].busy == TRUE) {
					if (newState.RS.IntegerRS[i].physicalSrc1
							== state.multUnit.stage[5].dest&& newState.RS.IntegerRS[i].validSrc1 == FALSE) {
						newState.RS.IntegerRS[i].readSrc1 = value;
						newState.RS.IntegerRS[i].validSrc1 = TRUE;
					}

					if (newState.RS.IntegerRS[i].physicalSrc2
							== state.multUnit.stage[5].dest&& newState.RS.IntegerRS[i].validSrc2 == FALSE) {
						newState.RS.IntegerRS[i].readSrc2 = value;
						newState.RS.IntegerRS[i].validSrc2 = TRUE;
					}
				}

				if (newState.RS.MultiplyRS[i].busy == TRUE) {
					if (newState.RS.MultiplyRS[i].physicalSrc1
							== state.multUnit.stage[5].dest&& newState.RS.MultiplyRS[i].validSrc1 == FALSE) {
						newState.RS.MultiplyRS[i].readSrc1 = value;
						newState.RS.MultiplyRS[i].validSrc1 = TRUE;
					}

					if (newState.RS.MultiplyRS[i].physicalSrc2
							== state.multUnit.stage[5].dest&& newState.RS.MultiplyRS[i].validSrc2 == FALSE) {
						newState.RS.MultiplyRS[i].readSrc2 = value;
						newState.RS.MultiplyRS[i].validSrc2 = TRUE;
					}
				}

				if (newState.RS.LoadRS[i].busy == TRUE) {
					if (newState.RS.LoadRS[i].physicalSrc1
							== state.multUnit.stage[5].dest&& newState.RS.LoadRS[i].validSrc1 == FALSE) {
						newState.RS.LoadRS[i].readSrc1 = value;
						newState.RS.LoadRS[i].validSrc1 = TRUE;
					}

					if (newState.RS.LoadRS[i].physicalSrc2
							== state.multUnit.stage[5].dest&& newState.RS.LoadRS[i].validSrc2 == FALSE) {
						newState.RS.LoadRS[i].readSrc2 = value;
						newState.RS.LoadRS[i].validSrc2 = TRUE;
					}
				}
			}
			/*Delete entry from RS*/
			pos = -1;
			insCycle = INT_MAX;
			for (i = 0; i < 3; i++) {
				if (newState.RS.MultiplyRS[i].operation
						== state.multUnit.stage[5].operation) {
					if (newState.RS.MultiplyRS[i].cycleID < insCycle) {
						pos = i;
						insCycle = newState.RS.MultiplyRS[i].cycleID;
					}
				}
			}

			if (pos != -1) {
				newState.RS.MultiplyRS[pos].busy = FALSE;
				newState.RS.MultiplyRS[pos].scheduled = FALSE;
				newState.RS.MultiplyRS[pos].cycleID = 0;
				newState.RS.MultiplyRS[pos].operation = 0;
				newState.RS.MultiplyRS[pos].physicalDest = 0;
				newState.RS.MultiplyRS[pos].physicalSrc1 = 0;
				newState.RS.MultiplyRS[pos].physicalSrc2 = 0;
				newState.RS.MultiplyRS[pos].readSrc1 = 0;
				newState.RS.MultiplyRS[pos].readSrc2 = 0;
				newState.RS.MultiplyRS[pos].validSrc1 = FALSE;
				newState.RS.MultiplyRS[pos].validSrc2 = FALSE;
			}
		} else {
			/*Multiply unit is empty*/
		}

		newState.loadUnit.stage[1] = state.loadUnit.stage[0];
		newState.loadUnit.stage[2] = state.loadUnit.stage[1];

		if (state.loadUnit.stage[2].operation != -1) {
			switch (state.loadUnit.stage[2].operation) {
			case LW:
				/*Entry into ROB*/
				value = state.dataMem[state.loadUnit.stage[2].readSrc1
						+ state.loadUnit.stage[2].offset];
				newState.ROB.ROBENTRY[state.loadUnit.stage[2].dest].destValue =
						value;
				newState.ROB.ROBENTRY[state.loadUnit.stage[2].dest].validValue =
				CALCULATED;

				/*Entry into RS*/
				for (i = 0; i < 3; i++) {
					if (newState.RS.IntegerRS[i].busy == TRUE) {
						if (newState.RS.IntegerRS[i].physicalSrc1
								== state.loadUnit.stage[2].dest&& newState.RS.IntegerRS[i].validSrc1 == FALSE) {
							newState.RS.IntegerRS[i].readSrc1 = value;
							newState.RS.IntegerRS[i].validSrc1 = TRUE;
						}

						if (newState.RS.IntegerRS[i].physicalSrc2
								== state.loadUnit.stage[2].dest&& newState.RS.IntegerRS[i].validSrc2 == FALSE) {
							newState.RS.IntegerRS[i].readSrc2 = value;
							newState.RS.IntegerRS[i].validSrc2 = TRUE;
						}
					}

					if (newState.RS.MultiplyRS[i].busy == TRUE) {
						if (newState.RS.MultiplyRS[i].physicalSrc1
								== state.loadUnit.stage[2].dest&& newState.RS.MultiplyRS[i].validSrc1 == FALSE) {
							newState.RS.MultiplyRS[i].readSrc1 = value;
							newState.RS.MultiplyRS[i].validSrc1 = TRUE;
						}

						if (newState.RS.MultiplyRS[i].physicalSrc2
								== state.loadUnit.stage[2].dest&& newState.RS.MultiplyRS[i].validSrc2 == FALSE) {
							newState.RS.MultiplyRS[i].readSrc2 = value;
							newState.RS.MultiplyRS[i].validSrc2 = TRUE;
						}
					}

					if (newState.RS.LoadRS[i].busy == TRUE) {
						if (newState.RS.LoadRS[i].physicalSrc1
								== state.loadUnit.stage[2].dest&& newState.RS.LoadRS[i].validSrc1 == FALSE) {
							newState.RS.LoadRS[i].readSrc1 = value;
							newState.RS.LoadRS[i].validSrc1 = TRUE;
						}

						if (newState.RS.LoadRS[i].physicalSrc2
								== state.loadUnit.stage[2].dest&& newState.RS.LoadRS[i].validSrc2 == FALSE) {
							newState.RS.LoadRS[i].readSrc2 = value;
							newState.RS.LoadRS[i].validSrc2 = TRUE;
						}
					}
				}
				/*Delete entry from RS*/
				pos = -1;
				insCycle = INT_MAX;
				for (i = 0; i < 3; i++) {
					if (newState.RS.LoadRS[i].operation
							== state.loadUnit.stage[2].operation) {
						if (newState.RS.LoadRS[i].cycleID < insCycle) {
							pos = i;
							insCycle = newState.RS.LoadRS[i].cycleID;
						}
					}
				}

				if (pos != -1) {
					newState.RS.LoadRS[pos].busy = FALSE;
					newState.RS.LoadRS[pos].scheduled = FALSE;
					newState.RS.LoadRS[pos].cycleID = 0;
					newState.RS.LoadRS[pos].operation = 0;
					newState.RS.LoadRS[pos].physicalDest = 0;
					newState.RS.LoadRS[pos].physicalSrc1 = 0;
					newState.RS.LoadRS[pos].physicalSrc2 = 0;
					newState.RS.LoadRS[pos].readSrc1 = 0;
					newState.RS.LoadRS[pos].readSrc2 = 0;
					newState.RS.LoadRS[pos].validSrc1 = FALSE;
					newState.RS.LoadRS[pos].validSrc2 = FALSE;
				}
				break;

			case SW:
				value = state.loadUnit.stage[2].readSrc2;
				newState.ROB.ROBENTRY[state.loadUnit.stage[2].dest].destValue =
						value;
				newState.ROB.ROBENTRY[state.loadUnit.stage[2].dest].validValue =
				CALCULATED;
				newState.ROB.ROBENTRY[state.loadUnit.stage[2].dest].offset =
						state.loadUnit.stage[2].readSrc1
								+ state.loadUnit.stage[2].offset;

				/*Delete entry from RS*/
				pos = -1;
				insCycle = INT_MAX;
				for (i = 0; i < 3; i++) {
					if (newState.RS.LoadRS[i].operation
							== state.loadUnit.stage[2].operation) {
						if (newState.RS.LoadRS[i].cycleID < insCycle) {
							pos = i;
							insCycle = newState.RS.LoadRS[i].cycleID;
						}
					}
				}

				/*SW do not cause hazards!*/

				if (pos != -1) {
					newState.RS.LoadRS[pos].busy = FALSE;
					newState.RS.LoadRS[pos].scheduled = FALSE;
					newState.RS.LoadRS[pos].cycleID = 0;
					newState.RS.LoadRS[pos].operation = 0;
					newState.RS.LoadRS[pos].physicalDest = 0;
					newState.RS.LoadRS[pos].physicalSrc1 = 0;
					newState.RS.LoadRS[pos].physicalSrc2 = 0;
					newState.RS.LoadRS[pos].readSrc1 = 0;
					newState.RS.LoadRS[pos].readSrc2 = 0;
					newState.RS.LoadRS[pos].validSrc1 = FALSE;
					newState.RS.LoadRS[pos].validSrc2 = FALSE;
				}
				break;
			}
		} else {
			/*Load unit is empty*/
		}

		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		/* --------------------- CT stage --------------------- */
		/* ---------------------------------------------------- */
		/* ---------------------------------------------------- */
		if (state.ROB.ROBENTRY[newState.ROB.head].validValue == CALCULATED) {
			newState.retired++;
			switch (state.ROB.ROBENTRY[newState.ROB.head].operation) {
			case ADD:
			case NAND:
			case MULT:
			case LW:

				if (state.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][0]
						== newState.ROB.head) {
					newState.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][0] =
							0;
					newState.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][1] =
					INVALID;
					newState.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][2] =
					EMPTY;
					newState.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][3] =
					COMMITTED;
					newState.reg[state.ROB.ROBENTRY[newState.ROB.head].destReg] =
							state.ROB.ROBENTRY[newState.ROB.head].destValue;
				}

				if (state.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][0]
						!= newState.ROB.head) {
					/*Storing values to avoid branch hazards*/
					newState.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][2] =
							state.ROB.ROBENTRY[newState.ROB.head].destValue;
					newState.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][3] =
					NOTCOMMITTED;
				}

				newState.ROB.ROBENTRY[newState.ROB.head].validValue =
				NOTCALCULATED;
				newState.ROB.ROBENTRY[newState.ROB.head].destReg = 0;
				newState.ROB.ROBENTRY[newState.ROB.head].destValue = 0;
				newState.ROB.ROBENTRY[newState.ROB.head].operation =
				NOINSTRUCTION;
				newState.ROB.ROBENTRY[newState.ROB.head].offset = 0;
				newState.ROB.ROBENTRY[newState.ROB.head].instruction =
				EMPTY;
				incHead();
				break;

			case SW:

				newState.dataMem[state.ROB.ROBENTRY[newState.ROB.head].offset] =
						state.ROB.ROBENTRY[newState.ROB.head].destValue;
				newState.ROB.ROBENTRY[newState.ROB.head].validValue =
				NOTCALCULATED;
				newState.ROB.ROBENTRY[newState.ROB.head].destReg = 0;
				newState.ROB.ROBENTRY[newState.ROB.head].destValue = 0;
				newState.ROB.ROBENTRY[newState.ROB.head].operation =
				NOINSTRUCTION;
				newState.ROB.ROBENTRY[newState.ROB.head].offset = 0;
				newState.ROB.ROBENTRY[newState.ROB.head].instruction =
				EMPTY;
				incHead();
				break;

			case NOOP:
				/*DO NOTHING*/
				newState.ROB.ROBENTRY[newState.ROB.head].validValue =
				NOTCALCULATED;
				newState.ROB.ROBENTRY[newState.ROB.head].destReg = 0;
				newState.ROB.ROBENTRY[newState.ROB.head].destValue = 0;
				newState.ROB.ROBENTRY[newState.ROB.head].operation =
				NOINSTRUCTION;
				newState.ROB.ROBENTRY[newState.ROB.head].offset = 0;
				newState.ROB.ROBENTRY[newState.ROB.head].instruction =
				EMPTY;

				incHead();
				break;

			case HALT:
				/*Return to MAIN*/
				for (i = 0; i < NUMROB; i++) {
					newState.ROB.ROBENTRY[i].destReg = 0;
					newState.ROB.ROBENTRY[i].destValue = 0;
					newState.ROB.ROBENTRY[i].offset = 0;
					newState.ROB.ROBENTRY[i].operation = EMPTY;
					newState.ROB.ROBENTRY[i].validValue =
					NOTCALCULATED;
					newState.ROB.ROBENTRY[i].instruction =
					EMPTY;
				}

				newState.ROB.full = FALSE;
				newState.ROB.head = 0;
				newState.ROB.tail = 0;
				for (i = 0; i < 3; i++) {
					newState.RS.IntegerRS[i].busy = FALSE;
					newState.RS.IntegerRS[i].cycleID = 0;
					newState.RS.IntegerRS[i].scheduled = FALSE;
					newState.RS.IntegerRS[i].operation = 0;
					newState.RS.IntegerRS[i].physicalDest = 0;
					newState.RS.IntegerRS[i].physicalSrc1 = 0;
					newState.RS.IntegerRS[i].physicalSrc2 = 0;
					newState.RS.IntegerRS[i].readSrc1 = 0;
					newState.RS.IntegerRS[i].readSrc2 = 0;
					newState.RS.IntegerRS[i].validSrc1 = FALSE;
					newState.RS.IntegerRS[i].validSrc2 = FALSE;

					newState.RS.MultiplyRS[i].busy = FALSE;
					newState.RS.MultiplyRS[i].cycleID = 0;
					newState.RS.MultiplyRS[i].operation = 0;
					newState.RS.MultiplyRS[i].physicalDest = 0;
					newState.RS.MultiplyRS[i].physicalSrc1 = 0;
					newState.RS.MultiplyRS[i].physicalSrc2 = 0;
					newState.RS.MultiplyRS[i].readSrc1 = 0;
					newState.RS.MultiplyRS[i].readSrc2 = 0;
					newState.RS.MultiplyRS[i].validSrc1 = FALSE;
					newState.RS.MultiplyRS[i].validSrc2 = FALSE;

					newState.RS.LoadRS[i].busy = FALSE;
					newState.RS.LoadRS[i].cycleID = 0;
					newState.RS.LoadRS[i].operation = 0;
					newState.RS.LoadRS[i].physicalDest = 0;
					newState.RS.LoadRS[i].physicalSrc1 = 0;
					newState.RS.LoadRS[i].physicalSrc2 = 0;
					newState.RS.LoadRS[i].readSrc1 = 0;
					newState.RS.LoadRS[i].readSrc2 = 0;
					newState.RS.LoadRS[i].validSrc1 = FALSE;
					newState.RS.LoadRS[i].validSrc2 = FALSE;
				}

				for (i = 0; i < NUMREGS; i++) {
					newState.RT.renameTable[i][0] = 0;
					newState.RT.renameTable[i][1] = INVALID;
					newState.RT.renameTable[i][2] = EMPTY;
					newState.RT.renameTable[i][3] = COMMITTED;
				}

				for (i = 0; i < 1; i++) {
					newState.intUnit.stage[i].operation = EMPTY;
					newState.intUnit.stage[i].readSrc1 = 0;
					newState.intUnit.stage[i].readSrc2 = 0;
				}

				for (i = 0; i < 6; i++) {
					newState.multUnit.stage[i].operation =
					EMPTY;
					newState.multUnit.stage[i].readSrc1 = 0;
					newState.multUnit.stage[i].readSrc2 = 0;
				}

				for (i = 0; i < 3; i++) {
					newState.loadUnit.stage[i].operation =
					EMPTY;
				}
				/*IMPORTANT! Delete both the tables and not one!*/
				state = newState;

				incHead();
				return;
				break;

			case BEQ:

				if (state.ROB.ROBENTRY[newState.ROB.head].destValue == 0) {
					/*branching TRUE*/
					predictedPath = modifyBTB(
							state.ROB.ROBENTRY[newState.ROB.head].pc,
							state.ROB.ROBENTRY[newState.ROB.head].offset,
							BRANCH_TRUE);
					if (predictedPath == 1) {
						/*Branching was not predicted at IFID stage
						 * Need to flush out all the instructions
						 * after the branch*/
						newState.pc = state.ROB.ROBENTRY[newState.ROB.head].pc
								+ 1
								+ state.ROB.ROBENTRY[newState.ROB.head].offset;
						newState.Fetch.instr1 = NOINSTRUCTION;
						newState.Fetch.instr2 = NOINSTRUCTION;

						/*IMPORTANT! Return non-committed register values*/
						for (i = 0; i < NUMREGS; i++) {
							if (newState.RT.renameTable[i][1] == VALID
									&& newState.RT.renameTable[i][3]
											== NOTCOMMITTED) {
								newState.reg[i] = newState.RT.renameTable[i][2];
							}
						}

						for (i = 0; i < NUMROB; i++) {
							newState.ROB.ROBENTRY[i].destReg = 0;
							newState.ROB.ROBENTRY[i].destValue = 0;
							newState.ROB.ROBENTRY[i].offset = 0;
							newState.ROB.ROBENTRY[i].operation = EMPTY;
							newState.ROB.ROBENTRY[i].validValue =
							NOTCALCULATED;
							newState.ROB.ROBENTRY[i].instruction =
							EMPTY;
						}

						newState.ROB.full = FALSE;
						newState.ROB.head = 0;
						newState.ROB.tail = 0;
						for (i = 0; i < 3; i++) {
							newState.RS.IntegerRS[i].busy = FALSE;
							newState.RS.IntegerRS[i].cycleID = 0;
							newState.RS.IntegerRS[i].scheduled = FALSE;
							newState.RS.IntegerRS[i].operation = 0;
							newState.RS.IntegerRS[i].physicalDest = 0;
							newState.RS.IntegerRS[i].physicalSrc1 = 0;
							newState.RS.IntegerRS[i].physicalSrc2 = 0;
							newState.RS.IntegerRS[i].readSrc1 = 0;
							newState.RS.IntegerRS[i].readSrc2 = 0;
							newState.RS.IntegerRS[i].validSrc1 = FALSE;
							newState.RS.IntegerRS[i].validSrc2 = FALSE;

							newState.RS.MultiplyRS[i].busy = FALSE;
							newState.RS.MultiplyRS[i].cycleID = 0;
							newState.RS.MultiplyRS[i].operation = 0;
							newState.RS.MultiplyRS[i].physicalDest = 0;
							newState.RS.MultiplyRS[i].physicalSrc1 = 0;
							newState.RS.MultiplyRS[i].physicalSrc2 = 0;
							newState.RS.MultiplyRS[i].readSrc1 = 0;
							newState.RS.MultiplyRS[i].readSrc2 = 0;
							newState.RS.MultiplyRS[i].validSrc1 = FALSE;
							newState.RS.MultiplyRS[i].validSrc2 = FALSE;

							newState.RS.LoadRS[i].busy = FALSE;
							newState.RS.LoadRS[i].cycleID = 0;
							newState.RS.LoadRS[i].operation = 0;
							newState.RS.LoadRS[i].physicalDest = 0;
							newState.RS.LoadRS[i].physicalSrc1 = 0;
							newState.RS.LoadRS[i].physicalSrc2 = 0;
							newState.RS.LoadRS[i].readSrc1 = 0;
							newState.RS.LoadRS[i].readSrc2 = 0;
							newState.RS.LoadRS[i].validSrc1 = FALSE;
							newState.RS.LoadRS[i].validSrc2 = FALSE;
						}

						for (i = 0; i < NUMREGS; i++) {
							newState.RT.renameTable[i][0] = 0;
							newState.RT.renameTable[i][1] = INVALID;
							newState.RT.renameTable[i][2] = EMPTY;
							newState.RT.renameTable[i][3] = COMMITTED;
						}

						for (i = 0; i < 1; i++) {
							newState.intUnit.stage[i].operation = EMPTY;
							newState.intUnit.stage[i].readSrc1 = 0;
							newState.intUnit.stage[i].readSrc2 = 0;
						}

						for (i = 0; i < 6; i++) {
							newState.multUnit.stage[i].operation =
							EMPTY;
							newState.multUnit.stage[i].readSrc1 = 0;
							newState.multUnit.stage[i].readSrc2 = 0;
						}

						for (i = 0; i < 3; i++) {
							newState.loadUnit.stage[i].operation =
							EMPTY;
						}
						/*IMPORTANT! Delete both the tables and not one!*/
						state = newState;
					} else {
						/*Branch was predicted at IFID stage*/
						newState.ROB.ROBENTRY[newState.ROB.head].validValue =
						NOTCALCULATED;
						newState.ROB.ROBENTRY[newState.ROB.head].destReg = 0;
						newState.ROB.ROBENTRY[newState.ROB.head].destValue = 0;
						newState.ROB.ROBENTRY[newState.ROB.head].operation =
						NOINSTRUCTION;
						newState.ROB.ROBENTRY[newState.ROB.head].offset = 0;
						newState.ROB.ROBENTRY[newState.ROB.head].instruction =
						EMPTY;
						incHead();
					}
				} else {
					/*branching FALSE*/
					predictedPath = modifyBTB(
							state.ROB.ROBENTRY[newState.ROB.head].pc,
							state.ROB.ROBENTRY[newState.ROB.head].offset,
							BRANCH_FALSE);
					if (predictedPath == 1) {
						/*Branch was predicted at IFID but not TAKEN!
						 * MISPREDICTION by the branch predictor
						 * Need to flush out all the instructions
						 * after the branch*/
						newState.pc = state.ROB.ROBENTRY[newState.ROB.head].pc
								+ 1;
						newState.Fetch.instr1 = NOINSTRUCTION;
						newState.Fetch.instr2 = NOINSTRUCTION;

						/*IMPORTANT! Return non-committed register values*/
						for (i = 0; i < NUMREGS; i++) {
							if (newState.RT.renameTable[i][1] == VALID
									&& newState.RT.renameTable[i][3]
											== NOTCOMMITTED) {
								newState.reg[i] = newState.RT.renameTable[i][2];
							}
						}

						for (i = 0; i < NUMROB; i++) {
							newState.ROB.ROBENTRY[i].destReg = 0;
							newState.ROB.ROBENTRY[i].destValue = 0;
							newState.ROB.ROBENTRY[i].offset = 0;
							newState.ROB.ROBENTRY[i].operation = EMPTY;
							newState.ROB.ROBENTRY[i].validValue =
							NOTCALCULATED;
							newState.ROB.ROBENTRY[i].instruction =
							EMPTY;
						}

						newState.ROB.full = FALSE;
						newState.ROB.head = 0;
						newState.ROB.tail = 0;

						for (i = 0; i < 3; i++) {
							newState.RS.IntegerRS[i].busy = FALSE;
							newState.RS.IntegerRS[i].cycleID = 0;
							newState.RS.IntegerRS[i].scheduled = FALSE;
							newState.RS.IntegerRS[i].operation = 0;
							newState.RS.IntegerRS[i].physicalDest = 0;
							newState.RS.IntegerRS[i].physicalSrc1 = 0;
							newState.RS.IntegerRS[i].physicalSrc2 = 0;
							newState.RS.IntegerRS[i].readSrc1 = 0;
							newState.RS.IntegerRS[i].readSrc2 = 0;
							newState.RS.IntegerRS[i].validSrc1 = FALSE;
							newState.RS.IntegerRS[i].validSrc2 = FALSE;

							newState.RS.MultiplyRS[i].busy = FALSE;
							newState.RS.MultiplyRS[i].cycleID = 0;
							newState.RS.MultiplyRS[i].operation = 0;
							newState.RS.MultiplyRS[i].physicalDest = 0;
							newState.RS.MultiplyRS[i].physicalSrc1 = 0;
							newState.RS.MultiplyRS[i].physicalSrc2 = 0;
							newState.RS.MultiplyRS[i].readSrc1 = 0;
							newState.RS.MultiplyRS[i].readSrc2 = 0;
							newState.RS.MultiplyRS[i].validSrc1 = FALSE;
							newState.RS.MultiplyRS[i].validSrc2 = FALSE;

							newState.RS.LoadRS[i].busy = FALSE;
							newState.RS.LoadRS[i].cycleID = 0;
							newState.RS.LoadRS[i].operation = 0;
							newState.RS.LoadRS[i].physicalDest = 0;
							newState.RS.LoadRS[i].physicalSrc1 = 0;
							newState.RS.LoadRS[i].physicalSrc2 = 0;
							newState.RS.LoadRS[i].readSrc1 = 0;
							newState.RS.LoadRS[i].readSrc2 = 0;
							newState.RS.LoadRS[i].validSrc1 = FALSE;
							newState.RS.LoadRS[i].validSrc2 = FALSE;
						}

						for (i = 0; i < NUMREGS; i++) {
							newState.RT.renameTable[i][0] = 0;
							newState.RT.renameTable[i][1] = INVALID;
							newState.RT.renameTable[i][2] = EMPTY;
							newState.RT.renameTable[i][3] = COMMITTED;
						}

						for (i = 0; i < 1; i++) {
							newState.intUnit.stage[i].operation = EMPTY;
							newState.intUnit.stage[i].readSrc1 = 0;
							newState.intUnit.stage[i].readSrc2 = 0;
						}

						for (i = 0; i < 6; i++) {
							newState.multUnit.stage[i].operation =
							EMPTY;
							newState.multUnit.stage[i].readSrc1 = 0;
							newState.multUnit.stage[i].readSrc2 = 0;
						}

						for (i = 0; i < 3; i++) {
							newState.loadUnit.stage[i].operation =
							EMPTY;
						}

						/*IMPORTANT! Delete both the tables and not one!*/
						state = newState;
					} else {
						/*Not Predicted/ Correct prediction!*/
						newState.ROB.ROBENTRY[newState.ROB.head].validValue =
						NOTCALCULATED;
						newState.ROB.ROBENTRY[newState.ROB.head].destReg = 0;
						newState.ROB.ROBENTRY[newState.ROB.head].destValue = 0;
						newState.ROB.ROBENTRY[newState.ROB.head].operation =
						NOINSTRUCTION;
						newState.ROB.ROBENTRY[newState.ROB.head].offset = 0;
						newState.ROB.ROBENTRY[newState.ROB.head].instruction =
						EMPTY;
						incHead();
					}
					break;
				}
			}

			if (state.ROB.ROBENTRY[newState.ROB.head].validValue == CALCULATED) {
				newState.retired++;
				switch (state.ROB.ROBENTRY[newState.ROB.head].operation) {
				case ADD:
				case NAND:
				case MULT:
				case LW:

					if (state.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][0]
							== newState.ROB.head) {
						newState.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][0] =
								0;
						newState.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][1] =
						INVALID;
						newState.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][2] =
						EMPTY;
						newState.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][3] =
						COMMITTED;
						newState.reg[state.ROB.ROBENTRY[newState.ROB.head].destReg] =
								state.ROB.ROBENTRY[newState.ROB.head].destValue;
					}

					if (state.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][0]
							!= newState.ROB.head) {
						/*Storing values to avoid branch hazards*/
						newState.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][2] =
								state.ROB.ROBENTRY[newState.ROB.head].destValue;
						newState.RT.renameTable[state.ROB.ROBENTRY[newState.ROB.head].destReg][3] =
						NOTCOMMITTED;
					}

					newState.ROB.ROBENTRY[newState.ROB.head].validValue =
					NOTCALCULATED;
					newState.ROB.ROBENTRY[newState.ROB.head].destReg = 0;
					newState.ROB.ROBENTRY[newState.ROB.head].destValue = 0;
					newState.ROB.ROBENTRY[newState.ROB.head].operation =
					NOINSTRUCTION;
					newState.ROB.ROBENTRY[newState.ROB.head].offset = 0;
					newState.ROB.ROBENTRY[newState.ROB.head].instruction =
					EMPTY;
					incHead();
					break;

				case SW:

					newState.dataMem[newState.ROB.ROBENTRY[newState.ROB.head].offset] =
							newState.ROB.ROBENTRY[newState.ROB.head].destValue;
					newState.ROB.ROBENTRY[newState.ROB.head].validValue =
					NOTCALCULATED;
					newState.ROB.ROBENTRY[newState.ROB.head].destReg = 0;
					newState.ROB.ROBENTRY[newState.ROB.head].destValue = 0;
					newState.ROB.ROBENTRY[newState.ROB.head].operation =
					NOINSTRUCTION;
					newState.ROB.ROBENTRY[newState.ROB.head].offset = 0;
					newState.ROB.ROBENTRY[newState.ROB.head].instruction =
					EMPTY;
					incHead();
					break;

				case NOOP:

					/*DO NOTHING*/
					newState.ROB.ROBENTRY[newState.ROB.head].validValue =
					NOTCALCULATED;
					newState.ROB.ROBENTRY[newState.ROB.head].destReg = 0;
					newState.ROB.ROBENTRY[newState.ROB.head].destValue = 0;
					newState.ROB.ROBENTRY[newState.ROB.head].operation =
					NOINSTRUCTION;
					newState.ROB.ROBENTRY[newState.ROB.head].offset = 0;
					newState.ROB.ROBENTRY[newState.ROB.head].instruction =
					EMPTY;
					incHead();
					break;

				case HALT:
					/*Return to MAIN*/
					for (i = 0; i < NUMROB; i++) {
						newState.ROB.ROBENTRY[i].destReg = 0;
						newState.ROB.ROBENTRY[i].destValue = 0;
						newState.ROB.ROBENTRY[i].offset = 0;
						newState.ROB.ROBENTRY[i].operation = EMPTY;
						newState.ROB.ROBENTRY[i].validValue =
						NOTCALCULATED;
						newState.ROB.ROBENTRY[i].instruction =
						EMPTY;
					}

					newState.ROB.full = FALSE;
					newState.ROB.head = 0;
					newState.ROB.tail = 0;
					for (i = 0; i < 3; i++) {
						newState.RS.IntegerRS[i].busy = FALSE;
						newState.RS.IntegerRS[i].cycleID = 0;
						newState.RS.IntegerRS[i].scheduled = FALSE;
						newState.RS.IntegerRS[i].operation = 0;
						newState.RS.IntegerRS[i].physicalDest = 0;
						newState.RS.IntegerRS[i].physicalSrc1 = 0;
						newState.RS.IntegerRS[i].physicalSrc2 = 0;
						newState.RS.IntegerRS[i].readSrc1 = 0;
						newState.RS.IntegerRS[i].readSrc2 = 0;
						newState.RS.IntegerRS[i].validSrc1 = FALSE;
						newState.RS.IntegerRS[i].validSrc2 = FALSE;

						newState.RS.MultiplyRS[i].busy = FALSE;
						newState.RS.MultiplyRS[i].cycleID = 0;
						newState.RS.MultiplyRS[i].operation = 0;
						newState.RS.MultiplyRS[i].physicalDest = 0;
						newState.RS.MultiplyRS[i].physicalSrc1 = 0;
						newState.RS.MultiplyRS[i].physicalSrc2 = 0;
						newState.RS.MultiplyRS[i].readSrc1 = 0;
						newState.RS.MultiplyRS[i].readSrc2 = 0;
						newState.RS.MultiplyRS[i].validSrc1 = FALSE;
						newState.RS.MultiplyRS[i].validSrc2 = FALSE;

						newState.RS.LoadRS[i].busy = FALSE;
						newState.RS.LoadRS[i].cycleID = 0;
						newState.RS.LoadRS[i].operation = 0;
						newState.RS.LoadRS[i].physicalDest = 0;
						newState.RS.LoadRS[i].physicalSrc1 = 0;
						newState.RS.LoadRS[i].physicalSrc2 = 0;
						newState.RS.LoadRS[i].readSrc1 = 0;
						newState.RS.LoadRS[i].readSrc2 = 0;
						newState.RS.LoadRS[i].validSrc1 = FALSE;
						newState.RS.LoadRS[i].validSrc2 = FALSE;
					}

					for (i = 0; i < NUMREGS; i++) {
						newState.RT.renameTable[i][0] = 0;
						newState.RT.renameTable[i][1] = INVALID;
						newState.RT.renameTable[i][2] = EMPTY;
						newState.RT.renameTable[i][3] = COMMITTED;
					}

					for (i = 0; i < 1; i++) {
						newState.intUnit.stage[i].operation = EMPTY;
						newState.intUnit.stage[i].readSrc1 = 0;
						newState.intUnit.stage[i].readSrc2 = 0;
					}

					for (i = 0; i < 6; i++) {
						newState.multUnit.stage[i].operation =
						EMPTY;
						newState.multUnit.stage[i].readSrc1 = 0;
						newState.multUnit.stage[i].readSrc2 = 0;
					}

					for (i = 0; i < 3; i++) {
						newState.loadUnit.stage[i].operation =
						EMPTY;
					}
					/*IMPORTANT! Delete both the tables and not one!*/
					state = newState;
					incHead();
					return;
					break;

				case BEQ:

					if (state.ROB.ROBENTRY[newState.ROB.head].destValue == 0) {
						/*branching TRUE*/
						predictedPath = modifyBTB(
								state.ROB.ROBENTRY[newState.ROB.head].pc,
								state.ROB.ROBENTRY[newState.ROB.head].offset,
								BRANCH_TRUE);
						if (predictedPath == 1) {
							/*Branching was not predicted at IFID stage
							 * Need to flush out all the instructions
							 * after the branch*/
							newState.pc =
									state.ROB.ROBENTRY[newState.ROB.head].pc + 1
											+ state.ROB.ROBENTRY[newState.ROB.head].offset;
							newState.Fetch.instr1 = NOINSTRUCTION;
							newState.Fetch.instr2 = NOINSTRUCTION;
							/*IMPORTANT! Return non-committed register values*/
							for (i = 0; i < NUMREGS; i++) {
								if (newState.RT.renameTable[i][1] == VALID
										&& newState.RT.renameTable[i][3]
												== NOTCOMMITTED) {
									newState.reg[i] =
											newState.RT.renameTable[i][2];
								}
							}
							for (i = 0; i < NUMROB; i++) {
								newState.ROB.ROBENTRY[i].destReg = 0;
								newState.ROB.ROBENTRY[i].destValue = 0;
								newState.ROB.ROBENTRY[i].offset = 0;
								newState.ROB.ROBENTRY[i].operation =
								EMPTY;
								newState.ROB.ROBENTRY[i].validValue =
								NOTCALCULATED;
								newState.ROB.ROBENTRY[i].instruction =
								EMPTY;
							}

							newState.ROB.full = FALSE;
							newState.ROB.head = 0;
							newState.ROB.tail = 0;

							for (i = 0; i < 3; i++) {
								newState.RS.IntegerRS[i].busy = FALSE;
								newState.RS.IntegerRS[i].cycleID = 0;
								newState.RS.IntegerRS[i].scheduled = FALSE;
								newState.RS.IntegerRS[i].operation = 0;
								newState.RS.IntegerRS[i].physicalDest = 0;
								newState.RS.IntegerRS[i].physicalSrc1 = 0;
								newState.RS.IntegerRS[i].physicalSrc2 = 0;
								newState.RS.IntegerRS[i].readSrc1 = 0;
								newState.RS.IntegerRS[i].readSrc2 = 0;
								newState.RS.IntegerRS[i].validSrc1 =
								FALSE;
								newState.RS.IntegerRS[i].validSrc2 =
								FALSE;

								newState.RS.MultiplyRS[i].busy = FALSE;
								newState.RS.MultiplyRS[i].cycleID = 0;
								newState.RS.MultiplyRS[i].operation = 0;
								newState.RS.MultiplyRS[i].physicalDest = 0;
								newState.RS.MultiplyRS[i].physicalSrc1 = 0;
								newState.RS.MultiplyRS[i].physicalSrc2 = 0;
								newState.RS.MultiplyRS[i].readSrc1 = 0;
								newState.RS.MultiplyRS[i].readSrc2 = 0;
								newState.RS.MultiplyRS[i].validSrc1 =
								FALSE;
								newState.RS.MultiplyRS[i].validSrc2 =
								FALSE;

								newState.RS.LoadRS[i].busy = FALSE;
								newState.RS.LoadRS[i].cycleID = 0;
								newState.RS.LoadRS[i].operation = 0;
								newState.RS.LoadRS[i].physicalDest = 0;
								newState.RS.LoadRS[i].physicalSrc1 = 0;
								newState.RS.LoadRS[i].physicalSrc2 = 0;
								newState.RS.LoadRS[i].readSrc1 = 0;
								newState.RS.LoadRS[i].readSrc2 = 0;
								newState.RS.LoadRS[i].validSrc1 = FALSE;
								newState.RS.LoadRS[i].validSrc2 = FALSE;
							}

							for (i = 0; i < NUMREGS; i++) {
								newState.RT.renameTable[i][0] = 0;
								newState.RT.renameTable[i][1] = INVALID;
								newState.RT.renameTable[i][2] = EMPTY;
								newState.RT.renameTable[i][3] = COMMITTED;
							}

							for (i = 0; i < 1; i++) {
								newState.intUnit.stage[i].operation =
								EMPTY;
								newState.intUnit.stage[i].readSrc1 = 0;
								newState.intUnit.stage[i].readSrc2 = 0;
							}

							for (i = 0; i < 6; i++) {
								newState.multUnit.stage[i].operation =
								EMPTY;
								newState.multUnit.stage[i].readSrc1 = 0;
								newState.multUnit.stage[i].readSrc2 = 0;
							}

							for (i = 0; i < 3; i++) {
								newState.loadUnit.stage[i].operation =
								EMPTY;
							}

							/*IMPORTANT! Delete both the tables and not one!*/
							state = newState;
						} else {
							/*Branch was predicted at IFID stage*/
							newState.ROB.ROBENTRY[newState.ROB.head].validValue =
							NOTCALCULATED;
							newState.ROB.ROBENTRY[newState.ROB.head].destReg =
									0;
							newState.ROB.ROBENTRY[newState.ROB.head].destValue =
									0;
							newState.ROB.ROBENTRY[newState.ROB.head].operation =
							NOINSTRUCTION;
							newState.ROB.ROBENTRY[newState.ROB.head].offset = 0;
							newState.ROB.ROBENTRY[newState.ROB.head].instruction =
							EMPTY;
							incHead();
						}
					} else {
						/*branching FALSE*/
						predictedPath = modifyBTB(
								newState.ROB.ROBENTRY[newState.ROB.head].pc,
								newState.ROB.ROBENTRY[newState.ROB.head].offset,
								BRANCH_FALSE);
						if (predictedPath == 1) {
							/*Branch was predicted at IFID but not TAKEN!
							 * MISPREDICTION by the branch predictor
							 * Need to flush out all the instructions
							 * after the branch*/
							newState.pc =
									newState.ROB.ROBENTRY[newState.ROB.head].pc
											+ 1;
							newState.Fetch.instr1 = NOINSTRUCTION;
							newState.Fetch.instr2 = NOINSTRUCTION;
							/*IMPORTANT! Return non-committed register values*/
							for (i = 0; i < NUMREGS; i++) {
								if (newState.RT.renameTable[i][1] == VALID
										&& newState.RT.renameTable[i][3]
												== NOTCOMMITTED) {
									newState.reg[i] =
											newState.RT.renameTable[i][2];
								}
							}
							for (i = 0; i < NUMROB; i++) {
								newState.ROB.ROBENTRY[i].destReg = 0;
								newState.ROB.ROBENTRY[i].destValue = 0;
								newState.ROB.ROBENTRY[i].offset = 0;
								newState.ROB.ROBENTRY[i].operation =
								EMPTY;
								newState.ROB.ROBENTRY[i].validValue =
								NOTCALCULATED;
								newState.ROB.ROBENTRY[i].instruction =
								EMPTY;
							}

							newState.ROB.full = FALSE;
							newState.ROB.head = 0;
							newState.ROB.tail = 0;

							for (i = 0; i < 3; i++) {
								newState.RS.IntegerRS[i].busy = FALSE;
								newState.RS.IntegerRS[i].cycleID = 0;
								newState.RS.IntegerRS[i].scheduled = FALSE;
								newState.RS.IntegerRS[i].operation = 0;
								newState.RS.IntegerRS[i].physicalDest = 0;
								newState.RS.IntegerRS[i].physicalSrc1 = 0;
								newState.RS.IntegerRS[i].physicalSrc2 = 0;
								newState.RS.IntegerRS[i].readSrc1 = 0;
								newState.RS.IntegerRS[i].readSrc2 = 0;
								newState.RS.IntegerRS[i].validSrc1 =
								FALSE;
								newState.RS.IntegerRS[i].validSrc2 =
								FALSE;

								newState.RS.MultiplyRS[i].busy = FALSE;
								newState.RS.MultiplyRS[i].cycleID = 0;
								newState.RS.MultiplyRS[i].operation = 0;
								newState.RS.MultiplyRS[i].physicalDest = 0;
								newState.RS.MultiplyRS[i].physicalSrc1 = 0;
								newState.RS.MultiplyRS[i].physicalSrc2 = 0;
								newState.RS.MultiplyRS[i].readSrc1 = 0;
								newState.RS.MultiplyRS[i].readSrc2 = 0;
								newState.RS.MultiplyRS[i].validSrc1 =
								FALSE;
								newState.RS.MultiplyRS[i].validSrc2 =
								FALSE;

								newState.RS.LoadRS[i].busy = FALSE;
								newState.RS.LoadRS[i].cycleID = 0;
								newState.RS.LoadRS[i].operation = 0;
								newState.RS.LoadRS[i].physicalDest = 0;
								newState.RS.LoadRS[i].physicalSrc1 = 0;
								newState.RS.LoadRS[i].physicalSrc2 = 0;
								newState.RS.LoadRS[i].readSrc1 = 0;
								newState.RS.LoadRS[i].readSrc2 = 0;
								newState.RS.LoadRS[i].validSrc1 = FALSE;
								newState.RS.LoadRS[i].validSrc2 = FALSE;
							}

							for (i = 0; i < NUMREGS; i++) {
								newState.RT.renameTable[i][0] = 0;
								newState.RT.renameTable[i][1] = INVALID;
								newState.RT.renameTable[i][2] = EMPTY;
								newState.RT.renameTable[i][3] = COMMITTED;
							}

							for (i = 0; i < 1; i++) {
								newState.intUnit.stage[i].operation =
								EMPTY;
								newState.intUnit.stage[i].readSrc1 = 0;
								newState.intUnit.stage[i].readSrc2 = 0;
							}

							for (i = 0; i < 6; i++) {
								newState.multUnit.stage[i].operation =
								EMPTY;
								newState.multUnit.stage[i].readSrc1 = 0;
								newState.multUnit.stage[i].readSrc2 = 0;
							}

							for (i = 0; i < 3; i++) {
								newState.loadUnit.stage[i].operation =
								EMPTY;
							}

							/*IMPORTANT! Delete both the tables and not one!*/
							state = newState;
						} else {
							/*Not Predicted/ Correct prediction!*/
							newState.ROB.ROBENTRY[newState.ROB.head].validValue =
							NOTCALCULATED;
							newState.ROB.ROBENTRY[newState.ROB.head].destReg =
									0;
							newState.ROB.ROBENTRY[newState.ROB.head].destValue =
									0;
							newState.ROB.ROBENTRY[newState.ROB.head].operation =
							NOINSTRUCTION;
							newState.ROB.ROBENTRY[newState.ROB.head].offset = 0;
							newState.ROB.ROBENTRY[newState.ROB.head].instruction =
							EMPTY;
							incHead();
						}
					}
					break;
				}
			}
		}
		state = newState;
	}

}

int main(int argc, char **argv) {
	int i = 0, count = 0;

	/* --------------------- Get the instructions --------------------- */
	FILE *fp = fopen(argv[1], "r");
	while (fscanf(fp, "%d\n", &i) == 1) {
		state.instrMem[count] = i;
		state.dataMem[count] = i;
		count++;
	}
	fclose(fp);
	state.numMemory = count;

	for (i = 0; i < NUMREGS; i++) {
		state.reg[i] = 0;
	}

	state.pc = 0;
	state.cycles = 0;
	state.fetched = 0;
	state.retired = 0;
	state.mispred = 0;
	state.branches = 0;

	state.Fetch.instr1 = NOOPINSTRUCTION;
	state.Fetch.instr2 = NOOPINSTRUCTION;

	for (i = 0; i < NUMROB; i++) {
		state.ROB.ROBENTRY[i].destReg = 0;
		state.ROB.ROBENTRY[i].destValue = 0;
		state.ROB.ROBENTRY[i].offset = 0;
		state.ROB.ROBENTRY[i].operation = EMPTY;
		state.ROB.ROBENTRY[i].validValue = NOTCALCULATED;
	}

	state.ROB.full = FALSE;
	state.ROB.head = 0;
	state.ROB.tail = 0;

	for (i = 0; i < 3; i++) {
		state.RS.IntegerRS[i].busy = FALSE;
		state.RS.IntegerRS[i].scheduled = FALSE;
		state.RS.IntegerRS[i].cycleID = 0;
		state.RS.IntegerRS[i].operation = 0;
		state.RS.IntegerRS[i].physicalDest = 0;
		state.RS.IntegerRS[i].physicalSrc1 = 0;
		state.RS.IntegerRS[i].physicalSrc2 = 0;
		state.RS.IntegerRS[i].readSrc1 = 0;
		state.RS.IntegerRS[i].readSrc2 = 0;
		state.RS.IntegerRS[i].validSrc1 = FALSE;
		state.RS.IntegerRS[i].validSrc2 = FALSE;

		state.RS.MultiplyRS[i].busy = FALSE;
		state.RS.MultiplyRS[i].scheduled = FALSE;
		state.RS.MultiplyRS[i].cycleID = 0;
		state.RS.MultiplyRS[i].operation = 0;
		state.RS.MultiplyRS[i].physicalDest = 0;
		state.RS.MultiplyRS[i].physicalSrc1 = 0;
		state.RS.MultiplyRS[i].physicalSrc2 = 0;
		state.RS.MultiplyRS[i].readSrc1 = 0;
		state.RS.MultiplyRS[i].readSrc2 = 0;
		state.RS.MultiplyRS[i].validSrc1 = FALSE;
		state.RS.MultiplyRS[i].validSrc2 = FALSE;

		state.RS.LoadRS[i].busy = FALSE;
		state.RS.LoadRS[i].scheduled = FALSE;
		state.RS.LoadRS[i].cycleID = 0;
		state.RS.LoadRS[i].operation = 0;
		state.RS.LoadRS[i].physicalDest = 0;
		state.RS.LoadRS[i].physicalSrc1 = 0;
		state.RS.LoadRS[i].physicalSrc2 = 0;
		state.RS.LoadRS[i].readSrc1 = 0;
		state.RS.LoadRS[i].readSrc2 = 0;
		state.RS.LoadRS[i].validSrc1 = FALSE;
		state.RS.LoadRS[i].validSrc2 = FALSE;
	}

	for (i = 0; i < NUMREGS; i++) {
		state.RT.renameTable[i][0] = 0;
		state.RT.renameTable[i][1] = FALSE;
	}

	for (i = 0; i < 1; i++) {
		state.intUnit.stage[i].operation = EMPTY;
		state.intUnit.stage[i].readSrc1 = 0;
		state.intUnit.stage[i].readSrc2 = 0;
	}

	for (i = 0; i < 6; i++) {
		state.multUnit.stage[i].operation = EMPTY;
		state.intUnit.stage[i].readSrc1 = 0;
		state.intUnit.stage[i].readSrc2 = 0;
	}

	for (i = 0; i < 3; i++) {
		state.loadUnit.stage[i].operation = EMPTY;
	}

	for (i = 0; i < 64; i++) {
		branchHistoryTable.branchHistory[i] = WEAKLY_NOT_TAKEN;
	}

	for (i = 0; i < 3; i++) {
		branchBuffer.BTB[i][0] = 0;
		branchBuffer.BTB[i][1] = 0;
		branchBuffer.BTB[i][2] = 1;
	}
	branchBuffer.curPos = 0;

	run();
#if MOREOUTPUT == TRUE
	printState();
#endif
	printf("machine halted\n");
	printf("total of %d cycles executed\n", state.cycles - 1);
	printf("%-10s: %d\n", "CYCLES", state.cycles - 1);
	printf("%-10s: %d\n", "FETCHED", state.fetched);
	printf("%-10s: %d\n", "RETIRED", state.retired);
	printf("%-10s: %d\n", "BRANCHES", state.branches);
	printf("%-10s: %d\n", "MISPRED", state.mispred);
	exit(0);
}
