#include"zip.h"

bool InitialFiles(char *soufilename, FILE* &pinf, char *objfilename, FILE* &poutf)
{
	char *temp;
	if (objfilename[0] == '\0')
	{
		if ((temp = strrchr(soufilename, '.')) == NULL)
		{
			strncpy(objfilename, soufilename, sizeof(soufilename));
			strcat(objfilename, ".zip");
		}
		else
		{
			strncpy(objfilename, soufilename, temp - soufilename);
			objfilename[temp - soufilename] = '\0';
			strcat(objfilename, ".zip");
		}
	}

	if (strcmp(soufilename, objfilename) == 0)
		return false;
	if ((pinf = fopen(soufilename, "rb")) == NULL)
		return false;
	if ((poutf = fopen(objfilename, "wb")) == NULL)
		return false;
	return true;
}

int DataFrequency(FILE *pinf, int frequency[])
{
	unsigned char readbuf[256];
	int filesize;
	int readlen;

	for (int i = 0; i < 256; i++)
		frequency[i] = 0;
	fseek(pinf, 0L, SEEK_SET);
	readlen = 256;

	while (readlen == 256)
	{
		readlen = fread(readbuf, sizeof(char), 256, pinf);
		for (int i = 0; i < readlen; i++)
			frequency[readbuf[i]]++;
	}
	filesize = 0;
	for (int i = 0; i < 256; i++)
		filesize += frequency[i];

	return filesize;
}

void FindTwoMin(HfmNode hfmtree[], int n, int &min1, int &min2)
{
	int minvalue = INT_MAX;
	int minindex = 0;

	for (int i = 0; i < n; i++)
		if (hfmtree[i].parent == -1 && hfmtree[i].weight < minvalue)
		{
			minvalue = hfmtree[i].weight;
			minindex = i;
		}
	min1 = minindex;

	minvalue = INT_MAX;
	minindex = 0;
	for (int i = 0; i < n; i++)
		if (hfmtree[i].parent == -1 && hfmtree[i].weight < minvalue && i != min1)
		{
			minvalue = hfmtree[i].weight;
			minindex = i;
		}
	min2 = minindex;
}

void CreateHfmtree(HfmNode hfmtree[], int frequency[])
{
	for (int i = 0; i < 256; i++)
	{
		hfmtree[i].weight = frequency[i];
		hfmtree[i].parent = hfmtree[i].lchild = hfmtree[i].rchild = -1;
	}
	for (int i = 256; i < 256 * 2 - 1; i++)
		hfmtree[i] = { -1,-1,-1,-1 };

	int min1, min2;
	for (int i = 256; i < 256 * 2 - 1; i++)
	{
		FindTwoMin(hfmtree, i, min1, min2);
		hfmtree[i].lchild = min1;
		hfmtree[i].rchild = min2;
		hfmtree[i].weight = hfmtree[min1].weight + hfmtree[min2].weight;
		hfmtree[min1].parent = hfmtree[min2].parent = i;
	}
}

bool EncodeHfmtree(HfmNode hfmtree[], HfmCode hfmcode[])
{
	unsigned char code[256];
	int codelen;

	for (int i = 0; i < 256; i++)
	{
		codelen = 0;
		for (int j = i; j < 256 * 2 - 2; j = hfmtree[j].parent, codelen++)
			code[255 - codelen] = (j == hfmtree[hfmtree[j].parent].lchild ? 0 : 1);
		if ((hfmcode[i].code = (unsigned char *)malloc(sizeof(char)*(codelen))) == NULL)
			return false;
		hfmcode[i].len = codelen;
		memcpy(hfmcode[i].code, code + 256 - codelen, codelen);
	}

	return true;
}

unsigned char charstobits(unsigned char chars[])
{
	unsigned char bits = 0;
	bits |= chars[0];
	for (int i = 1; i < 8; i++)
	{
		bits <<= 1;
		bits |= chars[i];
	}
	return bits;
}

void WriteCompressFile(FILE *pinf, FILE *poutf, HfmNode hfmtree[], HfmCode hfmcode[], char *soufilename, int soufilesize)
{
	int readcounter, writecounter, ziphead = 0xFFFFFFFF;
	int writecharcounter, codecharcounter, copycharcounter, filenamesize;
	unsigned char readbuf[256], writebuf[256], writechars[8];
	HfmCode *curcode;

	fseek(pinf, 0L, SEEK_SET);
	fseek(poutf, 0L, SEEK_SET);
	filenamesize = strlen(soufilename);
	fwrite(&ziphead, sizeof(int), 1, poutf);
	fwrite(&filenamesize, sizeof(int), 1, poutf);
	fwrite(soufilename, sizeof(char), filenamesize, poutf);
	fwrite(&soufilesize, sizeof(int), 1, poutf);
	for (int i = 256; i < 256 * 2 - 1; i++)
	{
		fwrite(&(hfmtree[i].lchild), sizeof(hfmtree[i].lchild), 1, poutf);
		fwrite(&(hfmtree[i].rchild), sizeof(hfmtree[i].rchild), 1, poutf);
	}

	writecounter = writecharcounter = 0;
	readcounter = 256;
	while (readcounter == 256)
	{
		readcounter = fread(readbuf, 1, 256, pinf);
		for (int i = 0; i < readcounter; i++)
		{
			curcode = &hfmcode[readbuf[i]];
			codecharcounter = 0;
			while (codecharcounter != curcode->len)
			{
				copycharcounter = Smaller(8 - writecharcounter, curcode->len - codecharcounter);
				memcpy(writechars + writecharcounter, curcode->code + codecharcounter, copycharcounter);
				writecharcounter += copycharcounter;
				codecharcounter += copycharcounter;
				if (writecharcounter == 8)
				{
					writecharcounter = 0;
					writebuf[writecounter++] = charstobits(writechars);
					if (writecounter == 256)
					{
						fwrite(writebuf, 1, 256, poutf);
						writecounter = 0;
					}
				}
			}
		}
	}
	fwrite(writebuf, 1, writecounter, poutf);
	if (writecounter != 0)
	{
		unsigned char ch = charstobits(writechars);
		fwrite(&ch, 1, 1, poutf);
	}
}

