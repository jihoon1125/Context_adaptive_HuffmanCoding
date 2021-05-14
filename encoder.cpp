#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <string.h>
#include <math.h>
#include "common_func.h"


/* byte align 함수 */
void bytealign(int length, char** arr)
{
	for (int i = 0; i < 8; i++)
	{
			if (arr[length - 1][i] != '0' && arr[length - 1][i] != '1') {//해당 비트가 0과 1이 아닌 경우
				arr[length - 1][i] = '0';								// 그 비트를 0으로 차근차근 채워나간다.
			}		
	}	
	arr[length - 1][8] = NULL;//마지막 비트는 NULL
}

/*normal table을 huffman_table.hbs에 쓰는 함수*/
int normal_tablewr(char **normal_table) {
	FILE* fp_table = NULL;
	if (fopen_s(&fp_table, "huffman_table.hbs", "wb") != 0)//허프만 테이블 파일 열기
		return -1;
	

	for (int i = 0; i < 128; i++) {
		if (normal_table[i] != NULL) {//비어있는 문자는 그냥 넘어간다

			char buf[9] = { 0 };
			char result[9] = { 0 };			

			fputc(i, fp_table);//문자출력
			fputc(strlen(normal_table[i]), fp_table);//허프만비트 길이 출력			
			fputs(normal_table[i], fp_table);//허프만 비트 출력
		}
	}

	fclose(fp_table);//허프만 테이블 파일 close
	return 0;
}

/*minimal cost의 adaptive table을 context_adaptive_huffman_table.hbs에 쓰는 함수*/
int adapt_tablewr(char*** adaptive_table, pair<int, int> adapt_p[][128], int eod, int gonormal) {
	int count;
	bool first = true;
	FILE* fp_table = NULL;
	if (fopen_s(&fp_table, "context_adaptive_huffman_table.hbs", "wb") != 0)//허프만 테이블 파일 열기
		return -1;


	for (int i = 0; i < 128; i++) {
		int count = 0;
		if (adaptive_table[i] != NULL) {//비어있는 문자는 그냥 넘어간다
			
			fputc(i, fp_table);//preceding symbol 출력

		/* symbol의 entry 개수 구하기 */
			for (int j = 0; j < 128; j++)
			{
				if (adapt_p[i][j].first != 0)
					count++;
			}	
			if (first != true) {// eod와 gonormal에 대한 정보를 symbol하나에다가만 남겨두고 나머지 symbol에선 다 지울 것이다.
				count-=2;
				adapt_p[i][eod].first--;
				adapt_p[i][gonormal].first--;
			}
			fputc(count, fp_table);//entry 개수 출력
			
			
			for (int j = 0; j < 128; j++)
			{
				if (adapt_p[i][j].first != 0)
				{
					fputc(j, fp_table);//entry 문자 출력
					fputc((adapt_p[i][j].first), fp_table);//entry 문자의 빈도수 출력			
				}
			}

			first = false;
			
		}
	}

	fclose(fp_table);//허프만 테이블 파일 close
	return 0;
}

/* 코스트 계산 함수*/
double calc_cost(char***table,  char** normal_table, int eod, int gonormal, pair<int, int> adapt_p[][128])
{
	int adapt_byte = 0;
	int temp = 0;
	int letter = 0;
	int char_num = 0;
	int encode_length = 0;
	double cost = 0;
	char** encode = NULL;
	FILE* fp_read = NULL;

	/////여기고쳐
	if (fopen_s(&fp_read, "training_input.txt", "rt") != 0)//트레이닝 파일 오픈
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	temp = fgetc(fp_read);//첫번째 글자 읽기

	for (int i = 0; i < 128; i++)
	{
		if (temp == i) {
			char_num += strlen(normal_table[i]);//첫번째 글자에 대한 허프만 코드를 normal table에서 찾아서 읽기
			break;
		}
	}

	while ((letter = fgetc(fp_read)) != EOF)
	{
		if (table[temp] == NULL)
			char_num += strlen(normal_table[letter]);//만약 preceding symbol에 대한 정보가 adaptive table에 없으면 normal table에서 코드 길이 계산 		
		else 
			char_num += strlen(table[temp][letter]);//정보가 있으면 adaptive table에서 코드길이 계산
		
		temp = letter;//읽은 문자를 preceding symbol로 업데이트
	}
	
	/* 마지막에는 eod의 코드길이를 더해준다 */
	if (table[temp] == NULL)
		char_num += strlen(normal_table[eod]);	
	else
		char_num += strlen(table[temp][eod]);

	fclose(fp_read);

	/* encoding 된 파일의 크기는 8로 나눈 만큼 된다*/
	if (char_num % 8 != 0) {
		encode_length = (char_num / 8) + 1;
	}
	else {
		encode_length = (char_num / 8);
	}


	bool first = true;
	for (int i = 0; i < 128; i++)
	{
		int sym_size = 0;
		if (table[i] == NULL)
			continue;
		for (int j = 0; j < 128; j++)
		{
			if (adapt_p[i][j].first != 0)				
				sym_size++;			//entry의 개수를 구한다.
		}
		if (first != true)
			sym_size-=2;//adaptive table 작성할 때 eod와 gonormal에 대해서는 하나의 symbol에만 저장했으므로 그 symbold을 넘어가면 entry 수에 대한 보정이 필요하다
		adapt_byte += (sym_size * 2) + 2;//하나의 entry당 entry의 문자와 빈도수, 즉 2개의 문자가 들어가고 preceding symbol에 대한 문자와 빈도수 2글자를 더해준다.
		first = false;
	}

	cost = ((double)encode_length / filesize("training_input.txt")) + (0.0001)*(adapt_byte + filesize("huffman_table.hbs"));//cost 계산식
	return cost;
}

