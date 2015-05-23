	lw	0	1	one	load 1 in reg1
	lw	0	2	two	load 2 in reg2
	lw	0	3	one	load 1 in reg3
	lw	0	4	two	load 2 in reg4
	lw	0	5	lst	load loopstart in reg5
	lw	0	6	lend	load loopend in reg6
	lw	0	7	lem1	load loopend-1 in reg7
spadd	beq	1	6	modsp	if sp==loopend then sp = loopstart
	add	1	3	1	if sp!=loopend then sp = sp+1
	beq	0	0	fpadd
modsp	add	0	5	1	
fpadd	beq	2	7	modfp1	if fp==loopend-1 then fp = loopstart
	beq	2	6	modfp2	if fp==loopend then fp = loopstart+1
	add	2	4	2	if fp!=loopend && fp!=loopend-1 then fp = fp+2
	beq	0	0	loop
modfp1	add	0	5	2
	beq	0	0	loop
modfp2	add	0	5	2
	add	5	3	2
loop	beq	1	2	out	if fp==sp break else goto start of loop
	beq	0	0	spadd
out	noop	0	0	0
	add	0	0	1
spad2	beq	1	6	mosp2	if sp==loopend then sp = loopstart
	add	1	3	1	if sp!=loopend then sp = sp+1
	beq	0	0	fpad2
mosp2	add	0	5	1
fpad2	beq	2	6	mofp2	if fp==loopend then fp = loopstart
	add	2	3	2	if fp!=loopend then fp = fp+1
	beq	0	0	loop2
mofp2	add	0	5	2	
loop2	beq	1	2	exit	if fp==sp break else goto start of loop
	beq	0	0	spad2
exit	halt
one	.fill	1
two	.fill	2
lst	.fill	11
lend	.fill	200
lem1	.fill	199
