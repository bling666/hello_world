unsigned short sum = 0;
unsigned short tempNum = 0;
for (int i = 0; i < 10; i++) {
	tempNum = ((unsigned char)send[i * 2] << 8) + (unsigned char)send[i * 2 + 1];
	if (0xffff - sum < tempNum)
		sum = sum + tempNum + 1;
	else
		sum = sum + tempNum;
}