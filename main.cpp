#include <iostream>
#include <string.h>
#include <stdlib.h>

using namespace std;

//�����ܴ�СΪ64*512
#define B        512       			//����洢��ĳ���B�ֽ�
#define L        64        			//������̵Ĵ洢������L���߼���0-(L-1)
#define K        3      			//���̵�ǰk �����Ǳ�����

#define OK       1     				//�����ɹ�
#define ERROR    -1    				//��������

#define File_Block_Length    (B-3)  //�����ļ����̿�����鳤��
#define File_Name_Length     (B-1)  //�����ļ����ĳ���

//8*8����ʾ64���߼��飬�����̵��ܿ�����L
#define map_row_num     8           //λͼ������
#define map_cow_num     8           //λͼ������

#define maxDirectoryNumber  49      //����Ŀ¼��Ŀ¼��ļ������������ֵ

#define Buffer_Length 64            //�����д�������Ĵ�С�����ڴ��ļ�����

typedef struct FileDescriptor//�ļ�������
{
	int fileLength;                                        //�ļ�����
	int file_allocation_blocknumber[File_Block_Length];    //�ļ����䵽�Ĵ��̿������
	int file_block_length;                                 //�ļ����䵽�Ĵ��̿������ʵ�ʳ���
	int beginpos;                                          //�ļ�����ʼ����λ��beginpos=map_cow_num*row + cow
	int rwpointer;                                         //��дָ�룺��ʼ��Ϊ0������RWBuffer�仯
	char RWBuffer[Buffer_Length];                          //��д�����������ļ�������
}FileDescriptor;

typedef struct Directory
{
	int  index;                     						//�ļ����������
	int  count;                     						//����Ŀ¼�м�¼�ļ�������
	char fileName[File_Name_Length];						//�ļ���
	int  isFileFlag;                						//�ж����ļ�����Ŀ¼��1Ϊ�ļ���0ΪĿ¼
	int  isOpenFlag;                						//�ж��ļ��Ƿ�򿪣�1Ϊ�򿪣�0Ϊ�ر�
	FileDescriptor fileDescriptor;  						//�ļ���������Ϣ����index ��Ӧ
}Directory;


char ldisk[L][B];											//��������ģ��

char memory_area[L*(B - K)];								//����һ���㹻�����ַ�����
char mem_area[L*(B - K)] = { '\0' };
Directory Directorys[maxDirectoryNumber + 1];				//����Ŀ¼���ڣ�����Ŀ¼��maxDirectoryNumber+1��Directorys[0]����Ŀ¼
int bitMap[map_row_num][map_cow_num];						// 0:���̿���У�1:���̿�ʹ��

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

//��ʾ�˵�ѡ��
void show_Menu()
{
	printf("---------------------�˵�------------------\n");
	printf("**   1.�����ļ�\n");
	printf("**   2.�г������ļ���Ϣ\n");
	printf("**   3.��ӡλʾͼ\n");
	printf("**   4.��ǰ����ʹ�����\n");
	printf("**   5.ɾ���ļ�\n");
	printf("**   6.���ļ�\n");
	printf("**   7.�ر��ļ�\n");
	printf("**   8.�ı��ļ���дָ��λ��\n");
	printf("**   9.�ļ���\n");
	printf("**   10.�ļ�д\n");
	printf("**   11.�鿴�ļ�״̬\n");
	printf("**   0.�˳�\n");
	printf("---------------------�˵�------------------\n");
}


