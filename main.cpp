#include"zip.h"

int main()
{
	freopen("result.txt", "w", stdout);

	char soufilename[6][256] =
	{
		"novel.txt","novel.pdf","novel.mobi",
		"����¶�.txt","����¶�.pdf","����¶�.mobi"
	};
	char objfilename[6][256] =
	{
		"txteng.zip","pdfeng.zip","mobieng.zip",
		"txtch.zip","pdfch.zip","mobich.zip"
	};

	for (int i = 0; i < 6; i++)
		Compress(soufilename[i], objfilename[i]);

	return 0;
}