#include<sys/types.h>
#include<dirent.h>
#include<unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "des_min_inc.h"
#include<fcntl.h>
#include "log/log.h"
#include "link/link.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* output config file type*/
typedef enum{
	APPENDFILE,/*append*/
	TRUCKFILE, /*truck*/
}FILETYPE;
/*pvid list*/
LINKLIST VPidList;
/*pvid list content*/
typedef struct __VPIDSTR{
	char key[10];/*keyword of pvid*/
	int num;     /*repeate num*/
}VPIDSTR;
/*every pvid element struct*/
struct PVidStr{
	char dtpvid[4][10];/*default pid vid and target pid vid string*/
	int pos[4];        /*contain pvid four string's flag*/
	int num;          /*pvid num*/
	char keyline[1024];/*key word generated by pvid and the repeated num*/

};
/*every pvid element line buffer*/
char linebuf[50][150];
/*line bufer current used index*/
int lineinx =0;

VPIDSTR * searchVPidList(LINKLIST *list, char *key);
LINK * addAVPidToList(LINKLIST *list, char *key);


void outputPvidStartToFile(FILE *fp, struct PVidStr *pvid)
{
	
	
	fprintf(fp, "[start_%s]\n", pvid->keyline);

	int pos = 0;
	if (pvid->pos[pos])
		fprintf(fp, "%s = 0x%s\n", "DefaultVendor", pvid->dtpvid[pos]);
	pos++;

	if (pvid->pos[pos])
		fprintf(fp, "%s = 0x%s\n", "DefaultProduct", pvid->dtpvid[pos]);
	pos++;

	if (pvid->pos[pos])
		fprintf(fp, "%s = 0x%s\n", "TargetVendor", pvid->dtpvid[pos]);
	pos++;
	if (pvid->pos[pos])
		fprintf(fp, "%s = 0x%s\n", "TargetProduct", pvid->dtpvid[pos]);
	pos++;
}

void outputPvidContentToFile(FILE *fp, char *line)
{
	
	if(strchr(line, '\n'))
		fprintf(fp, "%s", line);
	else
		fprintf(fp, "%s\n", line);
}
void outputPvidEndToFile(FILE *fp, struct PVidStr *pvid)
{
		fprintf(fp, "[end_%s]\n", pvid->keyline);
}
int setPvidKeyline(struct PVidStr *pvid, int number)
{
	if (pvid->num > 1)
	{
		
		if(pvid->pos[0] && pvid->pos[1])
		{
			sprintf(pvid->keyline, "%s_%s", pvid->dtpvid[0], pvid->dtpvid[1]);
		}else
		{
			return -1;
		}
	}else 
	{
		return -1;
	}
	VPIDSTR * res = searchVPidList(&VPidList, pvid->keyline);
	int num = 0;
	if(res == NULL)
	{
		addAVPidToList(&VPidList, pvid->keyline);
	}else
	{	
		res->num ++;
		num =res->num;
	}
	sprintf(&pvid->keyline[strlen(pvid->keyline)], "_%d", number);
	if(pvid->num == 1 || pvid->num == 3 )
	{
	}
	return num;
}
void getPvid(struct PVidStr *pvid,char *pline, int n)
{
        pline = strchr(pline, 'x');
	if (pline == NULL) 
	{
		myLog(WARN, "get pid vid fail.");
		return;
	}
	snprintf(pvid->dtpvid[n], 5, "%s", pline+1);
	pvid->pos[n] = 1;
	pvid->num ++;
}

