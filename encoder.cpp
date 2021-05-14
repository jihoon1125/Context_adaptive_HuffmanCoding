#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <string.h>
#include <math.h>
#include "common_func.h"


/* byte align �Լ� */
void bytealign(int length, char** arr)
{
	for (int i = 0; i < 8; i++)
	{
			if (arr[length - 1][i] != '0' && arr[length - 1][i] != '1') {//�ش� ��Ʈ�� 0�� 1�� �ƴ� ���
				arr[length - 1][i] = '0';								// �� ��Ʈ�� 0���� �������� ä��������.
			}		
	}	
	arr[length - 1][8] = NULL;//������ ��Ʈ�� NULL
}

/*normal table�� huffman_table.hbs�� ���� �Լ�*/
int normal_tablewr(char **normal_table) {
	FILE* fp_table = NULL;
	if (fopen_s(&fp_table, "huffman_table.hbs", "wb") != 0)//������ ���̺� ���� ����
		return -1;
	

	for (int i = 0; i < 128; i++) {
		if (normal_table[i] != NULL) {//����ִ� ���ڴ� �׳� �Ѿ��

			char buf[9] = { 0 };
			char result[9] = { 0 };			

			fputc(i, fp_table);//�������
			fputc(strlen(normal_table[i]), fp_table);//��������Ʈ ���� ���			
			fputs(normal_table[i], fp_table);//������ ��Ʈ ���
		}
	}

	fclose(fp_table);//������ ���̺� ���� close
	return 0;
}

/*minimal cost�� adaptive table�� context_adaptive_huffman_table.hbs�� ���� �Լ�*/
int adapt_tablewr(char*** adaptive_table, pair<int, int> adapt_p[][128], int eod, int gonormal) {
	int count;
	bool first = true;
	FILE* fp_table = NULL;
	if (fopen_s(&fp_table, "context_adaptive_huffman_table.hbs", "wb") != 0)//������ ���̺� ���� ����
		return -1;


	for (int i = 0; i < 128; i++) {
		int count = 0;
		if (adaptive_table[i] != NULL) {//����ִ� ���ڴ� �׳� �Ѿ��
			
			fputc(i, fp_table);//preceding symbol ���

		/* symbol�� entry ���� ���ϱ� */
			for (int j = 0; j < 128; j++)
			{
				if (adapt_p[i][j].first != 0)
					count++;
			}	
			if (first != true) {// eod�� gonormal�� ���� ������ symbol�ϳ����ٰ��� ���ܵΰ� ������ symbol���� �� ���� ���̴�.
				count-=2;
				adapt_p[i][eod].first--;
				adapt_p[i][gonormal].first--;
			}
			fputc(count, fp_table);//entry ���� ���
			
			
			for (int j = 0; j < 128; j++)
			{
				if (adapt_p[i][j].first != 0)
				{
					fputc(j, fp_table);//entry ���� ���
					fputc((adapt_p[i][j].first), fp_table);//entry ������ �󵵼� ���			
				}
			}

			first = false;
			
		}
	}

	fclose(fp_table);//������ ���̺� ���� close
	return 0;
}

