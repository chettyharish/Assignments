	lw	0	2	mult	the number to multiply
	lw	0	4	table	multiply upto this number
	lw	0	5	loc	storage location in memory
	lw	0	6	one	load 1 in reg6
lst	beq	3	4	end	counter==table
	add	3	6	3	increment counter
	mult	2	3	1	multiply the number and counter
	sw	5	1	0	store the result at loc
	add	5	6	5	increment loc
	beq	0	0	lst
end	halt
mult	.fill	5
table	.fill	10
one	.fill	1
loc	.fill	15
	.fill	0			inserting 0's to make the output visible
	.fill	0
	.fill	0
	.fill	0
	.fill	0
	.fill	0
	.fill	0
	.fill	0
	.fill	0
	.fill	0
	.fill	0
