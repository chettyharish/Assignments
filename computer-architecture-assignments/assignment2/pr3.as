	lw	0	2	val1	load reg2 = 4
	lw	0	3	val2	load reg3 = -1
b0	beq	0	1	b1	Always branch to b1
	noop
	noop
	noop
b1	beq	0	1	b2	always branch to b2
	noop
	noop
	noop
b2	beq	0	1	b3	always branch to b3
	noop
	noop
	noop
b3	beq	0	1	b4	always branch to b4
	noop
	noop
	noop
b4	beq	0	2	end	break loop if reg2 = 0
	add	2	3	2	reg2 = reg2 - 1
	beq	0	0	b0	jump to loop start b0
end	noop
	noop
	halt
val1	.fill	4
val2	.fill	-1