/* �ڽ�Ʈ ��� �Լ�*/
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

	/////�������
	if (fopen_s(&fp_read, "training_input.txt", "rt") != 0)//Ʈ���̴� ���� ����
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	temp = fgetc(fp_read);//ù��° ���� �б�

	for (int i = 0; i < 128; i++)
	{
		if (temp == i) {
			char_num += strlen(normal_table[i]);//ù��° ���ڿ� ���� ������ �ڵ带 normal table���� ã�Ƽ� �б�
			break;
		}
	}

	while ((letter = fgetc(fp_read)) != EOF)
	{
		if (table[temp] == NULL)
			char_num += strlen(normal_table[letter]);//���� preceding symbol�� ���� ������ adaptive table�� ������ normal table���� �ڵ� ���� ��� 		
		else 
			char_num += strlen(table[temp][letter]);//������ ������ adaptive table���� �ڵ���� ���
		
		temp = letter;//���� ���ڸ� preceding symbol�� ������Ʈ
	}
	
	/* ���������� eod�� �ڵ���̸� �����ش� */
	if (table[temp] == NULL)
		char_num += strlen(normal_table[eod]);	
	else
		char_num += strlen(table[temp][eod]);

	fclose(fp_read);

	/* encoding �� ������ ũ��� 8�� ���� ��ŭ �ȴ�*/
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
				sym_size++;			//entry�� ������ ���Ѵ�.
		}
		if (first != true)
			sym_size-=2;//adaptive table �ۼ��� �� eod�� gonormal�� ���ؼ��� �ϳ��� symbol���� ���������Ƿ� �� symbold�� �Ѿ�� entry ���� ���� ������ �ʿ��ϴ�
		adapt_byte += (sym_size * 2) + 2;//�ϳ��� entry�� entry�� ���ڿ� �󵵼�, �� 2���� ���ڰ� ���� preceding symbol�� ���� ���ڿ� �󵵼� 2���ڸ� �����ش�.
		first = false;
	}

	cost = ((double)encode_length / filesize("training_input.txt")) + (0.0001)*(adapt_byte + filesize("huffman_table.hbs"));//cost ����
	return cost;
}

/* cost�� �ּ��� adaptive table ���ϴ� �Լ� */
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
					total_freq+=(adapt_p[i][j].first);//�ϳ��� symbol�� ��� entry�� �� ���� ���Ѵ�.
			}

			if (total_freq != 0) {
				for (int k = 0; k < 128; k++)
				{
					if (adapt_p[i][k].first != 0)
					{
						/*	�� entry�� ���� entropy�� ���ϰ� ��� ���ؼ� symbol�� ���� entropy�� ���Ѵ�. */
						entro_expect += ((double)(adapt_p[i][k].first) / total_freq) * ((log(1.0 / ((double)(adapt_p[i][k].first) / total_freq))) / log(2));
					}
				}

				p[i].first = (double)normal_p[i].first/entro_expect;//symbol�� �󵵼����� entropy�� ������ ��
				p[i].second = i;//symbol�� ����
				queue.push(p[i]);//min heap queue�� ���� �ֱ�, ���� ��ġ�� ���� symbol���� ���� �������� �� ���̴�.
			}
		}
	
	
	while (queue.size() != 0) {
		char** temp = original_table[queue.top().second];		
		original_table[queue.top().second] = NULL;//ť ������ ���� ��ġ�� ���� symbol�� table�� �����.
	

		if (min_cost > calc_cost(original_table, normal_table, eod, gonormal, adapt_p)) {//cost ���
			min_cost = calc_cost(original_table, normal_table, eod, gonormal, adapt_p);	//�����ô��� cost �� ���������� �װɷ� min_cost update		
		}

		else
		{
			original_table[queue.top().second] = temp;//cost�� �� �ö󰡰ų� �״�θ� ������ table �ٽ� ����
		}
		queue.pop();//��� �� symbol�� pop
	}
}