//��ʼ���ļ�ϵͳ����ʼ��λͼ(0��ʾ���У�1��ʾ��ռ��)����ʼ��0���ļ���������
void Init()
{
	int i, j;//����������
			 /*��ʼ������*/
	memset(ldisk, 0, sizeof(ldisk));
	/*��ʼ��Ŀ¼��*/
	for (i = 0; i <= maxDirectoryNumber; i++)
	{
		memset(Directorys[i].fileName, 0, sizeof(Directorys[i].fileName));
		if (i == 0)
		{
			//��һ��ΪĿ¼
			Directorys[i].index = 0;                //Ŀ¼�ļ����������Ϊ0
			Directorys[i].isFileFlag = 0;
			Directorys[i].count = 0;
		}
		else
		{
			//��������ʾ�ļ�
			Directorys[i].index = -1;          //��ʼ���ļ����������Ϊ-1
			Directorys[i].count = -1;           //�����ļ����¼countֵ
			Directorys[i].isFileFlag = 1;       //��ʼ������Ŀ¼���Ϊ�ļ�
			Directorys[i].isOpenFlag = 0;       //�ļ����ر�
												/*�����ļ��������������*/
			memset(Directorys[i].fileDescriptor.file_allocation_blocknumber, -1, File_Block_Length);
			//��ʼ�����̺�����Ϊ-1
			Directorys[i].fileDescriptor.file_block_length = 0;    //��ʼ��ʵ�ʷ���Ĵ��̺����鳤��Ϊ0
			Directorys[i].fileDescriptor.fileLength = 0;           //��ʼ���ļ�����Ϊ0
			Directorys[i].fileDescriptor.beginpos = 0;             //��ʼ���ļ���ʼλ��Ϊ0
			memset(Directorys[i].fileDescriptor.RWBuffer, 0, sizeof(Directorys[i].fileDescriptor.RWBuffer));
			Directorys[i].fileDescriptor.rwpointer = 0;            //��ʼ����дָ��
		}
	}
	/*��ʼ��λʾͼ������ʼ��Ϊ����*/
	for (i = 0; i<map_row_num; i++)
	{
		for (j = 0; j<map_cow_num; j++)
		{
			if (i*map_cow_num + j<K)
				bitMap[i][j] = 1;      //K��������
			else
				bitMap[i][j] = 0;      //�����������
		}
	}
}


//��ָ���ļ�˳�����count ���ֽ�memarea ָ�����ڴ�λ�á����������ļ��Ķ�дָ��ָʾ��λ�ÿ�ʼ
int read(int index, char memory_area[], int count)
{
	int sub = isExist(index);
	if (sub == ERROR)
	{
		printf("��������ȷ��\n");
		return ERROR;
	}
	/*���ļ����в���*/
	if (!Directorys[sub].isOpenFlag)
		open(Directorys[sub].fileName);
	/*��ldisk���ݲ����Ƶ�memory_area[]��*/
	int step_L = Directorys[sub].fileDescriptor.file_allocation_blocknumber[0];
	//L����ʼλ��
	int step_ = Directorys[sub].fileDescriptor.file_block_length - 1;     //��¼Ҫ�����������
	int pos = 0;
	for (int i = 0; i<step_; i++)
	{
		load(step_L, B, pos, memory_area);
		pos += B;
		step_L++;
	}
	/*����ldisk[][]���һ������*/
	int strLen = Directorys[sub].fileDescriptor.fileLength - (B*step_);
	load(step_L, strLen, pos, memory_area);
	return OK;
}

//��memarea ָ�����ڴ�λ�ÿ�ʼ��count ���ֽ�˳��д��ָ���ļ���д�������ļ��Ķ�дָ��ָʾ��λ�ÿ�ʼ
int write(int index, char memory_area[], int count)
{
	/*����index�ҵ��ļ�*/
	int sub = isExist(index);
	if (sub == ERROR)
	{
		printf("��������ȷ��\n");
		return ERROR;
	}
	/*���ļ����в���*/
	if (!Directorys[sub].isOpenFlag)
		open(Directorys[sub].fileName);
	/*����Ҫд������*/
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
			//������
			int step_B = Buffer_Length * num;
			save(step_L, step_B, Buffer_Length, sub);//�������
			num++;
			if (num == B / Buffer_Length)
			{
				//д��һ�У�������һ��
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

//���ļ����ݻָ�������
int load(int step_L, int bufLen, int pos, char memory_area[])
{
	for (int i = 0; i<bufLen; i++)
		memory_area[pos + i] = ldisk[step_L][i];
	return OK;
}

//������ldisk �洢���ļ�
int save(int L_pos, int B_pos, int bufLen, int sub)
{
	for (int i = 0; i<bufLen; i++)
		ldisk[L_pos][B_pos + i] = Directorys[sub].fileDescriptor.RWBuffer[i];
	return OK;
}

//�ж�ĳ��indexֵ�Ƿ����
int isExist(int index)
{
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (Directorys[i].index == index)
			return i;            //����
	}
	return ERROR;        //������
}

//��Ŀ¼���±�
int getSub(char filename[])
{
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (strcmp(Directorys[i].fileName, filename) == 0)
			return i;            //����
	}
	return ERROR;      //������
}

