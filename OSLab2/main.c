#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define COLOR_DEFAULT 0
#define COLOR_RED 1

#pragma pack(push)
#pragma pack(1)

struct  BPB
{
    unsigned short BPB_BytsPerSec;//每扇区字节数
    unsigned char BPB_SecPerClus;//每簇占用的扇区数
    unsigned short BPB_RsvdSecCnt;//Boot占用的扇区数
    unsigned char BPB_NumFATs;//FAT表的记录数
    unsigned short BPB_RootEntCnt;//最大根目录文件数
    unsigned short BPB_TotSec16;//每个FAT占用扇区数
    unsigned char BPB_Media;//媒体描述符
    unsigned short BPB_FATSz16;//每个FAT占用扇区数
    unsigned short BPB_SecPerTrk;//每个磁道扇区数
    unsigned short BPB_NumHeads;//磁头数
    unsigned int BPB_HiddSec;//隐藏扇区数
    unsigned int BPB_TotSec32;//如果BPB_TotSec16是0，则在这里记录
}bpb;

struct RootEntry{
	char DIR_Name[11];//文件名 8 字节，拓展名 3 字节
	unsigned char   DIR_Attr;//文件属性
	char reserve[10];//保留位
	unsigned short  DIR_WrtTime;//最后一次写入时间
	unsigned short  DIR_WrtDate;//最后一次写入日期
	unsigned short  DIR_FstClus;//开始簇号
	unsigned int  DIR_FileSize;//文件大小  
}root;
  
#pragma pack(pop)

struct Info{
    int type;//文件属性，0 是文件夹，1 是文件
    unsigned short FstClus;
    unsigned int FileSize;//文件大小
    char fileName[12];
    int sibling;
    int children;
};

struct Count{
    unsigned int numOfFile;
    unsigned int numOfDir;
};

void myprint(char*, int, int);

struct Info inf[10];
int numOfRoot = 0;//根目录文件个数
int num = 0;

void myinput(FILE* fat12);//读取用户输入
void setBPB(FILE* fat12, struct BPB* bpb_ptr);//初始化BPB
void setRoot(FILE* fat12, struct RootEntry* rootEntry_ptr);//打印根目录
void setChildren(FILE* fat12 , char* rootName, int clus, int flag);//打印子目录
void setTree(FILE* fat12, int root, int num);//创建树结构
void printRoot(int root, char* path);
int  getFATValue(FILE* fat12 , int num);
int  getTarget(char* order, char* path, char* fullpath);//type=0 打印 type=1 计数
void countDir(int index, struct Count* count);
void printRootCount();
void printCount(int i, int depth, char* path);
int findFile(char* target);
void printFile(FILE* fat12, int i);//打印文件内容
void myItoa(int n, char* str);//数字转为字符
void preprint(char* content, int color);//输出前处理，计算长度
 
int main()
{
    FILE *fat12;
    fat12 = fopen("a.img", "rb");// 打开映像文件
    
    struct BPB* bpb_ptr = &bpb;
    struct RootEntry* root_ptr = &root;

    setBPB(fat12, bpb_ptr);//初始化 BPB
    setRoot(fat12, root_ptr);
    setTree(fat12, 0, numOfRoot);

    myinput(fat12);//读取用户输入
    fclose(fat12);

    //for(int i=0;i<10;i++){
        //printf("inf[%d]: %s %d\n", i, inf[i].fileName, inf[i].FileSize);
    //}
}

