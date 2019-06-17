#include <iostream>
#include <string.h>
#include <stdlib.h>

using namespace std;

//磁盘总大小为64*512
#define B        512       			//定义存储块的长度B字节
#define L        64        			//定义磁盘的存储块总数L，逻辑号0-(L-1)
#define K        3      			//磁盘的前k 个块是保留区

#define OK       1     				//操作成功
#define ERROR    -1    				//操作错误

#define File_Block_Length    (B-3)  //定义文件磁盘块号数组长度
#define File_Name_Length     (B-1)  //定义文件名的长度

//8*8共表示64个逻辑块，即磁盘的总块数：L
#define map_row_num     8           //位图的行数
#define map_cow_num     8           //位图的列数

#define maxDirectoryNumber  49      //定义目录中目录项（文件数量）的最大值

#define Buffer_Length 64            //定义读写缓冲区的大小，用于打开文件表项

typedef struct FileDescriptor//文件描述符
{
	int fileLength;                                        //文件长度
	int file_allocation_blocknumber[File_Block_Length];    //文件分配到的磁盘块号数组
	int file_block_length;                                 //文件分配到的磁盘块号数组实际长度
	int beginpos;                                          //文件的起始坐标位置beginpos=map_cow_num*row + cow
	int rwpointer;                                         //读写指针：初始化为0，随着RWBuffer变化
	char RWBuffer[Buffer_Length];                          //读写缓冲区，存文件的内容
}FileDescriptor;

typedef struct Directory
{
	int  index;                     						//文件描述符序号
	int  count;                     						//用于目录中记录文件的数量
	char fileName[File_Name_Length];						//文件名
	int  isFileFlag;                						//判断是文件和是目录，1为文件，0为目录
	int  isOpenFlag;                						//判断文件是否打开，1为打开，0为关闭
	FileDescriptor fileDescriptor;  						//文件描述符信息，与index 对应
}Directory;


char ldisk[L][B];											//构建磁盘模型

char memory_area[L*(B - K)];								//定义一个足够长的字符数组
char mem_area[L*(B - K)] = { '\0' };
Directory Directorys[maxDirectoryNumber + 1];				//包括目录在内，共有目录项maxDirectoryNumber+1，Directorys[0]保存目录
int bitMap[map_row_num][map_cow_num];						// 0:磁盘块空闲；1:磁盘块使用

void Init();
void directory();
void show_Menu();
void show_ldisk();
void show_bitMap();
int isExist(int index);
int close(int index);
int open(char *filename);
int getSub(char filename[]);
int create(char *filename);
int destroy(char *filename);
int show_File(char *filename);
int lseek(int index, int position);
int read(int index, char memory_area[], int count);
int write(int index, char memory_area[], int count);
int save(int L_pos, int B_pos, int bufLen, int sub);
int load(int step_L, int bufLen, int pos, char memory_area[]);

//提示菜单选项
void show_Menu()
{
	printf("---------------------菜单------------------\n");
	printf("**   1.创建文件\n");
	printf("**   2.列出所有文件信息\n");
	printf("**   3.打印位示图\n");
	printf("**   4.当前磁盘使用情况\n");
	printf("**   5.删除文件\n");
	printf("**   6.打开文件\n");
	printf("**   7.关闭文件\n");
	printf("**   8.改变文件读写指针位置\n");
	printf("**   9.文件读\n");
	printf("**   10.文件写\n");
	printf("**   11.查看文件状态\n");
	printf("**   0.退出\n");
	printf("---------------------菜单------------------\n");
}