//����ָ�����ļ����������ļ�
int create(char *filename)
{
	/*��ѯ���ļ��Ƿ����*/
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (strcmp(Directorys[i].fileName, filename) == 0)
		{
			printf("���ļ��Ѿ����ڣ����贴����\n");
			return ERROR;            //�ļ�����
		}
	}
	/*�ļ������ڣ�����Directorysֵ*/
	int sub, i, j;
	//�����ļ��б�������һ��index=-1��ֵ
	for (i = 1; i <= maxDirectoryNumber; i++)
	{
		if (Directorys[i].index == -1)
		{
			sub = i;    //�ҵ�һ�������õ�λ��
			break;
		}
		else if (i == maxDirectoryNumber)
		{
			printf("�����������޷������ļ���");
			return ERROR;
		}
	}
	/*ΪsubĿ¼�ֵ*/
	strcpy(Directorys[sub].fileName, filename);
	for (i = 1; i <= maxDirectoryNumber; i++)
	{
		if (isExist(i) == -1)       //���ڱ��Ϊi��indexֵ
		{
			Directorys[sub].index = i;
			break;
		}
	}
	/*����Ŀ¼��������*/
	printf("�������ڴ��С (��� 61*512 Byte) ");
	scanf("%d", &Directorys[sub].fileDescriptor.fileLength);
	//������Ҫ�ռ�������������ж�ʣ��ռ��Ƿ�����Ҫ��
	int L_Counter;
	if (Directorys[sub].fileDescriptor.fileLength%B)
		L_Counter = Directorys[sub].fileDescriptor.fileLength / B + 1;
	else
		L_Counter = Directorys[sub].fileDescriptor.fileLength / B;
	//��λʾͼ���ҵ�ûʹ�õ�λ��
	for (i = K; i<map_row_num*map_cow_num - L_Counter; i++)
	{
		int outflag = 0;
		//����������L_Counter��bitMap�ռ��
		for (int j = 0; j<L_Counter; j++)
		{
			int maprow = (i + j) / map_cow_num;
			int mapcow = (i + j) % map_cow_num;
			if (bitMap[maprow][mapcow])
				break;
			else
			{
				//λͼ����
				if (j == L_Counter - 1)
					outflag = 1;
			}
		}
		if (outflag == 1)
		{
			//Ϊ�����鸳ֵ����������
			Directorys[sub].fileDescriptor.file_block_length = L_Counter;
			Directorys[sub].fileDescriptor.beginpos = i;
			for (j = 0; j<L_Counter; j++)
				Directorys[sub].fileDescriptor.file_allocation_blocknumber[j] = i + j;
			//��ʼ��������
			Directorys[sub].isOpenFlag = 0;
			Directorys[sub].fileDescriptor.rwpointer = 0;
			memset(Directorys[sub].fileDescriptor.RWBuffer, '\0', Buffer_Length);
			break;
		}
		else if (L_Counter + i == map_row_num*map_cow_num - 1 - K)
		{
			printf("�ڴ治�㣬�޷����䣡\n");
			Directorys[sub].index = -1;
			return ERROR;
		}
	}
	int map_ = i;
	printf("�ļ� %s �����ɹ�!\n", filename);
	/*����λʾͼ*/
	for (int j = 0; j<Directorys[sub].fileDescriptor.file_block_length; j++)
	{
		//��i��ʼ����file_block_length������
		int maprow = (map_ + j) / map_cow_num;
		int mapcow = (map_ + j) % map_cow_num;
		bitMap[maprow][mapcow] = 1;
	}
	Directorys[0].count++;
	return OK;
}