LINK * addAVPidToList(LINKLIST *list, char *key)
{
	VPIDSTR *newMsg = malloc(sizeof(VPIDSTR));
	if(newMsg == NULL){
		 return NULL;
	}
	snprintf(newMsg->key, 10, "%s" , key);	
	newMsg->num = 0;
	LINK *element = LINK_new(list,newMsg);
	LINKLIST_insert(list, element);
	return element;

}
VPIDSTR * searchVPidList(LINKLIST *list, char *key)
{
	
	LINKLIST_Start(list);

	while (hasNextLinkElement(list)){
		VPIDSTR *pvid = (VPIDSTR *)((getNextLinkElement(list))->content);
		if(strncmp(pvid->key, key, 9) == 0)
		{
			return pvid;
			break;
		}	
		
	}
	return NULL;
}
void merge_config_from_file(char *inputfile, char * outputfile, FILETYPE type)
{
	FILE *fpin;
	fpin = fopen(inputfile, "rb+");
	
	if(fpin == NULL)
	{
		myLog(ERROR, "%s @%s", "open file fail.", inputfile);
		return;		
	}	
	
	char line[1024];
	struct PVidStr pvid;
	memset(&pvid, 0 ,sizeof(pvid));
	int status = 0;	
	int overFlag = 0;
	myLog(INFO, "deal with file :%s", inputfile);
	char lastline[200];
	int fileinx =0;
	while(1)
	{
		lineinx = 0;
		status = 0;
		memset(&pvid, 0 ,sizeof(pvid));
		while(1)
		{
			if(overFlag){
				strcpy(line, lastline);
				overFlag = 0;
			}else	
			if (fgets(line,1024,fpin) <= 0)
			{
				break;	
			}

			char *pline;
			if ((line[0] == '#' ||line[0]=='[')&& status == 0)
			{
				if(strncmp(line, "[start",6) == 0){
					fileinx = atoi(&line[17]);
				}

				continue;	
			}
			if (strncmp(line,"[end",4)==0)
			{
				
				break; 
			}

			if (strncmp(line, "######", 6) == 0 && status == 1)
			{
				strcpy(lastline, line);
				overFlag = 1;
				break;	
			}
			pline = line;
			if (line[0] == ';')
			{
				pline++;	
			}
				
			while(*pline == ' ' || *pline == '\t') pline++;
			if (strlen(pline) > 2){
				if(strncmp(pline, "DefaultVendor", 8) == 0)
				{
					if(status == 0)
					{
						status = 1;	
						memset(&pvid, 0 ,sizeof(pvid));
						
						getPvid(&pvid, pline, 0);
					}else
					{
						strcpy( lastline, pline);
						overFlag = 1;
						break;
					}
				
				}else
				if(strncmp(pline, "DefaultProduct", 8) == 0)
				{
					getPvid(&pvid, pline, 1);
				}else
				if(strncmp(pline, "TargetVendor", 7) == 0)
				{
					getPvid(&pvid, pline, 2);
				}else
				if(strncmp(pline, "TargetProduct", 7) == 0 && pline[13] != 'L' )
				{
					getPvid(&pvid, pline, 3);
				}else 
				{
					strcpy(linebuf[lineinx], pline);
					lineinx ++;
					if(lineinx >= 50 )
					{
						myLog(ERROR, "no enough buffer in line buf .");
						status = 0;
						break;
					}
				}
			
			}
		}	

		if(status == 1){
			status = 2;
			int res = setPvidKeyline(&pvid, fileinx);
			
			if (res == -1)
			{

				myLog(WARN, "%s,@file %s", "no defalut vid and pid pair.", inputfile);
				continue;
			}
			res = fileinx;
			char outsubfile[1024];
			char bufnumtmp[10];
			char bufnum[10];
			

			int tmp = res;
			int j =0; 
	 		int ii=0;
			do{
				int t=0;
				t = tmp %10;
				bufnumtmp[ii++] = t + '0';
				tmp /=10;
			}
			while(tmp>0);

			bufnumtmp[ii] =  '\0';
			j = 0;
			do{
			 	int t ;

				t =bufnumtmp[ii-j -1];
				bufnumtmp[ii-j -1 ]=bufnumtmp[j] ;
				bufnumtmp[j]=t ;
				j++;
			}
			while(j< ii/2);


			ii=0;
			j=0;
			int len =strlen(bufnumtmp);
			do{
				if(len == 1){
	
					bufnum[j++]=bufnumtmp[ii];
					break;	
				}
				bufnum[j++]='A';
				bufnum[j++]=bufnumtmp[ii];
			
				ii++;
			}
			while(ii<len);
		


			bufnum[j]='\0';
			sprintf(outsubfile,"tmp/%s_%s_%s", pvid.dtpvid[0], pvid.dtpvid[1], bufnum);
			FILE *fpsubout = fopen(outsubfile,"wb");
		
				
			outputPvidStartToFile(fpsubout, &pvid);
			int i = 0;
			while(i < lineinx)
			{
				outputPvidContentToFile(fpsubout, linebuf[i]);		
				i++;
			}
			outputPvidEndToFile(fpsubout, &pvid);
			fclose(fpsubout);
		}else
		{
			break;
		}
	}
	fclose(fpin);
}