//初始化文件系统：初始化位图(0表示空闲，1表示被占用)；初始化0号文件描述符；
void Init()
{
	int i, j;//两个计数量
			 /*初始化磁盘*/
	memset(ldisk, 0, sizeof(ldisk));
	/*初始化目录项*/
	for (i = 0; i <= maxDirectoryNumber; i++)
	{
		memset(Directorys[i].fileName, 0, sizeof(Directorys[i].fileName));
		if (i == 0)
		{
			//第一个为目录
			Directorys[i].index = 0;                //目录文件描述符序号为0
			Directorys[i].isFileFlag = 0;
			Directorys[i].count = 0;
		}
		else
		{
			//其他均表示文件
			Directorys[i].index = -1;          //初始化文件描述符序号为-1
			Directorys[i].count = -1;           //其他文件项不记录count值
			Directorys[i].isFileFlag = 1;       //初始化其他目录项均为文件
			Directorys[i].isOpenFlag = 0;       //文件均关闭
												/*配置文件描述符的相关项*/
			memset(Directorys[i].fileDescriptor.file_allocation_blocknumber, -1, File_Block_Length);
			//初始化磁盘号数组为-1
			Directorys[i].fileDescriptor.file_block_length = 0;    //初始化实际分配的磁盘号数组长度为0
			Directorys[i].fileDescriptor.fileLength = 0;           //初始化文件长度为0
			Directorys[i].fileDescriptor.beginpos = 0;             //初始化文件初始位置为0
			memset(Directorys[i].fileDescriptor.RWBuffer, 0, sizeof(Directorys[i].fileDescriptor.RWBuffer));
			Directorys[i].fileDescriptor.rwpointer = 0;            //初始化读写指针
		}
	}
	/*初始化位示图，均初始化为空闲*/
	for (i = 0; i<map_row_num; i++)
	{
		for (j = 0; j<map_cow_num; j++)
		{
			if (i*map_cow_num + j<K)
				bitMap[i][j] = 1;      //K个保留区
			else
				bitMap[i][j] = 0;      //其余区域空闲
		}
	}
}


//从指定文件顺序读入count 个字节memarea 指定的内存位置。读操作从文件的读写指针指示的位置开始
int read(int index, char memory_area[], int count)
{
	int sub = isExist(index);
	if (sub == ERROR)
	{
		printf("索引不正确！\n");
		return ERROR;
	}
	/*打开文件进行操作*/
	if (!Directorys[sub].isOpenFlag)
		open(Directorys[sub].fileName);
	/*读ldisk内容并复制到memory_area[]中*/
	int step_L = Directorys[sub].fileDescriptor.file_allocation_blocknumber[0];
	//L的起始位置
	int step_ = Directorys[sub].fileDescriptor.file_block_length - 1;     //记录要拷贝块的数量
	int pos = 0;
	for (int i = 0; i<step_; i++)
	{
		load(step_L, B, pos, memory_area);
		pos += B;
		step_L++;
	}
	/*拷贝ldisk[][]最后一块数据*/
	int strLen = Directorys[sub].fileDescriptor.fileLength - (B*step_);
	load(step_L, strLen, pos, memory_area);
	return OK;
}

//把memarea 指定的内存位置开始的count 个字节顺序写入指定文件。写操作从文件的读写指针指示的位置开始
int write(int index, char memory_area[], int count)
{
	/*根据index找到文件*/
	int sub = isExist(index);
	if (sub == ERROR)
	{
		printf("索引不正确！\n");
		return ERROR;
	}
	/*打开文件进行操作*/
	if (!Directorys[sub].isOpenFlag)
		open(Directorys[sub].fileName);
	/*读入要写的内容*/
	int i = 0;
	int step_ = 0;
	int num = 0;
	int step_L;
	int step_B;
	int COUNT = count;
	while (count)
	{
		Directorys[sub].fileDescriptor.RWBuffer[i] = memory_area[COUNT - count];
		count--;
		i++;
		if (i == Buffer_Length)
		{
			int step_L = Directorys[sub].fileDescriptor.file_allocation_blocknumber[step_];
			//列坐标
			int step_B = Buffer_Length * num;
			save(step_L, step_B, Buffer_Length, sub);//复制完成
			num++;
			if (num == B / Buffer_Length)
			{
				//写完一行，进入下一行
				num = 0;
				step_++;
			}
			i = 0;
		}
		if (count == 0)
		{
			step_L = Directorys[sub].fileDescriptor.file_allocation_blocknumber[step_];
			step_B = Buffer_Length * num;
			save(step_L, step_B, i, sub);
			break;
		}
	}
	memset(Directorys[sub].fileDescriptor.RWBuffer, '\0', Buffer_Length);
	return OK;
}

//把文件内容恢复到数组
int load(int step_L, int bufLen, int pos, char memory_area[])
{
	for (int i = 0; i<bufLen; i++)
		memory_area[pos + i] = ldisk[step_L][i];
	return OK;
}

//把数组ldisk 存储到文件
int save(int L_pos, int B_pos, int bufLen, int sub)
{
	for (int i = 0; i<bufLen; i++)
		ldisk[L_pos][B_pos + i] = Directorys[sub].fileDescriptor.RWBuffer[i];
	return OK;
}

