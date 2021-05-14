#include<stdio.h>
#include <stdlib.h>
#include <functional>
#include <string.h>
#include <math.h>
#include "common_func.h"

using namespace std;
/*	decoding했을 때 8bit 2진수가 되지 않는 수들은 앞에 0을 붙이기 위한 함수*/	
void bin_conversion_8bit(char*arr, int num)
{

	char buf[9] = { 0 };
	_itoa_s(num, arr, 9, 2);//먼저 num을 2진수 문자열로 변환
	for (int i = 0; i < 8- strlen(arr); i++)//8과 arr의 길이의 차이만큼 0을 앞에 덧붙이기 위함
	{
		buf[i] = '0';
		buf[i + 1] = NULL;
	}
	strcat_s(buf, 9, arr);
	strcpy_s(arr, 9, buf);
}

/* decoding 함수 */
int decode(const char* input, const char* output, int eod, int gonormal, char*** adaptive_table, char** normal_table, int std_length)
{
	FILE* code_encode = NULL;
	FILE* code_decode = NULL;
	FILE* decode_out = NULL;
	char* buf_transfer = { 0 };
	int file_size;
	char buf_8bit[9] = { 0 };
	char* decode_buf;
	if (fopen_s(&code_encode, input, "rb") != 0)//input 파일 오픈

	{
		printf("file doesn't exist!\n");
		return -1;
	}

	if (fopen_s(&code_decode, "decode.hbs", "wb") != 0)//decode 파일 오픈
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	file_size = filesize(input);	

	buf_transfer = (char*)malloc(file_size);//파일 사이즈만큼 buf_transfer에 메모리 할당

	fread(buf_transfer, 1, file_size, code_encode);//buf_transfer에 읽어오기

	fclose(code_encode);//input 파일 close

	/*		읽어온 코드 분석을 위해 문자를 8bit로 가공 후 decode파일에 write      */
	for (int j = 0; j < file_size; j++) {
		unsigned char value = buf_transfer[j];

		if (value > 127) {//8bit 꽉 채워진 숫자는 2진수 보정 불필요
			_itoa_s(value, buf_8bit, 9, 2);
			fwrite(buf_8bit, strlen(buf_8bit), 1, code_decode);
			continue;
		}
		else {////7bit 이하부터는 앞에 0을 붙여서 2진수를 8bit로 보정 필요
			bin_conversion_8bit(buf_8bit, value);
			fwrite(buf_8bit, strlen(buf_8bit), 1, code_decode);
			continue;
		}
	}

	fclose(code_decode);//decode 파일 close

	if (fopen_s(&code_decode, "decode.hbs", "rb") != 0)//decode파일 오픈
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	if (fopen_s(&decode_out, output, "wt") != 0)//output 파일 오픈
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

	/*	비트를 하나씩 이어붙이면서 huffman tree기준으로 ascii문자 출력 */
	for (int i = 0; i < 8 * file_size; i++)
	{
		decode_buf[k] = fgetc(code_decode);//한글자씩 비트 읽어옴
		decode_buf[k + 1] = NULL;
		k++;
		table_transition = false;
		for (int j = 0; j < 128; j++)
		{
			if (first == true) {//첫 symbol 읽을 때
				if (normal_table[j] != NULL)//허프만 트리의 값이 존재할 때에만
				{
					if (!strcmp(decode_buf, normal_table[j])) {// normal table에서 값 존재 하는지 확인
						if (j != eod) {
							fputc(j, decode_out);//해당 문자 출력
							k = 0;//decode_buf reset	
							precede_sym = j;//precede symbol update
							first = false;							
						}
						else
							if_eod = true;//eod면 바로 break 할 수 있도록 처리
						break;
					}
				}				
			}


			else if ((adaptive_table[precede_sym] == NULL)||if_gonormal==true) {//precede_symbol table이 존재하지 않거나 gonormal이 true인 경우
				if (normal_table[j] != NULL)//허프만 트리의 값이 존재할 때에만
				{
					if (!strcmp(decode_buf, normal_table[j])) {//normal table에서 해당 코드를 찾으면
						if (j != eod) {
							fputc(j, decode_out);//문자 출력
							k = 0;//decode_buf reset	
							precede_sym = j;//precede symbol update
							if (if_gonormal == true)//gonormal이어서 읽은거였으면 gonormal 다시 false
								if_gonormal = false;
						}

						else
							if_eod = true;//eod면 바로 break
						break;
					}
				}
				continue;
			}

			
			else  //adaptive table을 읽어야 하는 경우
			{	
				if (adaptive_table[precede_sym][j] != NULL) {
						if (!strcmp(decode_buf, adaptive_table[precede_sym][j])){//adaptive table에서 코드 확인하면
						
							if (j == gonormal) {//gonormal 인경우 gonormal true
								if_gonormal = true;
								k = 0;//decode buf reset
							}

							else if (j!=eod) {//정상작동
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
		FILE* code_encode = NULL;//허프만 코드 파일 스트림
		FILE * table_encode = NULL;//허프만 테이블 파일 스트림
		FILE* code_decode = NULL;//디코딩 파일 스트림
		FILE* decode_out = NULL;//output 파일 스트림
		char* normal_table[128] = { 0 };
		pair<int, int> adapt_p[128][128];
		char*** adaptive_table = { 0 };			
		int std_length = 0;		
		char buf_8bit[9] = { 0 };
		int eod = 128;
		int gonormal = 128;

		/* pair 초기화 */
		for (int i = 0; i < 128; i++) {			
			for (int j = 0; j < 128; j++)
				adapt_p[i][j] = make_pair(0, j);
		}		
	
		if (fopen_s(&table_encode, "huffman_table.hbs", "rb") != 0)//허프만 테이블 파일 오픈
		{
			printf("file doesn't exist!\n");
			return -1;
		}

	/*		허프만 테이블에서 문자, 비트 수, 허프만 비트 파싱 후 huf_table 배열에 저장		*/
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
		fclose(table_encode);//허프만 테이블 파일 close

		
		if (fopen_s(&table_encode, "context_adaptive_huffman_table.hbs", "rb") != 0)//context adaptive 테이블 파일 오픈
		{
			printf("file doesn't exist!\n");
			return -1;
		}

		/*		adaptive table에서 문자, 빈도 수파싱 후 adapt_p 배열에 저장		*/
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
		fclose(table_encode);//adaptive 테이블 파일 close

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
	
		/* adaptive table 메모리 할당 */
		adaptive_table = (char***)malloc(sizeof(char**) * 128);
		for (int i = 0; i < 128; i++)
			adaptive_table[i] = { 0 };

		for (int i = 0; i < 128; i++)
		{
			adaptive_table[i] = (char**)malloc(sizeof(char*) * (128));//EOD 포함하여 table 생성
			for (int j = 0; j < 128; j++)
				adaptive_table[i][j] = { 0 };
		}

		/* 모든 symbole들에 대해 eod, gonormal update */
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

		

		/* adaptive table 생성 */
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
				preorder_traversal(makehuffman(hp_temp), NULL, adaptive_table[i]);//순회 하면서 table 출력
			
			else {
				for (int j = 0; j < 128; j++)
					free(adaptive_table[i][j]);//메모리 해제
				adaptive_table[i] = NULL;//nullify
			}
		}
		/* 각 input들에 대한 output 파일 생성 */
		if (decode("training_input_code.hbs", "training_output.txt", eod, gonormal, adaptive_table, normal_table, std_length) == -1)
		{
			printf("file doesn't exist!\n");
			return -1;
		}
		printf("training_output.txt 압축 해제 완료!\n");

		if (decode("test_input1_code.hbs", "test_output1.txt", eod, gonormal, adaptive_table, normal_table, std_length) == -1)
		{
			printf("file doesn't exist!\n");
			return -1;
		}
		printf("test_output1.txt 압축 해제 완료!\n");
		
		if (decode("test_input2_code.hbs", "test_output2.txt", eod, gonormal, adaptive_table, normal_table, std_length) == -1)
		{
			printf("file doesn't exist!\n");
			return -1;
		}
		printf("test_output2.txt 압축 해제 완료!\n");

		if (decode("test_input3_code.hbs", "test_output3.txt",eod, gonormal, adaptive_table, normal_table, std_length) == -1)
		{
			printf("file doesn't exist!\n");
			return -1;
		}
		printf("test_output3.txt 압축 해제 완료!\n");

	/* 메모리 해제 */
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