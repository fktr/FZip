#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef struct hfmnode
{
	int weight;
	short parent;
	short lchild;
	short rchild;
}HfmNode, *pHfmNode;
typedef struct hfmcode
{
	unsigned char len;
	unsigned char *code;
}HfmCode, *pHfmCode;

inline int Smaller(int a, int b) { return a < b ? a : b; }
inline void Clear() { while (getchar() != '\n'); }

bool InitialFiles(char *soufilename, FILE* &pinf, char *objfilename, FILE* &poutf);
int DataFrequency(FILE *pinf, int frequency[]);
void FindTwoMin(HfmNode hfmtree[], int n, int &min1, int &min2);
void CreateHfmtree(HfmNode hfmtree[], int frequency[]);
bool EncodeHfmtree(HfmNode hfmtree[], HfmCode hfmcode[]);
unsigned char charstobits(unsigned char chars[]);
void WriteCompressFile(FILE *pinf, FILE *poutf, HfmNode hfmtree[], HfmCode hfmcode[], char *soufilename, int soufilesize);
bool Compress(char *soufilename, char *objfilename);
void CreateMiniHfmtree(FILE* pinf, short minihfmtree[][2]);
void WriteDecompressFile(FILE *pinf, FILE *poutf, short minihfmtree[][2], int objfilesize);
bool Decompress(char *soufilename);