void myinput(FILE* fat12){
    char order1[10];
    char order2[50];
    char order3[50];
    char temp[2]={0};
    while(1){
        preprint(">",COLOR_DEFAULT);
        scanf("%s", order1);
        //ls 开头的指令
        if(strcmp(order1,"ls")==0){
            char ch = getchar();
            //ls 指令，直接输出
            if(ch == '\n'){
                preprint("/:\n", COLOR_DEFAULT);
                //打印首行
                for(int i=0; i<numOfRoot; i++){
                    if(inf[i].type==1){
                        preprint(inf[i].fileName, COLOR_DEFAULT);
                        preprint("  ", COLOR_DEFAULT);
                    }else{
                        preprint(inf[i].fileName, COLOR_RED);
                        preprint("  ", COLOR_DEFAULT);
                    }
                }
                preprint("\n", COLOR_DEFAULT);
                //打印子目录
                for(int i=0; i<numOfRoot; i++){
                    char* path = (char*)malloc(1024);
                    if(inf[i].type==0){
                        printRoot(i, path);
                    }
                }
            }
            else//处理第二段指令
            {
                scanf("%s", order2);
                
                if(strcmp(order2, "-l")==0){
                    char ch = getchar();
                    //ls -l 指令
                    if(ch == '\n'){
                        printRootCount();
                    }
                    //ls -l /NJU/SUBNJU 指令
                    else
                    {
                        scanf("%s", order3);
                            
                        char* path1 = (char*)malloc(50*sizeof(char));
                        char* fullpath1 = (char*)malloc(50*sizeof(char));
                        int typeOfOrder = getTarget(order3, path1, fullpath1);
                        int result = findFile(path1);
                        if(typeOfOrder == -1){
                            preprint("WRONG PATH\n", COLOR_DEFAULT);
                        }else{
                            if (result >= 0 && result <= num) {
                                printCount(result, 0, fullpath1);
                            } else {
                                // 如果是文件，报错输出
                                preprint("WRONG DIRECTORY\n", COLOR_DEFAULT);
                            }
                        }                        
                    }
                }
                //ls /NJU 或 ls /NJU -l
                else if(order2[0] == '/')
                {
                    //ls /NJU
                    char* path2 = (char*)malloc(50*sizeof(char));
                    char* fullpath2 = (char*)malloc(50*sizeof(char));                    
                    int typeOfOrder = getTarget(order2, path2, fullpath2);
                    int result = findFile(path2);

                    //错误情况 ls /*.TXT
                    if(typeOfOrder == -1){
                        preprint("WRONG PATH\n", COLOR_DEFAULT);
                    }else{
                        char ch = getchar();
                        if(ch == '\n'){
                            if(result == -1){
                                preprint("WRONG PATH\n", COLOR_DEFAULT);
                            }else{
                                printRoot(result, fullpath2);
                            } 
                        }
                        //ls /NJU -l
                        else
                        {
                            scanf("%s", order3);
                            if (result >= 0 && result <= num) {
                                printCount(result, 0, fullpath2);
                            } else {
                                // 如果是文件，报错输出
                                preprint("WRONG DIRECTORY\n", COLOR_DEFAULT);
                            }
                        }
                    }
                }
                //错误指令
                else{
                    preprint("WRONG ORDER\n", COLOR_DEFAULT);
                }
            }
        }//cat 指令
        else if(strcmp(order1,"cat")==0){
            char ch = getchar();
            if(ch != '\n'){
                scanf("%s", order2);
                int isStart = 0;
                for(int i=0; i<strlen(order2); i++){
                    if(order2[i] == '/') isStart = i+1;
                }
                char *file = order2 + isStart;
                int result = findFile(file);
                if (result == 0) {
                    preprint("WRONG FILENAME\n", COLOR_DEFAULT);
                } else if (inf[result].type == 1) {
                    printFile(fat12, result);
                } else {
                    preprint("NOT FILE\n", COLOR_DEFAULT);
                }
            }
        }else if(strcmp(order1,"exit")==0){
            preprint("FINISH\n", COLOR_DEFAULT);
            return;
        }else{
            preprint("WRONG ORDER\n", COLOR_DEFAULT);
        }
    }
}

//初始化 BPB
void setBPB(FILE* fat12 , struct BPB* bpb_ptr){
    //查找开头，bpb 偏移 11 个字节开始
    fseek(fat12,11,SEEK_SET);
    //读取数据，字节长度为 25
    fread(bpb_ptr,1,25,fat12);
}