//�����ļ���ɾ���ļ�
int destroy(char *filename)
{
	int sub, i;
	for (i = 1; i <= maxDirectoryNumber; i++)
	{
		if (strcmp(Directorys[i].fileName, filename) == 0)
		{
			sub = i;
			break;                 //�ҵ����ļ�
		}
		else if (i == maxDirectoryNumber)//
		{
			printf("���ļ������ڣ�\n");
			return ERROR;
		}
	}
	//  ɾ������
	if (Directorys[sub].isOpenFlag)
	{
		printf("�ļ��򿪣��޷�ɾ����\n");
		return ERROR;
	}
	/*����λʾͼ��Ϣ*/
	int position = Directorys[sub].fileDescriptor.file_allocation_blocknumber[0];   //��ʼ���̺�
	for (i = 0; i < Directorys[sub].fileDescriptor.file_block_length; i++)
	{
		int d_row = (position + i) / map_row_num;
		int d_cow = (position + i) % map_row_num;
		bitMap[d_row][d_cow] = 0;
	}

	//����Directorys[sub]��Ϣ
	memset(Directorys[sub].fileName, 0, File_Name_Length);
	Directorys[sub].index = -1;
	/*�����ļ��������������*/
	memset(Directorys[sub].fileDescriptor.file_allocation_blocknumber, -1, File_Block_Length);
	//��ʼ�����̺�����Ϊ-1
	Directorys[sub].fileDescriptor.file_block_length = 0;
	Directorys[sub].fileDescriptor.fileLength = 0;
	Directorys[sub].fileDescriptor.beginpos = 0;
	memset(Directorys[sub].fileDescriptor.RWBuffer, '\0', Buffer_Length);
	Directorys[sub].fileDescriptor.rwpointer = 0;

	printf("�ļ� %s ɾ���ɹ���\n", filename);
	Directorys[0].count--;
	return OK;
}

//�����ļ������ļ����ú������ص������ſ����ں�����read, write, lseek, ��close ����
int open(char *filename)
{
	int sub;
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (strcmp(Directorys[i].fileName, filename) == 0)
		{
			sub = i;
			break;                 //�ҵ����ļ�
		}
		else if (i == maxDirectoryNumber)
		{
			return ERROR;           //����������δ�ҵ���Ӧ�ļ���
		}
	}
	Directorys[sub].isOpenFlag = 1;
	return OK;
}


//���ļ��Ķ�дָ���ƶ���pos ָ����λ��
int lseek(int index, int position)
{
	//���ҵ�index�����������
	int sub;
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (Directorys[i].index == index)
		{
			sub = i;
			break;                          //�ҵ�
		}
		else if (i == maxDirectoryNumber)                     //�Ƿ�Ϊ���һ����ѯ������
		{
			printf("index �����д����Ҳ���������\n");
			return ERROR;
		}
	}
	Directorys[sub].fileDescriptor.rwpointer = position;
	return OK;
}

//�ر��ļ����ѻ�����������д����̣��ͷŸ��ļ����ٴ��ļ����ж�Ӧ�ı�Ŀ������״̬��Ϣ
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
				printf("���ļ��Ѿ�Ϊ�ر�״̬��\n");
				return ERROR;
			}
			break;                          //�ҵ�
		}
		else if (i == maxDirectoryNumber)
		{
			printf("�ļ������ڣ���ʧ��\n");
			return ERROR;
		}
	}
	//�ѻ�����������д�����
	int pos = Directorys[sub].fileDescriptor.file_allocation_blocknumber[0];//���������ʼλ��
	for (int i = 0; i<Directorys[sub].fileDescriptor.fileLength; i++)
	{
		int L_Pos = i / B;
		int B_Pos = i % B;
		ldisk[pos + L_Pos][B_Pos] = Directorys[sub].fileDescriptor.RWBuffer[i];
	}
	//�ͷŸ��ļ��ڴ��ļ����ж�Ӧ�ı�Ŀ
	Directorys[sub].isOpenFlag = 0;
	Directorys[sub].fileDescriptor.rwpointer = 0;
	return OK;
}

//�б���ʾ�����ļ����䳤��
void directory()
{
	if (Directorys[0].count == 0)
		printf("Ŀǰû���ļ�\n");
	for (int i = 1; i <= Directorys[0].count; i++)
	{
		if (Directorys[i].index != -1)            //indexֵ��Ч���ļ����
		{
			printf("�� %d ���ļ�Ϊ��%s\n", i, Directorys[i].fileName);
			printf("�ļ�����Ϊ��%d Byte\n", Directorys[i].fileDescriptor.fileLength);
		}
	}
}

//��ʾ������Ϣ
void show_ldisk()
{
	for (int i = 0; i<L; i++)
	{
		printf("%d:", i);
		printf("%s\n", ldisk[i]);
	}
	printf("\n");
}