//判断某个index值是否存在
int isExist(int index)
{
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (Directorys[i].index == index)
			return i;            //存在
	}
	return ERROR;        //不存在
}

//求目录项下标
int getSub(char filename[])
{
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (strcmp(Directorys[i].fileName, filename) == 0)
			return i;            //存在
	}
	return ERROR;      //不存在
}

//根据指定的文件名创建新文件
int create(char *filename)
{
	/*查询该文件是否存在*/
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (strcmp(Directorys[i].fileName, filename) == 0)
		{
			printf("该文件已经存在，无需创建！\n");
			return ERROR;            //文件存在
		}
	}
	/*文件不存在，更新Directorys值*/
	int sub, i, j;
	//搜索文件列表，碰到第一个index=-1的值
	for (i = 1; i <= maxDirectoryNumber; i++)
	{
		if (Directorys[i].index == -1)
		{
			sub = i;    //找到一个可以用的位置
			break;
		}
		else if (i == maxDirectoryNumber)
		{
			printf("磁盘已满，无法创建文件！");
			return ERROR;
		}
	}
	/*为sub目录项赋值*/
	strcpy(Directorys[sub].fileName, filename);
	for (i = 1; i <= maxDirectoryNumber; i++)
	{
		if (isExist(i) == -1)       //存在编号为i的index值
		{
			Directorys[sub].index = i;
			break;
		}
	}
	/*输入目录项其他项*/
	printf("请输入内存大小 (最大 61*512 Byte) ");
	scanf("%d", &Directorys[sub].fileDescriptor.fileLength);
	//计算需要空间块数量，用于判断剩余空间是否满足要求
	int L_Counter;
	if (Directorys[sub].fileDescriptor.fileLength%B)
		L_Counter = Directorys[sub].fileDescriptor.fileLength / B + 1;
	else
		L_Counter = Directorys[sub].fileDescriptor.fileLength / B;
	//查位示图，找到没使用的位置
	for (i = K; i<map_row_num*map_cow_num - L_Counter; i++)
	{
		int outflag = 0;
		//查找连续的L_Counter块bitMap空间块
		for (int j = 0; j<L_Counter; j++)
		{
			int maprow = (i + j) / map_cow_num;
			int mapcow = (i + j) % map_cow_num;
			if (bitMap[maprow][mapcow])
				break;
			else
			{
				//位图空闲
				if (j == L_Counter - 1)
					outflag = 1;
			}
		}
		if (outflag == 1)
		{
			//为块数组赋值，保存数据
			Directorys[sub].fileDescriptor.file_block_length = L_Counter;
			Directorys[sub].fileDescriptor.beginpos = i;
			for (j = 0; j<L_Counter; j++)
				Directorys[sub].fileDescriptor.file_allocation_blocknumber[j] = i + j;
			//初始化其他项
			Directorys[sub].isOpenFlag = 0;
			Directorys[sub].fileDescriptor.rwpointer = 0;
			memset(Directorys[sub].fileDescriptor.RWBuffer, '\0', Buffer_Length);
			break;
		}
		else if (L_Counter + i == map_row_num*map_cow_num - 1 - K)
		{
			printf("内存不足，无法分配！\n");
			Directorys[sub].index = -1;
			return ERROR;
		}
	}
	int map_ = i;
	printf("文件 %s 创建成功!\n", filename);
	/*更新位示图*/
	for (int j = 0; j<Directorys[sub].fileDescriptor.file_block_length; j++)
	{
		//从i开始更新file_block_length的数据
		int maprow = (map_ + j) / map_cow_num;
		int mapcow = (map_ + j) % map_cow_num;
		bitMap[maprow][mapcow] = 1;
	}
	Directorys[0].count++;
	return OK;
}