//初始化目录
void setRoot(FILE* fat12, struct RootEntry* root_ptr){
    //根目录区起始地址 = (隐藏扇区数(0） + 保留扇区数(Boot Sector) + FAT表数量 × FAT表大小) * 每扇区字节数
    int root_start = (bpb.BPB_RsvdSecCnt + bpb.BPB_NumFATs * bpb.BPB_FATSz16) * bpb.BPB_BytsPerSec;  
    for(int i=0;i<bpb.BPB_RootEntCnt;i++){
        fseek(fat12, root_start, SEEK_SET);
        fread(root_ptr, 1, 32, fat12);
        root_start += 32;

        //过滤乱码文件和空文件
        if(root_ptr->DIR_Name[0]=='\0') continue;
		int isValid = 0;
        //非英文大写、数字、空格
		for (int j=0; j<11; j++) {
			if (!(((root_ptr->DIR_Name[j] >= '0')&&(root_ptr->DIR_Name[j] <= '9')) ||
				((root_ptr->DIR_Name[j] >= 'A')&&(root_ptr->DIR_Name[j] <= 'Z')) ||
					(root_ptr->DIR_Name[j] == ' '))) {
				isValid = 1;
				break;
			}
		}
		if (isValid == 1) continue;

		if ((root_ptr->DIR_Attr&0x10) == 0 ) {
			//文件
            if(num!=0){
                inf[num-1].sibling = num;
            }
            inf[num].FstClus = root_ptr->DIR_FstClus;
            inf[num].FileSize = root_ptr->DIR_FileSize;
            inf[num].type = 1;
			int k=0;
			for (int j=0;j<11;j++) {
				if (root_ptr->DIR_Name[j] != ' ') {
					inf[num].fileName[k] = root_ptr->DIR_Name[j];
                    k++;
				} else {
					inf[num].fileName[k] = '.';
                    k++;
					while (root_ptr->DIR_Name[j] == ' ') j++;
					j--;
				}
			}
			inf[num].fileName[k] = '\0';
            num++;
		} else {
			//文件夹
            if(num!=0){
                inf[num-1].sibling = num;
            }
            inf[num].FstClus = root_ptr->DIR_FstClus;
            inf[num].type = 0;
            int k=0;
			for (int j=0;j<8;j++) {
				if (root_ptr->DIR_Name[j] != ' ') {
					inf[num].fileName[k] = root_ptr->DIR_Name[j];
                    k++;
				} else {
					inf[num].fileName[k] = '\0';
					break;
				}
			}
            num++;
		}
 
    }
    numOfRoot = num;
}

