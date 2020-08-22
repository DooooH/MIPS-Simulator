#include <stdio.h>

//some definitions
#define FALSE 0
#define TRUE 1
#define FMT1 0b00000011111000000000000000000000 //25-21
#define FMT2 0b00000000000111110000000000000000 //20-16
#define FMT3 0b00000000000000001111100000000000 //15-11
#define FMT4 0b00000000000000001111111111111111 //15-0
#define FMT5 0b00000000000000000000000000111111 //5-0
#define FMT6 0b11111111111111110000000000000000 //sign_msb == 1
#define FMT7 0b00000000000000001111111111111111 //sing_msb == 0
#define FMT8 0b00000011111111111111111111111111 //25-0

//MUX mode
#define REGDST 0
#define BRANCH 1
#define MEMREAD 2
#define MEMTOREG 3
#define ALUOP 4
#define MEMWRITE 5
#define ALUSRC 6
#define REGWRITE 7
#define PCSRC 8
#define JUMP 9

//clock cycles
long long cycles = 0;

// registers
int regs[32];

// program counter
unsigned long pc = 0;

// memory
#define INST_MEM_SIZE 32*1024
#define DATA_MEM_SIZE 32*1024
unsigned long inst_mem[INST_MEM_SIZE]; //instruction memory
unsigned long data_mem[DATA_MEM_SIZE]; //data memory

//misc. function
int init(char* filename);
void fetch();
void decode();
void exe();
void mem();
void wb();
void update_pc();
void mainControl();
void ALU();
void ALUcontroller();
void signExtend();
int MUX(int);
//print function
void print_cycles();
void print_reg();
void print_pc();

//variable
int inst_idx = 0;
//fetch
unsigned long opcode;
unsigned long inst2500;
unsigned long inst2521;
unsigned long inst2016;
unsigned long inst1511;
long inst1500; //signed
unsigned long inst0500;
/////else
long extended; //sign extend
int RegDst, Branch, MemRead, MemtoReg, ALUOp, MemWrite, ALUSrc, RegWrite, PCSrc, Jump, Jal, Jr; //signals
long ALUcontrol; //signal
int readData1, readData2, readMem, ALUoutput, Zero; //data


int main(int ac, char *av[])
{
	if (ac < 3 )
	{
		printf("./mips_sim filename mode\n");
        return -1;
	}
	
	char done = FALSE;
	if(init(av[1])!=0)
        return -1;
	while (!done)
	{
		inst_idx = (pc / 4); //reflect pc
    // Todo
		fetch();     //fetch an instruction from the instruction memory //opcode(implement control),rs,rd,funct
		decode();    //decode the instruction and read data from register file
		exe();       //perform an appropriate operation 
		mem();       //access the data memory
		wb();        //write result of arithmetic operation or data read from the data memory to register file
		update_pc(); //update the pc to the address of the next instruction
    

		cycles++;    //increase clock cycle

		//if debug mode, print clock cycle, pc, reg
		if(*av[2]=='0'){
					 print_cycles();  //print clock cycles
					 print_pc();		 //print pc
					 print_reg();	 //print registers
        }
        // check the exit condition 
		if (regs[9] == 10)  //if value in $t1 is 10, finish the simulation
			done = TRUE;

	}

    if(*av[2]=='1')
    {
        print_cycles();  //print clock cycles
        print_pc();		 //print pc
        print_reg();	 //print registers
    }

	return 0;
}


/* initialize all datapat elements
//fill the instruction and data memory
//reset the registers
*/
int init(char* filename)
{
	FILE* fp = fopen(filename, "r");
	int i;
	long inst;

	if (fp == NULL)
	{
		fprintf(stderr, "Error opening file.\n");
		return -1;
	}

	/* fill instruction memory */
	i = 0;
	while (fscanf(fp, "%lx", &inst) == 1)
	{
		inst_mem[i++] = inst;
	}

	/*reset the registers*/
	for (i = 0; i<32; i++)
	{
		regs[i] = 0;
	}

	/*reset pc*/
	pc = 0;
    /*reset clock cycles*/
    cycles=0;
    return 0;
}

void fetch()
{
	opcode = inst_mem[inst_idx] >> 26;
	inst2500 = ( inst_mem[inst_idx] & FMT8 );
	inst2521 = ( inst_mem[inst_idx] & FMT1 ) >> 21;
	inst2016 = ( inst_mem[inst_idx] & FMT2 ) >> 16;
	inst1511 = ( inst_mem[inst_idx] & FMT3 ) >> 11;
	inst1500 = ( inst_mem[inst_idx] & FMT4 ) ;
	inst0500 = ( inst_mem[inst_idx++] & FMT5 ) ;
	signExtend();

	mainControl();
}