//��ʾ�ļ���Ϣ
int show_File(char *filename)
{
	int sub;
	for (int i = 1; i <= maxDirectoryNumber; i++)
	{
		if (strcmp(Directorys[i].fileName, filename) == 0)
		{
			sub = i;
			break;                 //�ҵ����ļ�
		}
		else if (i == maxDirectoryNumber)
			return ERROR;           //����������δ�ҵ���Ӧ�ļ���
	}
	printf("�ļ�����%s\n", Directorys[sub].fileName);
	printf("�ļ���״̬��1Ϊ�򿪣�0Ϊ�رգ���%d\n", Directorys[sub].isOpenFlag);
	printf("�ļ���index����ֵ: %d\n", Directorys[sub].index);
	printf("�ļ��ĳ��ȣ�%d Byte\n", Directorys[sub].fileDescriptor.fileLength);
	return OK;
}


//��ʾλͼ��Ϣ
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
	printf("��ѡ�� ");
	scanf("%d", &scanner);
	while (scanner != 0)
	{
		switch (scanner)
		{
		case 1://�����ļ�
		{
			char newFile[20];
			printf("�������ļ�����  ");
			scanf(" %s", newFile);
			create(newFile);
			break;
		}
		case 2://������е��ļ���Ϣ
		{
			directory();
			break;
		}
		case 3://���λʾͼ
		{
			show_bitMap();
			break;
		}
		case 4://��ʾ����ʹ�����
		{
			show_ldisk();
			break;
		}
		case 5://ɾ���ļ�
		{
			printf("������Ҫɾ�����ļ���:");
			char destroyfile[20];
			scanf(" %s", destroyfile);
			destroy(destroyfile);
			break;
		}
		case 6://���ļ�
		{
			printf("������Ҫ�򿪵��ļ���:");
			char openfile[20];
			scanf(" %s", openfile);
			if (open(openfile) == ERROR)
				printf("�������ļ� %s ����ʧ��\n", openfile);
			else
				printf("�ļ� %s �򿪳ɹ���\n", openfile);
			break;
		}
		case 7://�ر��ļ�
		{
			printf("������Ҫ�رյ��ļ���:");
			char closefile[20];
			scanf(" %s", closefile);
			int sub = getSub(closefile);
			if (sub == 0)
				printf("�������ļ� %s ���ر�ʧ��\n", closefile);
			else
			{
				if (close(Directorys[sub].index) != ERROR)
					printf("�ļ� %s �رճɹ���\n", closefile);
			}
			break;
		}
		case 8://�ı�ָ���λ��
		{
			printf("������Ҫ�������ļ�����");
			char movefile[20];
			int move_pos;
			scanf(" %s", movefile);
			printf("������Ҫ�ƶ�����λ�ã�");
			scanf("%d", &move_pos);
			int sub = getSub(movefile);
			if (sub != 0)
			{
				if (lseek(Directorys[sub].index, move_pos))
				{
					printf("�ļ� %s ָ���ƶ��ɹ���\n", movefile);
				}
				else
					printf("�ļ� %s �ƶ�ʧ��..\n", movefile);
			}
			break;
		}
		case 9://�ļ���
		{
			printf("������Ҫ�����ļ�����");
			char readfile[20];
			scanf(" %s", readfile);
			int sub = getSub(readfile);
			if (sub != 0)
			{
				//���������ļ���mem_area[]
				read(Directorys[sub].index, memory_area, Directorys[sub].fileDescriptor.fileLength);
				printf("%s\n", memory_area);
			}
			else
				printf("�ļ����������..\n");
			break;
		}
		case 10://�ļ�д
		{
			printf("������Ҫд�����ݵ��ļ�����");
			char writefile[20];
			scanf(" %s", writefile);
			int sub = getSub(writefile);
			if (sub != 0)
			{
				//���������ļ���mem_area[]
				char writebuf[L*(B - K)];
				memset(writebuf, '\0', L*(B - K));
				printf("������Ҫд������ݣ�");
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
				printf("�ļ����������..\n");
			break;
		}
		case 11:
		{
			printf("������Ҫд�����ݵ��ļ�����\n");
			char seefile[20];
			scanf(" %s", seefile);
			if (getSub(seefile))
				show_File(seefile);
			else
				printf("�ļ���������...\n");
		}
		}
		printf("\n");
		printf("��ѡ�� ");
		scanf("%d", &scanner);
	}
	return 0;
}