//设置子目录，创建树结构
void setChildren(FILE* fat12, char* rootName, int clus, int flag){
    char path[50];
    int data_start = bpb.BPB_BytsPerSec * ( bpb.BPB_RsvdSecCnt + bpb.BPB_FATSz16*bpb.BPB_NumFATs + (bpb.BPB_RootEntCnt*32 + bpb.BPB_BytsPerSec - 1)/bpb.BPB_BytsPerSec );
    int length = strlen(rootName);
	strcpy(path, rootName);

    int currentClus = clus;
	int value = 0;
	while (value < 0xFF8) {
	    value = getFATValue(fat12,clus);
		if (value == 0xFF7) {
			preprint("BAD CLUS\n", COLOR_DEFAULT);
			break;
        }

		char* content = (char* )malloc(bpb.BPB_SecPerClus * bpb.BPB_BytsPerSec);	//暂存从簇中读出的数据
		
		int offset = data_start + (clus - 2)* bpb.BPB_SecPerClus * bpb.BPB_BytsPerSec;
		fseek(fat12, offset, SEEK_SET);
		fread(content, 1, bpb.BPB_SecPerClus * bpb.BPB_BytsPerSec, fat12);

		int loop = 0;
		while (loop < bpb.BPB_SecPerClus * bpb.BPB_BytsPerSec) {        
			if (content[loop] == '\0') {
				loop += 32;
				continue;
			}
			int isValid = 0;
			for (int j=loop;j<loop+11;j++) {
				if (!(((content[j] >= '0')&&(content[j] <= '9')) ||
				        ((content[j] >= 'A')&&(content[j] <= 'Z')) ||
					    (content[j] == ' '))) {
                    isValid=1;
                    break;
				}	
			}
			if (isValid == 1) {
				loop += 32;
				continue;
			}

            //文件夹
            if((content[loop+11] & 0x10) != 0){
                inf[num].FstClus=content[loop+0x1A];
                inf[num].type = 0;
                int k=0;
			    for (int j=0;j<8;j++) {
				    if (content[loop+j] != ' ') {
					    inf[num].fileName[k] = content[loop+j];
                        k++;
				    } else {
					    inf[num].fileName[k] = '\0';
					    break;
				    }
			    }
                num++;
            }else{
                inf[num].FstClus=content[loop+0x1A];
                int filesize = 0;
                fseek(fat12, offset+loop+0x1C, SEEK_SET);
                fread(&filesize, 1, 4, fat12);
                inf[num].FileSize=filesize;
                inf[num].type = 1;
			    int k=0;
			    for (int j=0;j<11;j++) {
				    if (content[loop+j] != ' ') {
					    inf[num].fileName[k] = content[loop+j];
                        k++;
				    } else {
					    inf[num].fileName[k] = '.';
                        k++;
					    while (content[loop+j] == ' ') j++;
					    j--;
				    }
			    }
			    inf[num].fileName[k] = '\0';
                num++;
            }

            inf[num-1].sibling = 0;
            inf[num-1].children = 0;
            if (inf[flag].children == 0) {
                inf[flag].children = num-1;
            }else{
                //否则作为排列最后的子目录
                int temp = inf[flag].children;
                while (inf[temp].sibling != 0) {
                    temp = inf[temp].sibling;
                }
                inf[temp].sibling = num-1;
            }

            //文件夹
            if(inf[num-1].type==0){
                loop = loop + 26;
                setChildren(fat12, inf[num-1].fileName, content[loop], num-1);
                loop = loop - 26;          
            } 
			loop += 32;
		} 
 
		clus = value;
	};
}

void setTree(FILE* fat12, int root, int number){
    for(int i=root; i<root+number; i++){
        if(inf[i].type==0){
            setChildren(fat12, inf[i].fileName, inf[i].FstClus, i);           
        }
    }
}

void printRoot(int root, char* path){
    int children = inf[root].children;
    strcat(path, "/");
    strcat(path, inf[root].fileName);  
    preprint(path, COLOR_DEFAULT);
    preprint("/:\n", COLOR_DEFAULT);
    preprint(".  ..  ", COLOR_RED);
    
    while (children != 0) {
        if(inf[children].type == 0) {
            preprint(inf[children].fileName, COLOR_RED);
        }
        else{
            preprint(inf[children].fileName, COLOR_DEFAULT);
        }
        preprint("  ", COLOR_DEFAULT);

        children = inf[children].sibling;       
    }
    preprint("\n", COLOR_DEFAULT);

    children = inf[root].children;
    while (children != 0) {
        char* new_path = (char*)malloc(strlen(path) + strlen(inf[children].fileName) + 2);
        strcpy(new_path, path);
        if (inf[children].type == 0) {
            printRoot(children, new_path);
        } 
        free(new_path);
        children = inf[children].sibling; 
    }
}


int  getFATValue(FILE* fat12 , int clus) {
	int offset = bpb.BPB_RsvdSecCnt * bpb.BPB_BytsPerSec + clus*3/2;
 
	unsigned short byte;
	fseek(fat12, offset, SEEK_SET);
	fread(&byte, 1, 2, fat12);
 
	if (clus%2 == 0) {
        byte = byte << 4;
		return byte>>4;
	} else {
		return byte>>4;
	}
}

