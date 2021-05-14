# Context_adaptive_HuffmanCoding
기존의 허프만 알고리즘보다 더 적응적인 알고리즘을 사용하여 압축률을 향상시킨다.

---

## Explanations

* 기존에 본인이 개발한 일반적인 huffman encoder의 압축률은 대략 40%정도 였음

* 기존에는 각 문자별 출현 빈도를 기반으로 허프만 트리를 구축하여 허프만 코드를 생성하였음

* 텍스트 파일의 경우 현재 문자 앞에 나오는 문자의 빈도수를 기반으로 하면 좀 더 적응적인 압축이 가능함

* 단순히 앞의 문자만 분석하는 것이 아니라 n번째 전부터 패턴을 분석하면 압축률을 더 증가시킬 수 있음

* 압축률의 상승의 tradeoff = table 파일 크기의 증가

* 따라서 테이블 크기와 압축률의 증가율이 혼합된 cost function을 사용해야 한다.

* Cost function을 다음과 같이 정의 가능

![image](https://user-images.githubusercontent.com/67624104/118293165-554cc500-b514-11eb-9c82-2052cf34aee2.png)

---

## Result example

** Reconstruction 결과는 모두 정확히 복원되었음

![image](https://user-images.githubusercontent.com/67624104/118293857-13704e80-b515-11eb-9d83-c4370f7df3c8.png)


![image](https://user-images.githubusercontent.com/67624104/118293537-b5436b80-b514-11eb-8d10-51935d1c66c5.png)

![image](https://user-images.githubusercontent.com/67624104/118293588-c3918780-b514-11eb-9a64-cd2eccfe2ee3.png)