void getOutputFile(char *d);
int mkdir(char *);
void merge_config_from_dir(char *dirName, char * outputfile)
{
	
	DIR * dir;
	struct dirent * ptr;

	system("rm -rf ./tmp");	
	mkdir("tmp");
	system("chmod 777 tmp");
	dir =opendir(dirName);
	while((ptr = readdir(dir))!=NULL)
	{
		if(ptr->d_name[0] != '.'){
			char inputfile[1024];
			sprintf(inputfile, "%s/%s", dirName, ptr->d_name);
			merge_config_from_file(inputfile, outputfile, APPENDFILE);		
		}
	}

	closedir(dir);
	getOutputFile(outputfile);
}
void getOutputFile(char *outputfile){
	system("ls ./tmp >tmp.lst");
	FILE *fpout;
	if (access(outputfile, 0) == -1){
		fpout = fopen(outputfile, "wb+");
	}else
	{
		fpout = fopen(outputfile, "wb+");
	}
	
	if(fpout == NULL)
	{
		myLog(ERROR, "%s @%s", "open file fail.", outputfile);
		return;		
	}
	FILE *fp =fopen("tmp.lst","rb");
	char line[100];
	while(fgets(line,100,fp))
	{

			line[strlen(line) - 1]=0;
//			myLog(INFO, "d_name: %s", line);

			char f[100];
			sprintf(f,"./tmp/%s", line);
			
			FILE *fp2 =fopen(f,"rb");
			if(fp2== NULL){
			
				printf("open file fail.%s",f);
				return ;
			}
			
			char line2[100];
			int flag =0;
			while(fgets(line2,100,fp2))
			{
				
				if (
					strstr(line2, "-H")
					||strstr(line2, "-S")
					||strstr(line2, "-O")
					||strstr(line2, "-G")
					||strstr(line2, "-N")
					||strstr(line2, "-A")
					||strstr(line2, "-T")
					||strstr(line2, "-L")
				
					){
					flag =1;

				}
				if(flag){
				}else
				if(strstr(line2, "HuaweiMode")){
					char *p = "MessageContent = \"-H\"\n";
//					printf("find %s\n ",line2);
					fwrite(p,strlen(p),1,fpout);
				}else
				if(strstr(line2, "SierraMode")){
					char *p = "MessageContent = \"-S\"\n";
//					printf("find %s\n ",line2);
					fwrite(p,strlen(p),1,fpout);
				}else
				if(strstr(line2, "SonyMode")){
					char *p = "MessageContent = \"-O\"\n";
//					printf("find %s\n ",line2);
					fwrite(p,strlen(p),1,fpout);
				}
				else
				if(strstr(line2, "GctMode")){
					char *p = "MessageContent = \"-G\"\n";
//					printf("find %s\n ",line2);
					fwrite(p,strlen(p),1,fpout);
				}
				else
				if(strstr(line2, "KobilMode")){
					char *p = "MessageContent = \"-T\"\n";
//					printf("find %s\n ",line2);
					fwrite(p,strlen(p),1,fpout);
				}
				
				else
				if(strstr(line2, "SequansMode")){
					char *p = "MessageContent = \"-N\"\n";
//					printf("find %s\n ",line2);
					fwrite(p,strlen(p),1,fpout);
				}
				else
				if(strstr(line2, "MobileactionMode")){
					char *p = "MessageContent = \"-A\"\n";
//					printf("find %s\n ",line2);
					fwrite(p,strlen(p),1,fpout);
				}
				else
				if(strstr(line2, "CiscoMode")){
					char *p = "MessageContent = \"-L\"\n";
//					printf("find %s\n ",line2);
					fwrite(p,strlen(p),1,fpout);
				}
				fwrite(line2,strlen(line2),1,fpout);
			}

			fclose(fp2);
	}
	fclose(fp);
	fclose(fpout);
}
char * programname = NULL;
void usAge()
{

	printf("usAge: %s [-e/-d] -f cfgfile -o outputCfgfile\n", programname);
}