//根据文件名删除文件
int destroy(char *filename)
{
	int sub, i;
	for (i = 1; i <= maxDirectoryNumber; i++)
	{
		if (strcmp(Directorys[i].fileName, filename) == 0)
		{
			sub = i;
			break;                 //找到该文件
		}
		else if (i == maxDirectoryNumber)//
		{
			printf("该文件不存在！\n");
			return ERROR;
		}
	}
	//  删除操作
	if (Directorys[sub].isOpenFlag)
	{
		printf("文件打开，无法删除！\n");
		return ERROR;
	}
	/*更新位示图信息*/
	int position = Directorys[sub].fileDescriptor.file_allocation_blocknumber[0];   //起始磁盘号
	for (i = 0; i < Directorys[sub].fileDescriptor.file_block_length; i++)
	{
		int d_row = (position + i) / map_row_num;
		int d_cow = (position + i) % map_row_num;
		bitMap[d_row][d_cow] = 0;
	}

	//更新Directorys[sub]信息
	memset(Directorys[sub].fileName, 0, File_Name_Length);
	Directorys[sub].index = -1;
	/*配置文件描述符的相关项*/
	memset(Directorys[sub].fileDescriptor.file_allocation_blocknumber, -1, File_Block_Length);
	//初始化磁盘号数组为-1
	Directorys[sub].fileDescriptor.file_block_length = 0;
	Directorys[sub].fileDescriptor.fileLength = 0;
	Directorys[sub].fileDescriptor.beginpos = 0;
	memset(Directorys[sub].fileDescriptor.RWBuffer, '\0', Buffer_Length);
	Directorys[sub].fileDescriptor.rwpointer = 0;

	printf("文件 %s 删除成功！\n", filename);
	Directorys[0].count--;
	return OK;
}

//根据文件名打开文件。该函数返回的索引号可用于后续的read, write, lseek, 或close 操作
int open(char *filename)
{
	int sub;
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (strcmp(Directorys[i].fileName, filename) == 0)
		{
			sub = i;
			break;                 //找到该文件
		}
		else if (i == maxDirectoryNumber)
		{
			return ERROR;           //搜索结束，未找到对应文件名
		}
	}
	Directorys[sub].isOpenFlag = 1;
	return OK;
}


//把文件的读写指针移动到pos 指定的位置
int lseek(int index, int position)
{
	//先找到index代表的数据项
	int sub;
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (Directorys[i].index == index)
		{
			sub = i;
			break;                          //找到
		}
		else if (i == maxDirectoryNumber)                     //是否为最后一个查询的数据
		{
			printf("index 数据有错误，找不到该索引\n");
			return ERROR;
		}
	}
	Directorys[sub].fileDescriptor.rwpointer = position;
	return OK;
}

//关闭文件，把缓冲区的内容写入磁盘，释放该文件，再打开文件表中对应的表目，返回状态信息
int close(int index)
{
	int sub;                               //Directorys[sub]
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (Directorys[i].index == index)
		{
			sub = i;
			if (!Directorys[i].isOpenFlag)
			{
				printf("该文件已经为关闭状态！\n");
				return ERROR;
			}
			break;                          //找到
		}
		else if (i == maxDirectoryNumber)
		{
			printf("文件不存在，打开失败\n");
			return ERROR;
		}
	}
	//把缓冲区的内容写入磁盘
	int pos = Directorys[sub].fileDescriptor.file_allocation_blocknumber[0];//纵坐标的起始位置
	for (int i = 0; i<Directorys[sub].fileDescriptor.fileLength; i++)
	{
		int L_Pos = i / B;
		int B_Pos = i % B;
		ldisk[pos + L_Pos][B_Pos] = Directorys[sub].fileDescriptor.RWBuffer[i];
	}
	//释放该文件在打开文件表中对应的表目
	Directorys[sub].isOpenFlag = 0;
	Directorys[sub].fileDescriptor.rwpointer = 0;
	return OK;
}

//列表显示所有文件及其长度
void directory()
{
	if (Directorys[0].count == 0)
		printf("目前没有文件\n");
	for (int i = 1; i <= Directorys[0].count; i++)
	{
		if (Directorys[i].index != -1)            //index值有效的文件输出
		{
			printf("第 %d 个文件为：%s\n", i, Directorys[i].fileName);
			printf("文件长度为：%d Byte\n", Directorys[i].fileDescriptor.fileLength);
		}
	}
}

//显示磁盘信息
void show_ldisk()
{
	for (int i = 0; i<L; i++)
	{
		printf("%d:", i);
		printf("%s\n", ldisk[i]);
	}
	printf("\n");
}


