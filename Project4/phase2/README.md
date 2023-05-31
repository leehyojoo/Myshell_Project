1. 컴파일 방법:
Makefile, 필요한 Source코드(myshell.h, myshell.c, csapp.h, csapp.c)이 모두 들어있는 각 Phase폴더에서 'make'라는 명령어를 통해 컴파일 한다.

2. 실행 방법:
컴파일이 끝난 후 myshell 실행 파일이 생성되고 ./myshell을 통하여 프로그램을 실행한다.


Phase2
Phase2에서는 이전에 구현한 phase1에 파이프라인(pipe) 기능이 추가되었다. 
Shell Program에서는 파이프 라인에 연결된 각각의 명령어를 처리하기 위해 새로운 자식 프로세스와 파이프를 생성한다. 
파이프를 통해 앞쪽 명령의 출력을 뒤쪽 명령의 입력으로 전달한다. 
명세서에 나와있는 예제를 보면, "ls -al | grep filename" 명령은 "ls -al"의 결과 중 "filename"이라는 이름을 가진 파일의 정보만을 출력한다. 
"cat filename | less" 명령은 "filename"이라는 파일의 내용을 "less"를 이용하여 보여준다. 
마지막으로, "cat filename | grep -v “abc” | sort -r" 명령은 "filename"이라는 파일에서 "abc" 패턴이 없는 행만을 출력하고, 
그 결과를 역순으로 정렬하여 출력할 수 있다.