static unsigned char l_cDesKey[8] 	= {0x47, 0x8D, 0xA5, 0x0B, 0xF9, 0xE3, 0xD2, 0xCF};

void enCodeFile(char *in, char *out, int desType)
{
	FILE *fpin,*fpout;

	fpin = fopen(in,"rb");
	if(fpin == NULL) 
	{
		printf("open file fail.@%s\n",in);
		return;
	}
	
	fseek(fpin, 0 ,SEEK_END);
	long fileLen = ftell(fpin);
	fclose(fpin);

	printf("fileLen is %ld\n",fileLen);
	
	fpout = fopen(out,"wb");
	if(fpout == NULL) 
	{
		printf("open file fail.@%s\n",out);
		fclose(fpin);
		return ;
	}
	#define READ_BUF (200*1024)
	char line[READ_BUF];
	char buf[READ_BUF];
	int nRead = 0;
	int nEnCode = 0;
	
	int  fp = open(in, O_RDONLY);
	long nCurRead = 0;
	
	while(nCurRead < fileLen)
	{
		nRead = read(fp, line, READ_BUF);
 		printf("nRead is %d\n",nRead);
 		if(nRead > 0){
 			nEnCode = des_min_do(&line, nRead, &buf, READ_BUF, l_cDesKey, desType);
			printf("nEncode is %d\n",nEnCode);
			fwrite(buf,nEnCode, 1, fpout);	
		}
		nCurRead += nRead;
		
	}
	close(fp);
	fclose(fpout);


}
int main(int argc, char ** argv)
{
	char outputFile[1024]="default.conf";
	char outputFileTmp[1024]="default.conf";
	char inputFile[1024];
	int ch;
	int desType = 0;
	programname = argv[0];
	opterr = 0;
	int type = 0;
	while((ch = getopt(argc, argv, "f:o:deh")) != -1)
	{
		switch(ch)
		{
			case 'e':
				desType = 1;
				break;
			case 'd':
				desType = 0;
				break;
			case 'f':
				strcpy(inputFile, optarg);
				type += 1;
			break;
			case 'o':
				strcpy(outputFile, optarg);
				strcpy(outputFileTmp, optarg);
				strcat(outputFileTmp, ".tmp");
			break;
			case 'h':
				usAge();
				return 0;	
			default :
			break;
		}
	}
	if (type == 0) {	
		usAge();
	}

	
	if (desType == 1)
	{	
		if (type == 1) 
		{	
			LINKLIST_init(&VPidList);
			system("rm -rf ./tmp");	
			mkdir("tmp");
			system("chmod 777 tmp");
			merge_config_from_file(inputFile, outputFileTmp, TRUCKFILE);
			getOutputFile(outputFileTmp);
//			myLog(INFO,"done.");
			LINKLIST_desLink(&VPidList);
		}
		enCodeFile(outputFileTmp, outputFile, desType);
		system("rm -rf ./tmp");	
	}
	else
	{
		enCodeFile(inputFile, outputFile, desType);
	}
	
	
	remove(outputFileTmp);
	return 0;
}