void decode()
{
	readData1 = regs[inst2521];
	readData2 = regs[inst2016];
}

void exe()
{
	ALUcontroller();
	ALU();
}

void mem() //check after MainControl
{
	if (MemRead == 0 && MemWrite == 0)
	{
		;
		//to WB()
	}
	else if (MemWrite == 1)
	{
		data_mem[ALUoutput] = readData2;
		
	}
	else if (MemRead == 1)
	{
		readMem = data_mem[ALUoutput];
	}
}

void wb()
{
	if (RegWrite == 1)
	{
		
		regs[MUX(REGDST)] = MUX(MEMTOREG);
	}
	else
		return;
}

void update_pc()
{
	if (Jr == 1)
	{
		pc = readData1; //ra to pc
	}
	else
	{
		PCSrc = (Branch & Zero);
		pc = MUX(JUMP);
	}
}

void mainControl()
{
	switch (opcode)
	{
	case 0: //add,sub,jr
		if (inst0500 == 8) //jr 
		{
			RegDst = 0;
			Branch = 0;
			MemRead = 0;
			MemtoReg = 0;
			ALUOp = 0;
			MemWrite = 0;
			ALUSrc = 0;
			RegWrite = 0;
			Jump = 0;
			Jal = 0;
			Jr = 1;
		}
		else
		{
			RegDst = 1;
			Branch = 0;
			MemRead = 0;
			MemtoReg = 0;
			ALUOp = 2;
			MemWrite = 0;
			ALUSrc = 0;
			RegWrite = 1;
			Jump = 0;
			Jal = 0;
			Jr = 0;
		}
		break;
	case 8: //addi
		RegDst = 0;
		Branch = 0;
		MemRead = 0;
		MemtoReg = 0;
		ALUOp = 0;
		MemWrite = 0;
		ALUSrc = 1;
		RegWrite = 1;
		Jump = 0;
		Jal = 0;
		Jr = 0;
		break;
	case 3: //jal  
		RegDst = 2; //for jal
		Branch = 0;
		MemRead = 0;
		MemtoReg = 2; //for jal
		ALUOp = 0;
		MemWrite = 0;
		ALUSrc = 0;
		RegWrite = 1;
		Jump = 1;
		Jal = 1;
		Jr = 0;
		break;
	case 2: //j
		RegDst = 0;
		Branch = 0;
		MemRead = 0;
		MemtoReg = 0;
		ALUOp = 0;
		MemWrite = 0;
		ALUSrc = 0;
		RegWrite = 0;
		Jump = 1;
		Jal = 0;
		Jr = 0;
		break;
	case 35: //lw
		RegDst = 0;
		Branch = 0;
		MemRead = 1;
		MemtoReg = 1;
		ALUOp = 0;
		MemWrite = 0;
		ALUSrc = 1;
		RegWrite = 1;
		Jump = 0;
		Jal = 0;
		Jr = 0;
		break;
	case 43: //sw
		RegDst = 0;
		Branch = 0;
		MemRead = 0;
		MemtoReg = 0;
		ALUOp = 0;
		MemWrite = 1;
		ALUSrc = 1;
		RegWrite = 0;
		Jump = 0;
		Jal = 0;
		Jr = 0;
		break;
	case 10: //slti
		RegDst = 0;
		Branch = 0;
		MemRead = 0;
		MemtoReg = 0;
		ALUOp = 2;
		MemWrite = 0;
		ALUSrc = 1;
		RegWrite = 1;
		Jump = 0;
		Jal = 0;
		Jr = 0;
		break;
	case 4: //beq
		RegDst = 0;
		Branch = 1;
		MemRead = 0;
		MemtoReg = 0;
		ALUOp = 1;
		MemWrite = 0;
		ALUSrc = 0;
		RegWrite = 0;
		Jump = 0;
		Jal = 0;
		Jr = 0;
		break;
	}
}