bool Compress(char *soufilename, char *objfilename)
{
	FILE *pinf, *poutf;
	bool errcode;
	int soufilesize, objfilesize;
	int frequency[256];
	HfmNode hfmtree[256 * 2 - 1];
	HfmCode hfmcode[256];
	double compressrate;

	errcode = InitialFiles(soufilename, pinf, objfilename, poutf);
	if (!errcode)
	{
		printf("InitialFiles Failed.\n");
		return errcode;
	}
	soufilesize = DataFrequency(pinf, frequency);
	CreateHfmtree(hfmtree, frequency);

	errcode = EncodeHfmtree(hfmtree, hfmcode);
	if (!errcode)
	{
		printf("EncodeHfmtree Failed.\n");
		return errcode;
	}

	objfilesize = 0;
	for (int i = 0; i < 256; i++)
		objfilesize += frequency[i] * hfmcode[i].len;
	objfilesize = objfilesize % 8 == 0 ? objfilesize / 8 : objfilesize / 8 + 1;
	for (int i = 0; i < 256 - 1; i++)
		objfilesize += 2 * sizeof(short);//写入hfmtree从256到256*2-1的节点的左右孩子，用于后面解压缩构建minihfmtree
	objfilesize += sizeof(int);//写入压缩文件头0xFFFFFFFF
	objfilesize += sizeof(int);//写入被压缩文件名长度
	objfilesize += strlen(soufilename);//写入被压缩文件的文件名
	objfilesize += sizeof(int);//写入被压缩文件的大小
	compressrate = (double)objfilesize / soufilesize;

	printf("Compress......\n\n");
	WriteCompressFile(pinf, poutf, hfmtree, hfmcode, soufilename, soufilesize);
	printf("Done!\n\n");
	printf("Before Compressed:\nFile Name:%s\nFile Size:%d bytes\n\n", soufilename, soufilesize);
	printf("After Compressed:\nFile Name:%s\nFile Size:%d bytes\n\n", objfilename, objfilesize);
	printf("Compress Rate:%.2f%%\n\n", 100 * compressrate);
	fclose(pinf);
	fclose(poutf);
	for (int i = 0; i < 256; i++)
		free(hfmcode[i].code);
	return true;
}

void CreateMiniHfmtree(FILE* pinf, short minihfmtree[][2])
{
	for (int i = 0; i < 256; i++)
		minihfmtree[i][0] = minihfmtree[i][1] = -1;
	fread(minihfmtree[256], sizeof(short), 2 * 255, pinf);
}

void WriteDecompressFile(FILE *pinf, FILE *poutf, short minihfmtree[][2], int objfilesize)
{
	unsigned char readbuf[256], writebuf[256];
	int readcounter, writecounter;
	int cursize, curpos;
	unsigned char convertbit;

	fseek(pinf, 0L, SEEK_CUR);
	fseek(poutf, 0L, SEEK_SET);
	readcounter = 256;
	cursize = writecounter = 0;
	curpos = 256 * 2 - 2;

	while (cursize != objfilesize)
	{
		if (readcounter == 256)
		{
			fread(readbuf, 1, 256, pinf);
			readcounter = 0;
		}
		for (convertbit = 128; convertbit != 0; convertbit >>= 1)
		{
			curpos = ((readbuf[readcounter] & convertbit) == 0 ? minihfmtree[curpos][0] : minihfmtree[curpos][1]);
			if (curpos < 256)
			{
				writebuf[writecounter] = (unsigned char)curpos;
				if (++writecounter == 256)
				{
					fwrite(writebuf, 1, 256, poutf);
					writecounter = 0;
				}
				curpos = 256 * 2 - 2;
				if (++cursize == objfilesize)
					break;
			}
		}
		readcounter++;
	}
	fwrite(writebuf, 1, writecounter, poutf);
}

bool Decompress(char *soufilename)
{
	FILE *pinf, *poutf;
	int ziphead;
	char *objfilename;
	int objfilesize, filenamesize;
	short minihfmtree[2 * 256 - 1][2];

	if ((pinf = fopen(soufilename, "rb")) == NULL)
		return false;
	fread(&ziphead, sizeof(int), 1, pinf);
	if (ziphead != 0xFFFFFFFF)
		return false;
	fread(&filenamesize, sizeof(int), 1, pinf);
	if ((objfilename = (char*)malloc(sizeof(char)*filenamesize + 1)) == NULL)
		return false;
	fread(objfilename, sizeof(char), filenamesize, pinf);
	objfilename[filenamesize] = '\0';
	fread(&objfilesize, sizeof(int), 1, pinf);
	if ((poutf = fopen(objfilename, "wb")) == NULL)
		return false;

	CreateMiniHfmtree(pinf, minihfmtree);
	printf("Decompressing......\n\n");
	WriteDecompressFile(pinf, poutf, minihfmtree, objfilesize);
	printf("Done!\n\n");
	printf("After Decompressed:\nFile Name:%s\nFile Size:%d\n\n", objfilename, objfilesize);
	free(objfilename);
	fclose(pinf);
	fclose(poutf);
	return true;
}

