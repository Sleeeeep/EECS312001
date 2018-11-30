#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define TRUE 1
#define FALSE 0

fd_set reads, cpyreads;

time_t startTime;
char gamestat = 'C';
int ball[4];
int strike[4];
int users = 0;				// number of connecting users
char namelist[4][BUFSIZ];
int connecting[4];
int score[4];
int answer[3][3];
int answer_check[4];
int rounds = 1;
int set = 1;
char result_msg[BUFSIZ];
int final_flag = FALSE;

void error_handling(char *);
void make_msg(void);
void make_result_msg(int, int, int);
int parse_score(int, int, int);
void *time_check(void *);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	struct timeval timeout;

	socklen_t addrsize;
	int fdmax, str_len, fdnum;

	char myname[BUFSIZ];
	char buf[BUFSIZ];

	int a, b, c;
	int i, j, k;

	int clnt_score;
	int clnt_ans[4] = {0};
	
	pthread_t tTime;
	pthread_t tResult;

	if(argc != 2)
	{
		printf("Usage: %s<port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

	if(listen(serv_sock, 5) == -1)
		error_handling("listen() error");
	
	pthread_create(&tTime, NULL, time_check, NULL);

	FD_ZERO(&reads);
	FD_SET(serv_sock, &reads);
	fdmax = serv_sock;

	srand(time(NULL));
	for(a = 0 ; a < 3 ; a++)
	{
		for(b = 0 ; b < 3 ; b++)
		{
			answer[a][b] = rand() % 9;
			for(c = 0 ; c < b ; c++)
			{
				if(answer[a][c] == answer[a][b])
				{
					b--;
					continue;
				}
			}
		}
	}
	
	for(a=0; a<3; a++)
	{
		for(b=0; b<3; b++)
			printf("%d ", answer[a][b]);
		printf("\n");
	}
	while(1)
	{
		cpyreads = reads;
		timeout.tv_sec = 5;
		timeout.tv_usec = 5000;

		if((fdnum = select(fdmax + 1, &cpyreads, 0, 0, &timeout)) == -1)
			break;
		
		if(fdnum == 0)
			continue;

		for(i = 0 ; i < fdmax + 1 ; i++)
		{
			if(FD_ISSET(i, &cpyreads))
			{
				if(i == serv_sock)
				{
					addrsize = sizeof(clnt_adr);
					clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &addrsize);
					if(clnt_sock == -1)
						error_handling("accept() error");
					FD_SET(clnt_sock, &reads);

					if(fdmax < clnt_sock)
						fdmax = clnt_sock;
							
					connecting[users] = clnt_sock;
					
					read(clnt_sock, myname, BUFSIZ);
					if(users == 0)
						time(&startTime);
					printf("user number: %d   ", users);
					printf("new player : %s\n", myname);
					strcpy(namelist[users++], myname);
					make_msg();
				}

				else
				{
					str_len = read(i, buf, BUFSIZ);
					if(str_len == 0)
					{
						FD_CLR(i, &reads);
						close(i);
						printf("close client : %d\n", i);
						for(j=0; j<4; j++)
						{
							if(connecting[j] == i)
							{
								for(k=j; k<3; k++)
								{
									connecting[k] = connecting[k+1];
									strcpy(namelist[k], namelist[k+1]);
								}
								connecting[3] = 0;
								users--;
								break;
							}
						}
						if(users == 0)
							gamestat = 'F';
						if(gamestat != 'S')
							make_msg();
					}
					else
					{
						for(j = 0; j < 4; j++)
						{
							if(connecting[j] == i)
							{
								printf("Answer from %d : %s\n\n", j, buf);
								answer_check[j] = 1;
								
								strike[j] = 0;
								ball[j] = 0;
								sscanf(buf, "%d %d %d", &clnt_ans[0], &clnt_ans[1], &clnt_ans[2]);
								for(a = 0 ; a < 3 ; a++)
								{
									if(answer[set-1][a] == clnt_ans[a])
										strike[j]++;
								}
								
								for(a = 0 ; a < 3 ; a++)
								{
									for(b = 0 ; b < 3 ; b++)
									{
										if(a == b)
											continue;
										if(clnt_ans[a] == answer[set-1][b])
											ball[j]++;
									}
								}

								if(strike[j] == 3)
									score[j] += 100 - 10 * (rounds-1);

								break;
							}
						}
					}
				}
			}
		}
	}

	pthread_join(tTime, NULL);

	close(serv_sock);

	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void make_msg(void)
{
	int index = 0, usernum = 0, i = 0;
	int usercnt = users;
	int str_len;
	char makemsg[BUFSIZ] = "\0";
	char timetemp[100];
	
	sprintf(timetemp, " %ld", startTime);

	if(gamestat == 'C')
		makemsg[index++] = 'C';
	else if(gamestat == 'S')
		makemsg[index++] = 'S';
	else if(gamestat == 'F')
		makemsg[index++] = 'F';

	if(gamestat != 'F')
	{
		makemsg[index++] = ' ';
		makemsg[index++] = users+48;
	}

	if(gamestat == 'S')
	{
		while(usercnt)
		{
			makemsg[index++] = ' ';
			index += strlen(namelist[usernum]);
			strcat(makemsg, namelist[usernum]);
	
			usercnt--;
			usernum++;
		}
		makemsg[index++] = ' ';
		makemsg[index] = '\0';
	}

	if(gamestat == 'C')
		strcat(makemsg, timetemp);

	str_len = strlen(makemsg);
	
	printf("msg to All : %s\n", makemsg);
	// 모든 클라이언트들에게
	for(i=0; i<4; i++)
		if(FD_ISSET(connecting[i], &reads))
			write(connecting[i], makemsg, str_len + 1);

	if(gamestat == 'F')
		gamestat = 'C';
}

void make_result_msg(int j, int strike, int ball)
{
	int k, l;
	int index = 0, str_len;
	
	if(final_flag)
		result_msg[index++] = 'F';
	else if(!final_flag)
		result_msg[index++] = 'S';
	result_msg[index++] = ' ';
	result_msg[index++] = users + 48;
	result_msg[index++] = ' ';
	result_msg[index++] = set + 48;
	result_msg[index++] = ' ';
	result_msg[index++] = rounds + 48;
	result_msg[index++] = ' ';
	for(k = 0 ; k < users ; k++)
	{
		index = parse_score(k, index, final_flag);
		result_msg[index++] = ' ';
	}
	result_msg[index++] = strike + 48;
	result_msg[index++] = ' ';
	result_msg[index++] = ball + 48;
	result_msg[index] = '\0';

	str_len = strlen(result_msg);

	write(connecting[j], result_msg, str_len + 1);
	printf("%s\n", result_msg);

}
int parse_score(int j, int index, int final_flag)
{
	int i;
	int score_len;
	int tempScore = score[j];
	
	if(tempScore >= 100)
	{
		result_msg[index++] = tempScore / 100 + 48;
		tempScore = tempScore - (tempScore / 100) * 100;
		result_msg[index++] = tempScore / 10 + 48;
		tempScore = tempScore - (tempScore / 10) * 10;
	}
	else if(tempScore >= 10)
	{
		result_msg[index++] = tempScore / 10 + 48;
		tempScore = tempScore - (tempScore / 10) * 10;
	}
	result_msg[index++] = tempScore + 48;

	return index;
}

void *time_check(void *a)
{
	time_t currentTime;
	int i, cnt;

	while(1)
	{
		if(users > 0 && gamestat == 'C')
		{
			time(&currentTime);
			if(currentTime - startTime >= 30)
			{
				if(users > 1)
				{
					gamestat = 'S';
					make_msg();
				}
				else
				{
					gamestat = 'F';
					make_msg();
				}
			}
		}
		else if(users > 0 && gamestat == 'S')
		{
			cnt = 0;
			for(i=0; i<users; i++)
			{
				if(answer_check[i] == 1)
					cnt++;
			}
			if(cnt == users)
			{
				if(set < 3)
				{
					if(rounds == 5)
					{
						rounds = 1;
						set++;
					}
					else
						rounds++;
				}
				else
				{
					if(rounds == 5)
						final_flag = TRUE;
					else
						rounds++;
				}

				for(i=0; i<users; i++)
				{
					make_result_msg(i, strike[i], ball[i]);
					answer_check[i] = 0;
					strike[i] = 0;
					ball[i] = 0;
				}

			}
		}
	}
}