//处理带路径的指令
int getTarget(char* order, char* path, char* fullpath){
    int isCount = 0;
    int isFst = 0;
    int i = 0;
    int startOfPath = 0;
    int startOfFullpath = 0;
    int lenOfPath = 0;
    int lenOfFullpath = 0;
    int number = 0;
    char ch = order[i];
    while(ch!='\0'){
        if(ch == '.'){
            isCount = -1;
        }

        if(ch == '/'){
            number++;
            startOfPath = i;
            if(isFst == 0){
                startOfFullpath = i;
                isFst = 1;
            }
            lenOfPath = 0;
        }

        lenOfPath++;
        lenOfFullpath++;
        
        i++;
        ch = order[i];
    }
    if(isCount == -1){
        return -1;
    }else{
        memset(path, '\0', sizeof(path));
        strncpy(path, order+startOfPath+1, lenOfPath-1);
        strcat(path, "\0");
        if(number!=1){
            int p = strlen(path);
            memset(fullpath, '\0', sizeof(fullpath));
            strncpy(fullpath, order+startOfFullpath+1, lenOfFullpath-p-2);
            strcat(fullpath, "\0");
        }   
        return 0;
    }
}

//计算数量
void countDir(int index, struct Count* count){
    int flag = inf[index].children;
    while (flag != 0) {
        //文件夹
        if (inf[flag].type == 0) {
            count->numOfDir++;
        }else if (inf[flag].type == 1) {
            count->numOfFile++;
        }
        flag = inf[flag].sibling;
    }
}

void printRootCount(){
    int rootFile = 0;
    int rootDir = 0;
    char* ch_rootFile = (char*)malloc(3);
    char* ch_rootDir = (char*)malloc(3);
    for(int i=0; i<numOfRoot; i++){
        if(inf[i].type == 0){
            rootDir++;
        }else{
            rootFile++;
        }
    }
    myItoa(rootFile, ch_rootFile);
    myItoa(rootDir, ch_rootDir);
    preprint("/ ", COLOR_DEFAULT);
    preprint(ch_rootDir, COLOR_DEFAULT);
    preprint(" ", COLOR_DEFAULT);
    preprint(ch_rootFile, COLOR_DEFAULT);
    preprint(":\n", COLOR_DEFAULT);
    free(ch_rootFile);
    free(ch_rootDir);

    struct Count* count = (struct Count*)malloc(sizeof(struct Count)); 
    char* file = (char*)malloc(5);
    char* dir = (char*)malloc(5);
    for(int i=0; i<numOfRoot; i++){
        if(inf[i].type == 0){
            count->numOfDir = 0;
            count->numOfFile = 0;
            myItoa(0, dir);
            myItoa(0, file);
            countDir(i, count);
            myItoa(count->numOfDir, dir);  
            myItoa(count->numOfFile, file);

            preprint(inf[i].fileName, COLOR_RED);
            preprint("  ", COLOR_DEFAULT);
            preprint(dir, COLOR_DEFAULT);
            preprint(" ", COLOR_DEFAULT);
            preprint(file, COLOR_DEFAULT);
            preprint("\n", COLOR_DEFAULT);
        }else{
            preprint(inf[i].fileName, COLOR_DEFAULT);
            preprint("  ", COLOR_DEFAULT);
            char* size = (char*)malloc(3);
            myItoa(inf[i].FileSize, size);
            preprint(size, COLOR_DEFAULT);
            preprint("\n", COLOR_DEFAULT);
        }
    }
    preprint("\n", COLOR_DEFAULT);

    for(int i=0; i<numOfRoot; i++){
        if(inf[i].type == 0){
            char* path = (char*)malloc(1024);
            printCount(i, 0, path);
        }
    }
}