/* cost가 최소인 adaptive table 구하는 함수 */
void min_table(char*** original_table, char** normal_table, int eod, int gonormal,  pair<int, int> normal_p[], pair<int, int> adapt_p[][128], priority_queue <node*, vector<node*>, compfreq> hp) {
	int total_freq = 0;
	double entro_expect = 0;
	double min_cost = calc_cost(original_table, normal_table, eod, gonormal, adapt_p);	
	pair<double, char> p[128];
	priority_queue<pair<double, char>, vector<pair<double, char>>, greater<pair<double,char>>> queue;
	
	for (int i = 0; i < 128; i++)
	{
		total_freq = 0;
		entro_expect = 0;
			for (int j = 0; j < 128; j++)
			{
				if (adapt_p[i][j].first != 0)
					total_freq+=(adapt_p[i][j].first);//하나의 symbol의 모든 entry의 빈도 수를 구한다.
			}

			if (total_freq != 0) {
				for (int k = 0; k < 128; k++)
				{
					if (adapt_p[i][k].first != 0)
					{
						/*	각 entry에 대해 entropy를 구하고 계속 더해서 symbol에 대한 entropy를 구한다. */
						entro_expect += ((double)(adapt_p[i][k].first) / total_freq) * ((log(1.0 / ((double)(adapt_p[i][k].first) / total_freq))) / log(2));
					}
				}

				p[i].first = (double)normal_p[i].first/entro_expect;//symbol의 빈도수에서 entropy를 나눠준 값
				p[i].second = i;//symbol의 문자
				queue.push(p[i]);//min heap queue에 집어 넣기, 존재 가치가 낮은 symbol부터 빠져 나오도록 할 것이다.
			}
		}
	
	
	while (queue.size() != 0) {
		char** temp = original_table[queue.top().second];		
		original_table[queue.top().second] = NULL;//큐 내에서 가장 가치가 낮은 symbol의 table을 지운다.
	

		if (min_cost > calc_cost(original_table, normal_table, eod, gonormal, adapt_p)) {//cost 계산
			min_cost = calc_cost(original_table, normal_table, eod, gonormal, adapt_p);	//지워봤더니 cost 더 낮아졌으면 그걸로 min_cost update		
		}

		else
		{
			original_table[queue.top().second] = temp;//cost가 더 올라가거나 그대로면 지웠던 table 다시 복구
		}
		queue.pop();//계산 한 symbol은 pop
	}
}

