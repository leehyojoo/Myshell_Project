1. 컴파일 방법:
Makefile, 필요한 Source코드(myshell.h, myshell.c, csapp.h, csapp.c)이 모두 들어있는 각 Phase폴더에서 'make'라는 명령어를 통해 컴파일 한다.

2. 실행 방법:
컴파일이 끝난 후 myshell 실행 파일이 생성되고 ./myshell을 통하여 프로그램을 실행한다.


Phase3
Phase3의 Shell Program에서는 job control 기능을 제공하여 백그라운드에서 실행되는 프로세스를 제어할 수 있다. 이를 통해 메모리 누수를 방지하고 Foreground와 Background에서 모두 프로세스를 실행할 수 있게 된다.

Background에서 실행하고자 하는 명령어 끝에 ‘&’ 기호를 붙이면 해당 명령어가 백그라운드에서 실행되며, ‘jobs’ 명령어를 사용하여 백그라운드에서 실행 중인 job들의 상태를 확인할 수 있다.

‘bg’ 명령어를 사용하여 중지된 백그라운드 job을 실행 상태로 변경하고, ‘fg’ 명령어를 사용하여 백그라운드 job을 Foreground로 변경할 수 있다.

또한 ‘kill’ 명령어를 사용하여 job을 종료시킬 수 있다.
Ctrl-c를 입력하면 Foreground job의 각 프로세스에 SIGINT 시그널이 전달되어 해당 프로세스가 종료된다. Ctrl-z를 입력하면 Foreground job의 각 프로세스에 SIGTSTP 시그널이 전달되어 해당 프로세스가 중지 상태가 되며, SIGCONT 시그널을 받으면 다시 실행된다.