void printCount(int root, int depth, char* path){
    struct Count* count = (struct Count*)malloc(sizeof(struct Count)); 
    char* file = (char*)malloc(5);
    char* dir = (char*)malloc(3);
    char* size = (char*)malloc(3);
    
    //文件夹，计算数量
    if(inf[root].type == 0){
        count->numOfFile = 0;
        count->numOfDir = 0;
        countDir(root, count);//计算出总共的文件和文件夹数目
        myItoa(count->numOfDir, dir);  
        myItoa(count->numOfFile, file);
    } else{
        //文件，计算大小
        myItoa(inf[root].FileSize, size);
    }
    
    if(inf[root].type == 0){
        strcat(path, "/");
        strcat(path, inf[root].fileName);  
        preprint(path, COLOR_DEFAULT);
        preprint("/ ", COLOR_DEFAULT);
        preprint(dir, COLOR_DEFAULT);
        preprint(" ", COLOR_DEFAULT);
        preprint(file, COLOR_DEFAULT);
        preprint(":\n", COLOR_DEFAULT);
        preprint(".\n", COLOR_RED);
        preprint("..\n", COLOR_RED);
    }
    
    int children = inf[root].children;  
    while (children != 0) {
        //重定位count
        count->numOfDir = 0;
        count->numOfFile = 0;
        countDir(children, count);
        myItoa(count->numOfDir, dir);  
        myItoa(count->numOfFile, file);
        myItoa(inf[children].FileSize, size);
        if(inf[children].type == 1) {
            preprint(inf[children].fileName, COLOR_DEFAULT);
            preprint("  ", COLOR_DEFAULT);
            preprint(size, COLOR_DEFAULT);
        }
        else{
            preprint(inf[children].fileName, COLOR_RED);
            preprint("  ", COLOR_DEFAULT);
            preprint(dir, COLOR_DEFAULT);
            preprint(" ", COLOR_DEFAULT);
            preprint(file, COLOR_DEFAULT);
        }
        preprint("\n", COLOR_DEFAULT);
        children = inf[children].sibling;       
    }
    preprint("\n", COLOR_DEFAULT);

    children = inf[root].children;
    while (children != 0) {
        char* new_path = (char*)malloc(strlen(path) + strlen(inf[children].fileName) + 2);
        strcpy(new_path, path);
        if (inf[children].type == 0) {
            printCount(children, depth-1, new_path);
        } 
        free(new_path);
        children = inf[children].sibling; 
    }
}

//找到文件inf 中对应数
int findFile(char* target)
{
    int result = -1;
    for(int i=0; i<num; i++){
        if(strcmp(inf[i].fileName, target) == 0){
            result = i;
            break;
        }
    }
    return result;
}

void printFile(FILE* fat12, int index) {
    unsigned short clus = inf[index].FstClus;
    char temp[512];
    while(clus < 0xFF8){
		if(clus == 0xFF7){
			preprint("BAD CLUS\n", COLOR_DEFAULT);
			break;
		}
        int data_start = (bpb.BPB_RsvdSecCnt + bpb.BPB_FATSz16 * bpb.BPB_NumFATs) * bpb.BPB_BytsPerSec
                + bpb.BPB_RootEntCnt * 32 + (clus - 2) * bpb.BPB_SecPerClus * bpb.BPB_BytsPerSec;
		fseek(fat12, data_start, SEEK_SET);
		fread(temp, 512, 1, fat12);
		preprint(temp, COLOR_DEFAULT);
		clus = getFATValue(fat12,clus);
	}
    preprint("\n", COLOR_DEFAULT);
}

//void myprint(char* content, int length, int color){
    //if(color){
        //printf("\x1b[31m%s\x1b[0m", content);
    //}else{
        //printf("%s", content);
    //}
//}

void myItoa(int n, char* str){
    if(n==0){
        strcpy(str, "0");
    }else{
        int i = 0, j = 0;
	    char* temp = (char*)malloc(10);
	    while(n){
		    temp[i] = n % 10 + '0';
		    n /= 10;
		    i++;
	    }
        temp[i]='\0';
        i--;
	    while (i >= 0)
	    {
		    str[j] = temp[i];
            j++;
            i--;	
	    }
	    str[j] = '\0';
        free(temp);
    }   
}

void preprint(char* content, int color){
    int len = 0;
    while(content[len]!='\0'){
        len++;
    }
    myprint(content, len, color);
}
