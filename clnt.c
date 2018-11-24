#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<time.h>
#include<pthread.h>
#include<termios.h>     // 버퍼식 입력 on/off

#define TRUE 1
#define FALSE 0
#define BUF_SIZE 1024

int sock;
char prevstat = 'F';
char gamestat = 'C';     // Connecting Start Waiting
time_t startTime = 0;     // 매 라운드 시작 시간
int users=0;     // 접속한 유저 수
int mynumber;
char myname[10];
char username[4][10];
int score[4] = {0, };

int Set = 1, Round = 1;
int answer[6][3];
int answer_index;
int strike[5];
int ball[5];
struct termios t;     // 버퍼제어용 변수

void error_handling(char *msg);
void initAnswer();     // answer 배열 초기화용
void initSB();
void msgParse(char *msg);     // msg parsing

void *printUI(void *a);     // 쓰레드용
void connectUI();     // 게임 시작 대기화면
void waitUI();     // 다음 라운드 진행 대기
void playUI();     // 게임 진행 화면
void finalUI();     // 최종 결과

void sendAnswer();

void *doInput(void *a);
void bufferOff();
void bufferOn();

void printLine(int n);     // 화면 밀기

int main(int ac, char *av[])
{
	struct sockaddr_in serv_addr;
	char msg[BUF_SIZE];
	int len;
	pthread_t tUI, tInput;     // 화면 출력용 쓰레드

	bufferOn();

	if(ac != 3)
	{
		fprintf(stderr, "Usage : %s <IP> <port>\n", av[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);

	if(sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(av[1]);
	serv_addr.sin_port = htons(atoi(av[2]));

	if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
	
	printLine(22);
	printf("Input your nickname(in 10 char without blank)\n\n\n");
	scanf("%s", myname);
	
	bufferOff();
	initAnswer();
	initSB();

	write(sock, myname, sizeof(myname)-1);
	read(sock, msg, sizeof(msg));
	msgParse(msg);
	mynumber = users;

	pthread_create(&tUI, NULL, printUI, NULL);
	pthread_create(&tInput, NULL, doInput, NULL);

	while((len = read(sock, msg, sizeof(msg))) != 0)
	{
		msgParse(msg);
		if((prevstat == 'F' && gamestat == 'C') || gamestat == 'S')
			time(&startTime);
	}

	pthread_join(tUI, NULL);
	pthread_join(tInput, NULL);

	bufferOn();
	close(sock);

	return 0;
}

void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void initAnswer()
{
	int i;
	answer_index = 0;
	for(i=0; i<6; i++)
	{
		answer[i][0] = -1;
		answer[i][1] = -1;
		answer[i][2] = -1;
	}
}
void initSB()
{
	int i;
	for(i=0; i<5; i++)
	{
		strike[i] = 0;
		ball[i] = 0;
	}
}

void msgParse(char *msg)
{
	prevstat = gamestat;

	if(prevstat == 'W')
	{
		switch(users)
		{
			case 2:
				sscanf(msg, "%c %d %d %d %d %d %d %d", &gamestat, &users, &Set, &Round, &score[0], &score[1], &strike[Round-1], &ball[Round-1]);
				break;
			case 3:
				sscanf(msg, "%c %d %d %d %d %d %d %d %d", &gamestat, &users, &Set, &Round, &score[0], &score[1], &score[2], &strike[Round-1], &ball[Round-1]);
				break;
			case 4:
				sscanf(msg, "%c %d %d %d %d %d %d %d %d %d", &gamestat, &users, &Set, &Round, &score[0], &score[1], &score[2], &score[3], &strike[Round-1], &ball[Round-1]);
				break;
		}
		if(Round == 1)
			initSB();
	}
	else if(prevstat == 'C')
	{
		sscanf(msg, "%c %d %ld", &gamestat, &users, &startTime);
		if(gamestat == 'S')
			sscanf(msg, "%c %d %s %s %s %s", &gamestat, &users, username[0], username[1], username[2], username[3]);
	}
}

void *printUI(void *a)
{
	while(1)
	{
		if(gamestat == 'C')
			connectUI();
		else if(gamestat == 'F')
		{
			if(Set == 3 && Round == 5)
				finalUI();
			else
				error_handling("Connection closed...");
		}
		else if(gamestat == 'S')	
			playUI();
		else if(gamestat == 'W' && gamestat != 'F')
			waitUI();
	}
}

void connectUI()
{
	time_t currentTime;
	if(startTime == 0)
		return ;
	time(&currentTime);
	
	printLine(23);
	printLine(10);
	printf("\t\t\t\tconnecting %ld...\n\n", currentTime-startTime);
	printf("\t\t\t\tUsers  :  %d\n", users);
	printLine(10);
	usleep(1000*200);
}

void waitUI()
{
	int i = 0;

	while(i < 5)
	{
		printLine(23);
		printLine(10);
		printf("\t\t\t\twaiting...\n\n");
		printf("\t\t\t\t   %d", 5-i);
		printLine(10);
		i++;
		usleep(1000*500);
	}
}

void playUI()
{
	time_t currentTime;
	int i, j;
	int correct = FALSE;

	while(time(&currentTime) - 15<= startTime)
	{
		printLine(23);
		printf("NUMBER BASEBALL GAME\t\t  ");
		// 시간
		if(currentTime - startTime > 10)
			printf("TIME : \x1b[31m%ld\x1b[0m", 15 - (currentTime-startTime));
		else
			printf("TIME : %ld", 15 - (currentTime-startTime));
		printf("\t\t\t Round  %d  (%d/5)", Set, Round);
		printLine(4);
		// 점수판
		for(i=0; i<users; i++)
			printf("%-10s%4d\n", username[i], score[i]);
		for(i=1; i<Round; i++)
		{

			printf("\t\t\t\t\t\t\t\t\t");
			for(j=0; j<3; j++)
			{
				if(answer[i][j] == -1)
					printf("_  ");
				else if(j == 2)
					printf("%d\n", answer[i][j]);
				else
					printf("%d  ", answer[i][j]);
			}
			printf("\t\t\t\t\t\t\t\t\t %dS  %dB\n", strike[i-1], ball[i-1]);
		}
		printLine(10-2*(Round-1));
		// 입력 상태
		printf("\t\t\t\t    ");
		
		for(i=0; i<3; i++)
		{
			if(answer[Round][i] != -1)
				printf("%d ", answer[Round][i]);
			else
				printf("_ ");
		}
		
		printLine(9-users);
		sleep(1);
	}
	gamestat = 'W';
	sendAnswer();
	if(Round == 5)
		initAnswer();
}

void finalUI()
{
	int i;
	int Index = -1;
	int Max = 0;

	for(i=0; i<users; i++)
	{
		if(score[i] > Max)
		{
			Max = score[i];
			Index = i;
		}
	}
	for(i=0; i<10; i++)
	{
		printLine(23);
		printLine(10);
		if(Index == -1)
			printf("\t\t\tNo body ...\n\n");
		else
		{
			printf("\t\t\t\tWINNER\n\n");
			printf("\t\t\t\t  %s", username[Index]);
		}
		printLine(10);
		sleep(1);
	}

	exit(1);
}

void sendAnswer()
{
	char temp[10];
	sprintf(temp, "%d %d %d", answer[Round][0], answer[Round][1], answer[Round][2]);
	write(sock, temp, sizeof(temp)-1);
}

void *doInput(void *a)
{
	int temp;
	int i, S_flag;

	while(1)
	{
		S_flag = 0;
		if(gamestat == 'F')
			break;
		if(gamestat == 'W')
			continue;
		for(i=0; i<Round; i++)
			if(strike[i] == 3)
				S_flag = 1;
		if(S_flag == 1)
			continue;
		temp = getchar();
		if(temp >= 48 && temp <= 57)
			answer[Round][(answer_index++) % 3] = temp - 48;
	}
}

void bufferOff()
{
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag &= ~ICANON;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void bufferOn()
{
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag |= ICANON;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void printLine(int n)
{
	int i;
	for(i=0; i<n; i++)
		printf("\n");
}
