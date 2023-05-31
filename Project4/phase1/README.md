1. 컴파일 방법:
Makefile, 필요한 Source코드(myshell.h, myshell.c, csapp.h, csapp.c)이 모두 들어있는 각 Phase폴더에서 'make'라는 명령어를 통해 컴파일 한다.

2. 실행 방법:
컴파일이 끝난 후 myshell 실행 파일이 생성되고 ./myshell을 통하여 프로그램을 실행한다.


Phase1
Shell Program은 사용자의 명령을 받고, 입력 받은 command에 해당하는 작업을 수행한다. 
사용자가 quit이나 exit를 입력할 때까지 프롬프트를 출력하며 무한루프 안에서 반복적인 입력을 받는다. 
Phase1에서는 cd, ls, mkdir, rmdir, touch, cat, echo, history, exit 명령어를 수행할 수 있다. 
이 명령어들은 기본적으로 원래 기능과 동일한 작업을 수행한다.

‘cd’ 명령어는 현재 작업 디렉토리를 변경할 수 있고, ‘ls’ 명령어는 현재 디렉토리에 있는 파일과 디렉토리의 목록을 출력할 수 있다. 
‘mkdir’와 ‘rmdir’ 명령어는 새 디렉토리를 생성하거나, 빈 디렉토리를 삭제할 수 있다. 
‘touch’ 명령어는 새로운 빈 파일을 생성하거나, 파일의 최종 수정 시간을 업데이트할 수 있고, ‘cat’ 명령어는 파일의 내용을 출력한다. 
‘echo’ 명령어는 주어진 입력을 터미널에 출력하고, ‘history’ 명령어는 실행된 명령어들의 리스트를 출력한다.

Parent process가 사용자의 명령어를 해석하고, 명령은 child process로 실행된다. fork()를 통해 process를 생성하고, 
execve를 사용해 process를 새 program으로 실행된다.