/* 인코딩 함수 */
int encode(const char* input, const char* output, char*** adaptive_table, char** normal_table, int eod, int gonormal)
{
	FILE* read_file = NULL;
	FILE* encoding = NULL;
	FILE* huf_write = NULL;
	char** compress_arr = NULL;
	int compress_arr_length = 0;
	int letter = 0;
	int temp = 0; 
	int encode_byte = 0;
	if (fopen_s(&read_file,input, "rt") != 0)//input 파일 오픈
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	if (fopen_s(&encoding, "spreadbits.hbs", "wb") != 0)//인코딩용 파일 open
	{
		printf("file doesn't exist!\n");
		return -1;
	}


	/* 문자 계속 읽어들이면서 총 문자 개수 구하고, 문자에 맞는 허프만 비트를 인코딩용 파일에 출력*/
	if ((temp = fgetc(read_file)) != EOF)
		fwrite(normal_table[temp], strlen(normal_table[temp]), 1, encoding);// 첫 글자는 normal table 참조

	while ((letter = fgetc(read_file)) != EOF)
	{
		if (adaptive_table[temp] == NULL)//adaptive table에 symbol이 없으면 normal table 참조
			fwrite(normal_table[letter], strlen(normal_table[letter]), 1, encoding);
		else if (adaptive_table[temp][letter] != NULL)//adaptive table에 존재하면 그대로 참조
				fwrite(adaptive_table[temp][letter], strlen(adaptive_table[temp][letter]), 1, encoding);
		else {//adaptive table에  symbol은 있는데 entry에 대한 정보가 없으면 gonormal 코드 쓰고 그다음에 normal table 코드 값 출력
			fwrite(adaptive_table[temp][gonormal], strlen(adaptive_table[temp][gonormal]), 1, encoding);
			fwrite(normal_table[letter], strlen(normal_table[letter]), 1, encoding);
		}
		
		temp = letter;		//preceding symbol update
	}


	/* eod 마지막에 꼭 쓰기 */
	if (adaptive_table[temp] == NULL)
		fwrite(normal_table[eod], strlen(normal_table[eod]), 1, encoding);
	else
		fwrite(adaptive_table[temp][eod], strlen(adaptive_table[temp][eod]), 1, encoding);
	
	

	fclose(read_file);//input 파일 close
	fclose(encoding);//인코딩용 파일 close

	encode_byte = filesize("spreadbits.hbs");//인코딩용 파일 사이즈 계산

	if (encode_byte % 8 != 0) {//8의배수가 아니면 8로 나눈 몫 + 1 만큼 배열 메모리 할당
		compress_arr = (char**)malloc(sizeof(char*) * ((encode_byte / 8) + 1));
		compress_arr_length = (encode_byte / 8) + 1;
	}
	else {//8의배수이면 그냥 8로 나눈 몫 만큼 메모리 할당
		compress_arr = (char**)malloc(sizeof(char*) * (encode_byte / 8));
		compress_arr_length = (encode_byte / 8) ;
	}

	/*compress_arr의 index마다 메모리 할당*/
	for (int i = 0; i < compress_arr_length; i++)
	{
		compress_arr[i] = (char*)malloc(9);	//8비트씩 저장할 예정	
		compress_arr[i][0] = { 0 };
	}


	if (fopen_s(&encoding, "spreadbits.hbs", "rb") != 0)//인코딩용 파일 열기
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	/* compress_arr 의 길이만큼 8글자씩 배열에 저장*/
	for (int i = 0; i < compress_arr_length; i++)
		fread(compress_arr[i], 8, 1, encoding);

	fclose(encoding);	//인코딩용 파일 close	

	bytealign(compress_arr_length, compress_arr);//byte align

	if (fopen_s(&huf_write, output, "wb") != 0)//허프만코드 파일 열기
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	/* 8bit 2진수 문자열을 숫자로 바꾼 후 하나의 문자로 허프만코드파일에 출력*/
	for (int i = 0; i < compress_arr_length; i++) {
		char send = strtol(compress_arr[i], NULL, 2);
		fwrite(&send, 1, 1, huf_write);
	}

	remove("spreadbits.hbs");
	fclose(huf_write);	//허프만코드 파일 닫기

	/*memory free*/
	for (int i = 0; i < compress_arr_length; i++)
		free(compress_arr[i]);
	free(compress_arr);	

	return 0;
}
/* main */
int main() {	
	FILE* fp_read = NULL;
	priority_queue <node*, vector<node*>, compfreq> hp;
	pair<int, int> normal_p[128];
	pair<int, int> adapt_p[128][128];
	char* normal_table[128] = { 0 };
	char*** adaptive_table = { 0 };			
	int letter;			
	int eod=0;
	int gonormal = 0;

	/* 빈도수 0, 문자값 i로 pair 생성*/
	for (int i = 0; i < 128; i++) {
		normal_p[i] = make_pair(0, i);
		for (int j = 0; j < 128; j++)
			adapt_p[i][j] = make_pair(0, j);
	}

	if (fopen_s(&fp_read, "training_input.txt", "rt") != 0)//training_input.txt 오픈
	{
		printf("file doesn't exist!\n");
		return -1;	}
	

	while ((letter = fgetc(fp_read)) != EOF)//eof일 때 까지 문자 읽으면서 빈도수 갱신	
		(normal_p[letter].first)++;
	
		
	/* eod 값 찾기 */
	for (int i = 0; i < 128; i++)
	{
		if ((normal_p[i].first) == 0)
		{
			eod = i;
		(normal_p[i].first)++;
			break;
		}
	}	

	/* gonormal 값 찾기 */
	for (int i = 0; i < 128; i++)
	{
		if ((normal_p[i].first) == 0)
		{
			gonormal = i;
			(normal_p[i].first)++;
			break;
		}
	}
	
	/* 만든 pair 기반으로 node를 queue에 집어넣기*/
	for (int i = 0; i < 31; i++)//0~31의 ascii 값은 제어문자로, 일반적으로 쓰이지 않기 때문에 혹시라도 training data에 출현한 제어문자가 있다면 그것에 대해서만 노드 생성
	{
		if (normal_p[i].first != 0) {
			node* n = new node;
			n->info = normal_p[i];
			hp.push(n);
		}
	}

	for (int i = 32; i < 127; i++)//32부터는 126까지는 일반적으로 쓰이므로 출현하지 않았더라도 나중에 쓰일 수 있기 때문에 노드를 생성 
	{
		node* n = new node;
		n->info = normal_p[i];
		hp.push(n);				
	}	

	preorder_traversal(makehuffman(hp),NULL, normal_table);//순회 하면서 normal table 제작

	/* adaptive table을 만들기 위해 빈도수가 0이었던 노드는 이제 삭제*/
	for (int i = 0; i < 128; i++)
	{
		if (hp.top()->info.first == 0)
			hp.pop();
		else break;
	}
		
	/*	adaptive_table 메모리 할당	*/	
	adaptive_table = (char***)malloc(sizeof(char**) * 128);
	for (int i = 0; i < 128; i++)
		adaptive_table[i] = { 0 };

	for (int i = 0; i < 128; i++)
	{
		adaptive_table[i] = (char**)malloc(sizeof(char*) * (128));//EOD 포함하여 table 생성
		for(int j=0; j<128; j++)
		adaptive_table[i][j] = { 0 };
	}
	
	fseek(fp_read, 0, SEEK_SET);
	 
	int temp = fgetc(fp_read);

	/* symbol 뒤에 나온 entry에 대해 빈도수 update*/
	while ((letter = fgetc(fp_read)) != EOF)
	{		
		adapt_p[temp][letter].first++;
		temp = letter;		
	}	

	/* gonormal과 eod가 기록 안된 table들의 값 갱신*/
	for (int i = 0; i < 128; i++) {
		for (int j = 0; j < 128; j++)
		{
			if (adapt_p[i][j].first != 0) {
				adapt_p[i][eod].first++;
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
			adaptive_table[i] = NULL;//존재하지 않는 symbol에 대해서는 table nullify
		}
	}
	
	min_table(adaptive_table, normal_table, eod, gonormal, normal_p, adapt_p, hp);//minimum cost table 생성

	/*normal table 파일에 쓰기*/
	if (normal_tablewr(normal_table) == -1) {
		printf("file doesn't exist!\n");
		return -1;
	}

	/*adaptive table 파일에 쓰기*/
	if (adapt_tablewr(adaptive_table, adapt_p, eod, gonormal) == -1) {
		printf("file doesn't exist!\n");
		return -1;
	}
	fclose(fp_read);	
	
	/* 각 input들에 대해서 encoding 파일 만들기 */
	if(encode("training_input.txt", "training_input_code.hbs", adaptive_table, normal_table, eod, gonormal) == -1) {
		printf("file doesn't exist!\n");
		return -1;
	}
	printf("training_input.txt 압축완료! \n");

	if(encode("test_input1.txt", "test_input1_code.hbs", adaptive_table, normal_table, eod, gonormal) == -1) {
		printf("file doesn't exist!\n");
		return -1;
	}
	printf("test_input1.txt 압축완료! \n");

	if(encode("test_input2.txt", "test_input2_code.hbs", adaptive_table, normal_table, eod, gonormal) == -1) {
		printf("file doesn't exist!\n");
		return -1;
	}
	printf("test_input2.txt 압축완료! \n");

	if(encode("test_input3.txt", "test_input3_code.hbs", adaptive_table, normal_table, eod, gonormal) == -1) {
		printf("file doesn't exist!\n");
		return -1;
	}
	printf("test_input3.txt 압축완료! \n");
	
	
	/*memory 해제*/
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