/* ���ڵ� �Լ� */
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
	if (fopen_s(&read_file,input, "rt") != 0)//input ���� ����
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	if (fopen_s(&encoding, "spreadbits.hbs", "wb") != 0)//���ڵ��� ���� open
	{
		printf("file doesn't exist!\n");
		return -1;
	}


	/* ���� ��� �о���̸鼭 �� ���� ���� ���ϰ�, ���ڿ� �´� ������ ��Ʈ�� ���ڵ��� ���Ͽ� ���*/
	if ((temp = fgetc(read_file)) != EOF)
		fwrite(normal_table[temp], strlen(normal_table[temp]), 1, encoding);// ù ���ڴ� normal table ����

	while ((letter = fgetc(read_file)) != EOF)
	{
		if (adaptive_table[temp] == NULL)//adaptive table�� symbol�� ������ normal table ����
			fwrite(normal_table[letter], strlen(normal_table[letter]), 1, encoding);
		else if (adaptive_table[temp][letter] != NULL)//adaptive table�� �����ϸ� �״�� ����
				fwrite(adaptive_table[temp][letter], strlen(adaptive_table[temp][letter]), 1, encoding);
		else {//adaptive table��  symbol�� �ִµ� entry�� ���� ������ ������ gonormal �ڵ� ���� �״����� normal table �ڵ� �� ���
			fwrite(adaptive_table[temp][gonormal], strlen(adaptive_table[temp][gonormal]), 1, encoding);
			fwrite(normal_table[letter], strlen(normal_table[letter]), 1, encoding);
		}
		
		temp = letter;		//preceding symbol update
	}


	/* eod �������� �� ���� */
	if (adaptive_table[temp] == NULL)
		fwrite(normal_table[eod], strlen(normal_table[eod]), 1, encoding);
	else
		fwrite(adaptive_table[temp][eod], strlen(adaptive_table[temp][eod]), 1, encoding);
	
	

	fclose(read_file);//input ���� close
	fclose(encoding);//���ڵ��� ���� close

	encode_byte = filesize("spreadbits.hbs");//���ڵ��� ���� ������ ���

	if (encode_byte % 8 != 0) {//8�ǹ���� �ƴϸ� 8�� ���� �� + 1 ��ŭ �迭 �޸� �Ҵ�
		compress_arr = (char**)malloc(sizeof(char*) * ((encode_byte / 8) + 1));
		compress_arr_length = (encode_byte / 8) + 1;
	}
	else {//8�ǹ���̸� �׳� 8�� ���� �� ��ŭ �޸� �Ҵ�
		compress_arr = (char**)malloc(sizeof(char*) * (encode_byte / 8));
		compress_arr_length = (encode_byte / 8) ;
	}

	/*compress_arr�� index���� �޸� �Ҵ�*/
	for (int i = 0; i < compress_arr_length; i++)
	{
		compress_arr[i] = (char*)malloc(9);	//8��Ʈ�� ������ ����	
		compress_arr[i][0] = { 0 };
	}


	if (fopen_s(&encoding, "spreadbits.hbs", "rb") != 0)//���ڵ��� ���� ����
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	/* compress_arr �� ���̸�ŭ 8���ھ� �迭�� ����*/
	for (int i = 0; i < compress_arr_length; i++)
		fread(compress_arr[i], 8, 1, encoding);

	fclose(encoding);	//���ڵ��� ���� close	

	bytealign(compress_arr_length, compress_arr);//byte align

	if (fopen_s(&huf_write, output, "wb") != 0)//�������ڵ� ���� ����
	{
		printf("file doesn't exist!\n");
		return -1;
	}

	/* 8bit 2���� ���ڿ��� ���ڷ� �ٲ� �� �ϳ��� ���ڷ� �������ڵ����Ͽ� ���*/
	for (int i = 0; i < compress_arr_length; i++) {
		char send = strtol(compress_arr[i], NULL, 2);
		fwrite(&send, 1, 1, huf_write);
	}

	remove("spreadbits.hbs");
	fclose(huf_write);	//�������ڵ� ���� �ݱ�

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

	/* �󵵼� 0, ���ڰ� i�� pair ����*/
	for (int i = 0; i < 128; i++) {
		normal_p[i] = make_pair(0, i);
		for (int j = 0; j < 128; j++)
			adapt_p[i][j] = make_pair(0, j);
	}

	if (fopen_s(&fp_read, "training_input.txt", "rt") != 0)//training_input.txt ����
	{
		printf("file doesn't exist!\n");
		return -1;	}
	

	while ((letter = fgetc(fp_read)) != EOF)//eof�� �� ���� ���� �����鼭 �󵵼� ����	
		(normal_p[letter].first)++;
	
		
	/* eod �� ã�� */
	for (int i = 0; i < 128; i++)
	{
		if ((normal_p[i].first) == 0)
		{
			eod = i;
		(normal_p[i].first)++;
			break;
		}
	}	

	/* gonormal �� ã�� */
	for (int i = 0; i < 128; i++)
	{
		if ((normal_p[i].first) == 0)
		{
			gonormal = i;
			(normal_p[i].first)++;
			break;
		}
	}
	
	/* ���� pair ������� node�� queue�� ����ֱ�*/
	for (int i = 0; i < 31; i++)//0~31�� ascii ���� ����ڷ�, �Ϲ������� ������ �ʱ� ������ Ȥ�ö� training data�� ������ ����ڰ� �ִٸ� �װͿ� ���ؼ��� ��� ����
	{
		if (normal_p[i].first != 0) {
			node* n = new node;
			n->info = normal_p[i];
			hp.push(n);
		}
	}

	for (int i = 32; i < 127; i++)//32���ʹ� 126������ �Ϲ������� ���̹Ƿ� �������� �ʾҴ��� ���߿� ���� �� �ֱ� ������ ��带 ���� 
	{
		node* n = new node;
		n->info = normal_p[i];
		hp.push(n);				
	}	

	preorder_traversal(makehuffman(hp),NULL, normal_table);//��ȸ �ϸ鼭 normal table ����

	/* adaptive table�� ����� ���� �󵵼��� 0�̾��� ���� ���� ����*/
	for (int i = 0; i < 128; i++)
	{
		if (hp.top()->info.first == 0)
			hp.pop();
		else break;
	}
		
	/*	adaptive_table �޸� �Ҵ�	*/	
	adaptive_table = (char***)malloc(sizeof(char**) * 128);
	for (int i = 0; i < 128; i++)
		adaptive_table[i] = { 0 };

	for (int i = 0; i < 128; i++)
	{
		adaptive_table[i] = (char**)malloc(sizeof(char*) * (128));//EOD �����Ͽ� table ����
		for(int j=0; j<128; j++)
		adaptive_table[i][j] = { 0 };
	}
	
	fseek(fp_read, 0, SEEK_SET);
	 
	int temp = fgetc(fp_read);

	/* symbol �ڿ� ���� entry�� ���� �󵵼� update*/
	while ((letter = fgetc(fp_read)) != EOF)
	{		
		adapt_p[temp][letter].first++;
		temp = letter;		
	}	

	/* gonormal�� eod�� ��� �ȵ� table���� �� ����*/
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
			adaptive_table[i] = NULL;//�������� �ʴ� symbol�� ���ؼ��� table nullify
		}
	}
	
	min_table(adaptive_table, normal_table, eod, gonormal, normal_p, adapt_p, hp);//minimum cost table ����

	/*normal table ���Ͽ� ����*/
	if (normal_tablewr(normal_table) == -1) {
		printf("file doesn't exist!\n");
		return -1;
	}

	/*adaptive table ���Ͽ� ����*/
	if (adapt_tablewr(adaptive_table, adapt_p, eod, gonormal) == -1) {
		printf("file doesn't exist!\n");
		return -1;
	}
	fclose(fp_read);	
	
	/* �� input�鿡 ���ؼ� encoding ���� ����� */
	if(encode("training_input.txt", "training_input_code.hbs", adaptive_table, normal_table, eod, gonormal) == -1) {
		printf("file doesn't exist!\n");
		return -1;
	}
	printf("training_input.txt ����Ϸ�! \n");

	if(encode("test_input1.txt", "test_input1_code.hbs", adaptive_table, normal_table, eod, gonormal) == -1) {
		printf("file doesn't exist!\n");
		return -1;
	}
	printf("test_input1.txt ����Ϸ�! \n");

	if(encode("test_input2.txt", "test_input2_code.hbs", adaptive_table, normal_table, eod, gonormal) == -1) {
		printf("file doesn't exist!\n");
		return -1;
	}
	printf("test_input2.txt ����Ϸ�! \n");

	if(encode("test_input3.txt", "test_input3_code.hbs", adaptive_table, normal_table, eod, gonormal) == -1) {
		printf("file doesn't exist!\n");
		return -1;
	}
	printf("test_input3.txt ����Ϸ�! \n");
	
	
	/*memory ����*/
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