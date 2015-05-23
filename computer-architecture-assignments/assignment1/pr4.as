	lw	0	1	nine	load reg1 with 9
	lw	0	2	mthree	load reg2 with -3
cycle	add	1	2	3	reg3 = reg1+reg2
	nand	3	1	4	reg4 = ~(reg3 & reg1)
	mult	4	3	5	reg5 = reg4*reg3
	nand	4	5	6	reg6 = ~(reg4 & reg5)
	mult	4	5	7	reg7 = reg4*reg5
	add	0	3	1	reg1 = reg3
	beq	2	3	exit	break loop when reg3 = -3
	beq	0	0	cycle	jump to loop start
exit	halt
nine	.fill	9	
mthree	.fill	-3	
