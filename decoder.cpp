#include<stdio.h>
#include <stdlib.h>
#include <functional>
#include <string.h>
#include <math.h>
#include "common_func.h"

using namespace std;
/*	decoding���� �� 8bit 2������ ���� �ʴ� ������ �տ� 0�� ���̱� ���� �Լ�*/	
void bin_conversion_8bit(char*arr, int num)
{

	char buf[9] = { 0 };
	_itoa_s(num, arr, 9, 2);//���� num�� 2���� ���ڿ��� ��ȯ
	for (int i = 0; i < 8- strlen(arr); i++)//8�� arr�� ������ ���̸�ŭ 0�� �տ� �����̱� ����
	{
		buf[i] = '0';
		buf[i + 1] = NULL;
	}
	strcat_s(buf, 9, arr);
	strcpy_s(arr, 9, buf);
}

/* decoding �Լ� */
int decode(const char* input, const char* output, int eod, int gonormal, char*** adaptive_table, char** normal_table, int std_length)
{
	FILE* code_encode = NULL;
	FILE* code_decode = NULL;
	FILE* decode_out = NULL;
	char* buf_transfer = { 0 };
	int file_size;
	char buf_8bit[9] = { 0 };
	char* decode_buf;
	if (fopen_s(&code_encode, input, "rb") != 0)//input ���� ����

	{
		printf("file doesn't exist!\n");
		return -1;
	}

	if (fopen_s(&code_decode, "decode.hbs", "wb") != 0)//decode ���� ����
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	file_size = filesize(input);	

	buf_transfer = (char*)malloc(file_size);//���� �����ŭ buf_transfer�� �޸� �Ҵ�

	fread(buf_transfer, 1, file_size, code_encode);//buf_transfer�� �о����

	fclose(code_encode);//input ���� close

	/*		�о�� �ڵ� �м��� ���� ���ڸ� 8bit�� ���� �� decode���Ͽ� write      */
	for (int j = 0; j < file_size; j++) {
		unsigned char value = buf_transfer[j];

		if (value > 127) {//8bit �� ä���� ���ڴ� 2���� ���� ���ʿ�
			_itoa_s(value, buf_8bit, 9, 2);
			fwrite(buf_8bit, strlen(buf_8bit), 1, code_decode);
			continue;
		}
		else {////7bit ���Ϻ��ʹ� �տ� 0�� �ٿ��� 2������ 8bit�� ���� �ʿ�
			bin_conversion_8bit(buf_8bit, value);
			fwrite(buf_8bit, strlen(buf_8bit), 1, code_decode);
			continue;
		}
	}

	fclose(code_decode);//decode ���� close

	if (fopen_s(&code_decode, "decode.hbs", "rb") != 0)//decode���� ����
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	if (fopen_s(&decode_out, output, "wt") != 0)//output ���� ����
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	decode_buf = (char*)malloc(std_length + 1);
	decode_buf[0] = NULL;
	int k = 0;
	bool first = true;
	bool table_transition = false;
	bool if_eod = false;
	bool if_gonormal = false;	
	int precede_sym;

	/*	��Ʈ�� �ϳ��� �̾���̸鼭 huffman tree�������� ascii���� ��� */
	for (int i = 0; i < 8 * file_size; i++)
	{
		decode_buf[k] = fgetc(code_decode);//�ѱ��ھ� ��Ʈ �о��
		decode_buf[k + 1] = NULL;
		k++;
		table_transition = false;
		for (int j = 0; j < 128; j++)
		{
			if (first == true) {//ù symbol ���� ��
				if (normal_table[j] != NULL)//������ Ʈ���� ���� ������ ������
				{
					if (!strcmp(decode_buf, normal_table[j])) {// normal table���� �� ���� �ϴ��� Ȯ��
						if (j != eod) {
							fputc(j, decode_out);//�ش� ���� ���
							k = 0;//decode_buf reset	
							precede_sym = j;//precede symbol update
							first = false;							
						}
						else
							if_eod = true;//eod�� �ٷ� break �� �� �ֵ��� ó��
						break;
					}
				}				
			}


			else if ((adaptive_table[precede_sym] == NULL)||if_gonormal==true) {//precede_symbol table�� �������� �ʰų� gonormal�� true�� ���
				if (normal_table[j] != NULL)//������ Ʈ���� ���� ������ ������
				{
					if (!strcmp(decode_buf, normal_table[j])) {//normal table���� �ش� �ڵ带 ã����
						if (j != eod) {
							fputc(j, decode_out);//���� ���
							k = 0;//decode_buf reset	
							precede_sym = j;//precede symbol update
							if (if_gonormal == true)//gonormal�̾ �����ſ����� gonormal �ٽ� false
								if_gonormal = false;
						}

						else
							if_eod = true;//eod�� �ٷ� break
						break;
					}
				}
				continue;
			}

			
			else  //adaptive table�� �о�� �ϴ� ���
			{	
				if (adaptive_table[precede_sym][j] != NULL) {
						if (!strcmp(decode_buf, adaptive_table[precede_sym][j])){//adaptive table���� �ڵ� Ȯ���ϸ�
						
							if (j == gonormal) {//gonormal �ΰ�� gonormal true
								if_gonormal = true;
								k = 0;//decode buf reset
							}

							else if (j!=eod) {//�����۵�
								fputc(j, decode_out);
								k = 0;
								precede_sym = j;
							}
							else//eod
								if_eod = true;
							break;
						}						
					}
				
				continue;
			}			

		}
		if (if_eod == true)
			break;
	}

	/*file close*/
	fclose(code_decode);
	fclose(decode_out);

	/* memory free*/
	free(decode_buf);
	free(buf_transfer);
	remove("decode.hbs");
	return 0;
}
/* Decoder */
int main() {
		FILE* code_encode = NULL;//������ �ڵ� ���� ��Ʈ��
		FILE * table_encode = NULL;//������ ���̺� ���� ��Ʈ��
		FILE* code_decode = NULL;//���ڵ� ���� ��Ʈ��
		FILE* decode_out = NULL;//output ���� ��Ʈ��
		char* normal_table[128] = { 0 };
		pair<int, int> adapt_p[128][128];
		char*** adaptive_table = { 0 };			
		int std_length = 0;		
		char buf_8bit[9] = { 0 };
		int eod = 128;
		int gonormal = 128;

		/* pair �ʱ�ȭ */
		for (int i = 0; i < 128; i++) {			
			for (int j = 0; j < 128; j++)
				adapt_p[i][j] = make_pair(0, j);
		}		
	
		if (fopen_s(&table_encode, "huffman_table.hbs", "rb") != 0)//������ ���̺� ���� ����
		{
			printf("file doesn't exist!\n");
			return -1;
		}

	/*		������ ���̺��� ����, ��Ʈ ��, ������ ��Ʈ �Ľ� �� huf_table �迭�� ����		*/
		while (!feof(table_encode))
		{
			char length_buf[9] = { 0 };
			int symbol = fgetc(table_encode);
			int length=0;
			if (symbol == EOF)
				break;			
			length = fgetc(table_encode);
			if(length>std_length)
				std_length = length;
			normal_table[symbol] = (char*)malloc(length+1);
			fgets(normal_table[symbol], length+1, table_encode);
		}
		fclose(table_encode);//������ ���̺� ���� close

		
		if (fopen_s(&table_encode, "context_adaptive_huffman_table.hbs", "rb") != 0)//context adaptive ���̺� ���� ����
		{
			printf("file doesn't exist!\n");
			return -1;
		}

		/*		adaptive table���� ����, �� ���Ľ� �� adapt_p �迭�� ����		*/
		while (!feof(table_encode))
		{
			int pre_symbol = fgetc(table_encode);
			if (pre_symbol == EOF)
				break;		
			unsigned char cycle = fgetc(table_encode);
			if (cycle == EOF)
				break;
			
			for (int i = 0; i < cycle; i++)
			{
				int symbol = fgetc(table_encode);
				if (symbol == EOF)
					break;
				int freq = fgetc(table_encode);				
				if (freq == EOF)
					break;				
				
				adapt_p[pre_symbol][symbol].first=freq;				
			}			
		}
		fclose(table_encode);//adaptive ���̺� ���� close

		/* eod search */
		for (int i = 0; i < 128; i++)
		{
			for (int j = 0; j < 128; j++)
				if (adapt_p[i][j].first != 0) {
					if (eod >= j)
						eod = j;
					break;
				}
		}	

		/* gonormal search */
		for (int i = 0; i < 128; i++)
		{
			for (int j = eod+1; j < 128; j++)
				if (adapt_p[i][j].first != 0) {
					if (gonormal >= j)
						gonormal = j;
					break;
				}
		}
	
		/* adaptive table �޸� �Ҵ� */
		adaptive_table = (char***)malloc(sizeof(char**) * 128);
		for (int i = 0; i < 128; i++)
			adaptive_table[i] = { 0 };

		for (int i = 0; i < 128; i++)
		{
			adaptive_table[i] = (char**)malloc(sizeof(char*) * (128));//EOD �����Ͽ� table ����
			for (int j = 0; j < 128; j++)
				adaptive_table[i][j] = { 0 };
		}

		/* ��� symbole�鿡 ���� eod, gonormal update */
		for (int i = 0; i < 128; i++) {	
			for (int j = 0; j < 128; j++) {
				if (adapt_p[i][j].first != 0) {
					if(adapt_p[i][eod].first==0)
						adapt_p[i][eod].first++;
					if(adapt_p[i][gonormal].first==0)
						adapt_p[i][gonormal].first++;
					break;
				}
			}
		}		

		

		/* adaptive table ���� */
		for (int i = 0; i < 128; i++)
		{
			priority_queue <node*, vector<node*>, compfreq> hp_temp;
			for (int j = 0; j < 128; j++) {

				if (adapt_p[i][j].first != 0) {
					node* n = new node;
					n->info = adapt_p[i][j];
					hp_temp.push(n);
				}
			}
			if (hp_temp.size() != 0) 				
				preorder_traversal(makehuffman(hp_temp), NULL, adaptive_table[i]);//��ȸ �ϸ鼭 table ���
			
			else {
				for (int j = 0; j < 128; j++)
					free(adaptive_table[i][j]);//�޸� ����
				adaptive_table[i] = NULL;//nullify
			}
		}
		/* �� input�鿡 ���� output ���� ���� */
		if (decode("training_input_code.hbs", "training_output.txt", eod, gonormal, adaptive_table, normal_table, std_length) == -1)
		{
			printf("file doesn't exist!\n");
			return -1;
		}
		printf("training_output.txt ���� ���� �Ϸ�!\n");

		if (decode("test_input1_code.hbs", "test_output1.txt", eod, gonormal, adaptive_table, normal_table, std_length) == -1)
		{
			printf("file doesn't exist!\n");
			return -1;
		}
		printf("test_output1.txt ���� ���� �Ϸ�!\n");
		
		if (decode("test_input2_code.hbs", "test_output2.txt", eod, gonormal, adaptive_table, normal_table, std_length) == -1)
		{
			printf("file doesn't exist!\n");
			return -1;
		}
		printf("test_output2.txt ���� ���� �Ϸ�!\n");

		if (decode("test_input3_code.hbs", "test_output3.txt",eod, gonormal, adaptive_table, normal_table, std_length) == -1)
		{
			printf("file doesn't exist!\n");
			return -1;
		}
		printf("test_output3.txt ���� ���� �Ϸ�!\n");

	/* �޸� ���� */
	for (int i = 0; i < 128; i++)
	{
		if (normal_table[i] != NULL)
			free(normal_table[i]);
	}	

	for (int i = 0; i < 128; i++)
	{
		if (adaptive_table[i] == NULL)
			continue;
		for (int j = 0; j < 128; j++) {
			if (adaptive_table[i][j] != NULL)
				free(adaptive_table[i][j]);
		}
		free(adaptive_table[i]);
	}
	free(adaptive_table);

		return 0;
}