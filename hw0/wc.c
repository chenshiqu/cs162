#include <stdio.h>
int main(int argc, char *argv[]) 
{
	if(argc==1)
	{
		char ch[30];
		scanf("%s",ch);
		printf("%s",ch);
	}
	else{
		FILE* fp=fopen("main.c","r");
		char c;
		int c_number=0;
		int word=0;
		int row=0;
		while((c=fgetc(fp))!=EOF)
		{
			c_number++;
			if(c=='\n')
			{
				row++;
			}
		}
		fseek(fp,0L,SEEK_SET);
		char s[30];
		while(fscanf(fp,"%s",s)!=EOF)
		{
			word++;
		}	
		printf("%d %d %d",word,row,c_number);
	}
    return 0;
}
