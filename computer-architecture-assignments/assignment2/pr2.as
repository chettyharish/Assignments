	lw	0	1	val1	load 7 in reg1
	lw	0	3	val3	load -1 in reg3
outer	beq	0	1	obreak	break outer loop when reg1 is 0
	add	1	3	1	reg1 = reg1 - 1
	lw	0	2	val2	load 3 in reg2
inner	beq	0	2	ibreak	break inner loop when reg2 is 0
	add	2	3	2	reg2 = reg2 -1
	beq	0	0	inner	goto start of inner loop
ibreak	beq	0	0	outer	goto start of outer loop 
obreak	noop
done	halt
val1	.fill	7
val2	.fill	3
val3	.fill	-1