void ALU()
{
	int src; //ALUsrc

	src = MUX(ALUSRC);

	switch (ALUcontrol)
	{
	case 0: //AND
		ALUoutput = src & readData1;
		break;
	case 1: //OR
		ALUoutput = src | readData1;
		break;
	case 2: //add
		ALUoutput = src + readData1;
		break;
	case 6: //sub
		if (src >= readData1)
			ALUoutput = src - readData1;
		else
			ALUoutput = readData1 - src;

		if (ALUoutput == 0)
			Zero = 1;
		else
			Zero = 0;
		break;
	case 7: //slti
		if (readData1 < src)
			ALUoutput = 1;
		else
			ALUoutput = 0;
		break;
	}
}

void ALUcontroller()
{
	switch (ALUOp)
	{
	case 0://lw, sw
		ALUcontrol = 2;
		break;
	case 1: //beq
		ALUcontrol = 6;
		break;
	case 2: //R-type, slti
		if (inst0500 == 32) // add
		{
			ALUcontrol = 2;
		}
		else if (inst0500 == 34) //sub
		{
			ALUcontrol = 6;
		}
		else if (inst0500 == 36) //AND
		{
			ALUcontrol = 0;
		}
		else if (inst0500 == 37) //OR
		{
			ALUcontrol = 1;
		}
		else if (opcode == 10) //slti
		{
			ALUcontrol = 7;
		}
		break;
	}
}

void signExtend()
{
	if ((inst1500 >> 15) == 1) //MSB == 1
	{
		extended = inst1500 | FMT6; 
	}
	else
	{
		extended = inst1500 & FMT7;
	}
}

int MUX(int n) 
{
	switch(n)
	{
	case REGDST:
		if (RegDst == 0)
			return inst2016;
		else if (RegDst == 2)
		{
			return 31;
		}	
		else//RegDst 1
			return inst1511;
	case ALUSRC:
		if (ALUSrc == 0)
			return readData2;
		else //from sign-extend
			return extended;
	case PCSRC:
		if (PCSrc == 0)// pc + 4
			return pc+4;
		else //branch
			return (extended<<2) + (pc+4) ;
	case MEMTOREG:
		if (MemtoReg == 0)
			return ALUoutput;
		else if (MemtoReg == 2)
			return pc + 4;
		else
			return readMem;
	case JUMP:
		if (Jump == 0)
			return MUX(PCSRC);
		else
			return ((pc + 4) & 0b11110000000000000000000000000000) | (inst2500 << 2);
	}
}

void print_cycles()
{
	printf("---------------------------------------------------\n");

	printf("Clock cycles = %lld\n", cycles);
}


void print_pc()
{
	printf("PC	   = %ld\n\n", pc);
}


void print_reg()
{
	printf("R0   [r0] = %d\n", regs[0]);
	printf("R1   [at] = %d\n", regs[1]);
	printf("R2   [v0] = %d\n", regs[2]);
	printf("R3   [v1] = %d\n", regs[3]);
	printf("R4   [a0] = %d\n", regs[4]);
	printf("R5   [a1] = %d\n", regs[5]);
	printf("R6   [a2] = %d\n", regs[6]);
	printf("R7   [a3] = %d\n", regs[7]);
	printf("R8   [t0] = %d\n", regs[8]);
	printf("R9   [t1] = %d\n", regs[9]);
	printf("R10  [t2] = %d\n", regs[10]);
	printf("R11  [t3] = %d\n", regs[11]);
	printf("R12  [t4] = %d\n", regs[12]);
	printf("R13  [t5] = %d\n", regs[13]);
	printf("R14  [t6] = %d\n", regs[14]);
	printf("R15  [t7] = %d\n", regs[15]);
	printf("R16  [s0] = %d\n", regs[16]);
	printf("R17  [s1] = %d\n", regs[17]);
	printf("R18  [s2] = %d\n", regs[18]);
	printf("R19  [s3] = %d\n", regs[19]);
	printf("R20  [s4] = %d\n", regs[20]);
	printf("R21  [s5] = %d\n", regs[21]);
	printf("R22  [s6] = %d\n", regs[22]);
	printf("R23  [s7] = %d\n", regs[23]);
	printf("R24  [t8] = %d\n", regs[24]);
	printf("R25  [t9] = %d\n", regs[25]);
	printf("R26  [k0] = %d\n", regs[26]);
	printf("R27  [k1] = %d\n", regs[27]);
	printf("R28  [gp] = %d\n", regs[28]);
	printf("R29  [sp] = %d\n", regs[29]);
	printf("R30  [fp] = %d\n", regs[30]);
	printf("R31  [ra] = %d\n", regs[31]);
	printf("\n");
}