//显示文件信息
int show_File(char *filename)
{
	int sub;
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (strcmp(Directorys[i].fileName, filename) == 0)
		{
			sub = i;
			break;                 //找到该文件
		}
		else if (i == maxDirectoryNumber)
			return ERROR;           //搜索结束，未找到对应文件名
	}
	printf("文件名：%s\n", Directorys[sub].fileName);
	printf("文件打开状态（1为打开，0为关闭）：%d\n", Directorys[sub].isOpenFlag);
	printf("文件的index索引值: %d\n", Directorys[sub].index);
	printf("文件的长度：%d Byte\n", Directorys[sub].fileDescriptor.fileLength);
	return OK;
}


//显示位图信息
void show_bitMap()
{
	for (int i = 0; i<map_row_num; i++)
	{
		for (int j = 0; j<map_cow_num; j++)
			printf("%d  ", bitMap[i][j]);
		printf("\n");
	}
}

int main()
{
	int scanner;
	Init();
	show_Menu();
	printf("请选择： ");
	scanf("%d", &scanner);
	while (scanner != 0)
	{
		switch (scanner)
		{
		case 1://创建文件
		{
			char newFile[20];
			printf("请输入文件名：  ");
			scanf(" %s", newFile);
			create(newFile);
			break;
		}
		case 2://输出所有的文件信息
		{
			directory();
			break;
		}
		case 3://输出位示图
		{
			show_bitMap();
			break;
		}
		case 4://显示磁盘使用情况
		{
			show_ldisk();
			break;
		}
		case 5://删除文件
		{
			printf("请输入要删除的文件名:");
			char destroyfile[20];
			scanf(" %s", destroyfile);
			destroy(destroyfile);
			break;
		}
		case 6://打开文件
		{
			printf("请输入要打开的文件名:");
			char openfile[20];
			scanf(" %s", openfile);
			if (open(openfile) == ERROR)
				printf("不存在文件 %s ，打开失败\n", openfile);
			else
				printf("文件 %s 打开成功！\n", openfile);
			break;
		}
		case 7://关闭文件
		{
			printf("请输入要关闭的文件名:");
			char closefile[20];
			scanf(" %s", closefile);
			int sub = getSub(closefile);
			if (sub == 0)
				printf("不存在文件 %s ，关闭失败\n", closefile);
			else
			{
				if (close(Directorys[sub].index) != ERROR)
					printf("文件 %s 关闭成功！\n", closefile);
			}
			break;
		}
		case 8://改变指针的位置
		{
			printf("请输入要操作的文件名：");
			char movefile[20];
			int move_pos;
			scanf(" %s", movefile);
			printf("请输入要移动到的位置：");
			scanf("%d", &move_pos);
			int sub = getSub(movefile);
			if (sub != 0)
			{
				if (lseek(Directorys[sub].index, move_pos))
				{
					printf("文件 %s 指针移动成功！\n", movefile);
				}
				else
					printf("文件 %s 移动失败..\n", movefile);
			}
			break;
		}
		case 9://文件读
		{
			printf("请输入要读的文件名：");
			char readfile[20];
			scanf(" %s", readfile);
			int sub = getSub(readfile);
			if (sub != 0)
			{
				//读入整个文件到mem_area[]
				read(Directorys[sub].index, memory_area, Directorys[sub].fileDescriptor.fileLength);
				printf("%s\n", memory_area);
			}
			else
				printf("文件名输入错误..\n");
			break;
		}
		case 10://文件写
		{
			printf("请输入要写入数据的文件名：");
			char writefile[20];
			scanf(" %s", writefile);
			int sub = getSub(writefile);
			if (sub != 0)
			{
				//读入整个文件到mem_area[]
				char writebuf[L*(B - K)];
				memset(writebuf, '\0', L*(B - K));
				printf("请输入要写入的数据：");
				scanf(" %s", writebuf);
				int len = 0;
				for (int i = 0; i<Directorys[sub].fileDescriptor.fileLength; i++)
				{
					if (writebuf[i] == '\0')
					{
						len = i;
						break;
					}
				}
				write(Directorys[sub].index, writebuf, len);
			}
			else
				printf("文件名输入错误..\n");
			break;
		}
		case 11:
		{
			printf("请输入要写入数据的文件名：\n");
			char seefile[20];
			scanf(" %s", seefile);
			if (getSub(seefile))
				show_File(seefile);
			else
				printf("文件名不存在...\n");
		}
		}
		printf("\n");
		printf("请选择： ");
		scanf("%d", &scanner);
	}
	return 0;
}
