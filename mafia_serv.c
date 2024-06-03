#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <signal.h>

#define BUF_SIZE 1024
#define MAX_CLNT 8
#define NAMESIZE 20
char* game_waiting = "입장하였습니다. 게임이 곧 시작됩니다...\n";
char* game_start = "8명 전원 입장 완료. 게임을 시작합니다...\n";
char* out_sign = "QuitCode_0";
char* job_selection = "직업을 정하는 중입니다.\n";
char* job_finished = "직업이 모두 정해졌습니다. 본인의 직업을 확인해주세요.\n";
char* day_msg = "낮입니다. 2분간 자유롭게 토론해주시기 바랍니다.\n";
char* vote_msg = "투표 시간입니다. 해커로 의심되는 플레이어의 번호를 '#N#' 형식으로 입력해주십시오. 30초간 투표하지 않으면 기권처리됩니다.\n";
char* defense_msg = "최후 변론입니다. 마지막으로 자신을 변호하십시오.\n";
char* bq_msg = "선택된 플레이어를 처형하려면 '#q#'를, 그렇지 않다면 '#b#'를 입력해주십시오. 30초간 투표하지 않으면 기권처리됩니다.\n";
char* night_msg = "밤이 되었습니다. 각자 자신의 능력을 사용할 상대를 '#N#' 형식으로 입력해주십시오. 30초 후 밤이 끝납니다.\n";
char* result_msg = "지난 밤에 일어난 일들을 알려드리겠습니다.\n";
char* invalid_msg = "잘못된 입력입니다. 다시 입력해주세요.\n";
char* already_msg = "이미 선택을 완료하셨습니다.\n";
char* vote_none = "과반수의 표가 없으므로 아무도 선택되지 않습니다.\n";
char* select_done = "선택이 완료되었습니다.\n";

typedef enum{
	HACKER,
	WHITEHACKER,
	V3,
	USER
}JOB;

typedef struct person{
	char name[NAMESIZE];
	bool life;
	bool protected;
	JOB job;
}person;

const char *jobname[]={
	"HACKER",
	"WHITEHACKER",
	"V3",
	"USER"
};

const char* ability_tell[] = {
   "당신은 [해커]입니다. 다른 플레이어의 방화벽을 뚫어 상대방을 무력화시킬 수 있습니다! \n",
   "당신은 [화이트해커]입니다. 플레이어 한 명을 선택해 해당 플레이어의 직업을 알 수 있습니다!\n",
   "당신은 [V3]입니다. 플레이어 한 명을 선택해 해커의 공격으로부터 지켜줄 수 있습니다 !\n",
   "당신은 [유저]입니다. [화이트해커], [V3]와 함께 [해커]를 찾아 투표로 처형시키세요!\n"
};


person table[MAX_CLNT];

void * handle_clnt(void * arg);
void send_msg(char * msg, int len, int sock);
void error_handling(char * msg);
int accounting(int* leftlist);
int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
int leftone[4] = {2, 1, 1, 4};
int turn = 0;
bool init = true;
bool dissfl = false;
bool votefl = false;
bool deffl = false;
bool bqfl = false;
bool nightfl = false;
void phase_discuss();
void phase_vote();
void phase_defense();
void phase_bq();
void phase_night();
void phase_resulting();
int set_ticker(int ms, int next);
pthread_mutex_t mutx;
struct itimerval nset;
int ability[MAX_CLNT];
int vote[MAX_CLNT];
bool vres[MAX_CLNT];
bool bqres[MAX_CLNT];
unsigned int bq[2] = {0, MAX_CLNT};
void sendall(char* msg);
char* inputcheck(char* input, char* res);
int usernum();
int bqman = -1;
void bq_exe();
void vote_finish();
void function();
int main(int argc, char *argv[])
{
	for(int i = 0; i < MAX_CLNT; i++){
		table[i].life = true;
		table[i].protected = false;
		strcpy(table[i].name, "[DEFAULT]");
		table[i].job = 3;
	}
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
	srand(time(NULL));
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
  
	pthread_mutex_init(&mutx, NULL);
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);
	//server side socket
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET; 
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));
	//bind & listen
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock, 8)==-1)
		error_handling("listen() error");
	//managing client socket connection
	while(clnt_cnt < MAX_CLNT)
	{
		clnt_adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);
		
		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_cnt]=clnt_sock;
		table[clnt_cnt++].life = true;
		pthread_mutex_unlock(&mutx);
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
		pthread_detach(t_id);
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
		printf("%d번째 사용자 입장\n", clnt_cnt);
	}
	pthread_mutex_lock(&mutx);
	//start message
	for(int i = 0; i < MAX_CLNT; i++){
		if(table[i].life){
			write(clnt_socks[i], game_start, strlen(game_start));
		}
	}
	sleep(1);
	//job selection message
	for(int i = 0; i < MAX_CLNT; i++){
		write(clnt_socks[i], job_selection, strlen(job_selection));
	}
	//job selection(by random)
	for(int i = 0; i < 2; ){
		int hack = rand() % MAX_CLNT;
		if(table[hack].job == 3 && i < 2){
			table[hack].job = 0;
			i++;
		}
	}
	for(int i = 0; i < 1; ){
		int white = rand() % MAX_CLNT;
		if(table[white].job == 3 && i < 1){
			table[white].job = 1;
			i++;
		}
	}
	for(int i = 0; i < 1; ){
		int v3 = rand() % MAX_CLNT;
		if(table[v3].job == 3 && i < 1){
			table[v3].job = 2;
			i++;
		}
	}
	sleep(1);
	//notifying their job
	for(int i = 0; i < MAX_CLNT; i++){
		write(clnt_socks[i], job_finished, strlen(job_finished));
	}
	for(int i = 0; i < MAX_CLNT; i++){
		write(clnt_socks[i], ability_tell[table[i].job], strlen(ability_tell[table[i].job]));
	}
	sleep(1);
	pthread_mutex_unlock(&mutx);
	//game start. accounting current survivors by turna
	int res_account;
	while((res_account = accounting(leftone)) == 0){
		if(init){
			printf("%d턴\n", turn+1);
			phase_discuss();
		}
	//	turn++;
	}
	if(res_account == -1) sendall("[해커]의 승리입니다!\n");
	else if(res_account == 1) sendall("[유저]의 승리입니다!\n");
	sendall(out_sign);
	close(serv_sock);
	return 0;
}
	
void * handle_clnt(void * arg)
{
	int clnt_sock=*((int*)arg);
	int str_len=0, i;
	char msg[BUF_SIZE];
	while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0){
		char buf[BUF_SIZE] = "";
		int socknum = -1;
		for(int j = 0; j < MAX_CLNT; j++)
			if(clnt_socks[j] == clnt_sock){
			       socknum = j;
			       break;
			}
		if(!table[socknum].life) continue;
		if(strcmp(table[socknum].name, "[DEFAULT]") == 0){
			pthread_mutex_lock(&mutx);
			strcpy(table[socknum].name, msg);
			pthread_mutex_unlock(&mutx);
			printf("사용자 %d번 닉네임 : %s\n", socknum+1, table[socknum].name);
		}
		else if(votefl){
			//msg compare
			if(!table[socknum].life) continue;
			pthread_mutex_lock(&mutx);
			inputcheck(msg, buf);
			if(buf == NULL)
				write(clnt_socks[socknum], invalid_msg, strlen(invalid_msg));
			
			else if(vres[socknum] == false){
				if(isdigit(buf[0]) && strlen(buf) == 1){
					int v = atoi(buf)-1;
					if(0 <= v && v <= 7 && table[v].life){
					       vote[v]++;
					       vres[socknum] = true;
					}
				}
				else{
					for(int i = 0; i < MAX_CLNT; i++)
						if(strcmp(table[i].name, buf) == 0 && table[i].life){
							vote[i]++;
							vres[socknum] = true;
						}
				}
				if(!vres[socknum]) write(clnt_socks[socknum], invalid_msg, strlen(invalid_msg));
				else write(clnt_socks[socknum], select_done, strlen(select_done));
				//없는 사람 투표하면 기권표? 재투표?
			}
			else write(clnt_socks[socknum], already_msg, strlen(already_msg));
			pthread_mutex_unlock(&mutx);
			//add vote[]
		}
		else if(!votefl && bqfl){
			if(!table[socknum].life) continue;
			pthread_mutex_lock(&mutx);
			//msg compare
			inputcheck(msg, buf);
			if(bqres[socknum] == false && strcmp(buf, "q") == 0){
				bqres[socknum] = true;
				bq[0]++;
				bq[1]--;
			}
			if(bqres[socknum]) write(clnt_socks[socknum], select_done, strlen(select_done));
			pthread_mutex_unlock(&mutx);
			//add bq[]
			//bq[0] = q(kill) and bq[1] = b(save)
		}
		else if(!votefl && !bqfl && nightfl){
			if(!table[socknum].life) continue;
			pthread_mutex_lock(&mutx);
			inputcheck(msg, buf);
			if(buf == NULL) write(clnt_socks[socknum], invalid_msg, strlen(invalid_msg));
			else if(ability[socknum] == -1){
				if(isdigit(buf[0]) && strlen(buf) == 1){
					int v = atoi(buf)-1;
					if(0 <= v && v <= 7 && table[socknum].life)
						ability[socknum] = v;
				}
				else{
					for(int i = 0; i < MAX_CLNT; i++)
						if(strcmp(table[i].name, buf) == 0 && table[socknum].life)
							ability[socknum] = i;
				}
				write(clnt_socks[socknum], select_done, strlen(select_done));
			}
			else write(clnt_socks[socknum], already_msg, strlen(already_msg));
			pthread_mutex_unlock(&mutx);
			//msg compare
			//function
			//just collect their ability
		}
		else send_msg(msg, str_len, socknum);
	}
	
	close(clnt_sock);
	return NULL;
}
void send_msg(char * msg, int len, int sock)   // send with writers' number
{
	int i;
	pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++){
		char tmp[len+3+strlen(table[sock].name)];
		sprintf(tmp, "[%d]", sock+1);
		strcpy(tmp+3, msg);
		write(clnt_socks[i], tmp, len+3);
	}
	pthread_mutex_unlock(&mutx);
}
void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
int accounting(int* leftlist){
	int imposter = leftlist[0];
	int civil = 0;
	for(int i = 1; i < 4; i++) civil += leftlist[i];
	if(imposter == 0) return 1;
	else if(imposter >= civil) return -1;
	else return 0;
}
void phase_discuss(){
	init = false;
	signal(SIGALRM, phase_vote);
	sendall(day_msg);
	dissfl = true;
	alarm(120);
	/*
	struct itimerval it;
	int tmp;
	if(( tmp = set_ticker(120000, 30000)) == -1){
		perror("set_ticker");
	}
	bool check = false;
	while(1){
		if((tmp = getitimer(ITIMER_REAL, &it))==-1) perror("getitimer");
		if(it.it_value.tv_sec == 30 && !check){
			char mess[30];
			strcpy(mess, "30초 남았습니다.");
			sendall(mess);
			check = true;
		}
	}*/
	while(dissfl);
}
void phase_vote(){//get vote via handle_clnt
	signal(SIGALRM, phase_defense);
	dissfl = false;
	for(int i = 0; i < MAX_CLNT; i++){
		vote[i] = 0;
		vres[i] = false;
	}
	if(turn == 0){
		signal(SIGALRM, phase_night);
		alarm(1);
		votefl = true;
		while(votefl);
	}
	else{
		sendall(vote_msg);
		alarm(30);
		votefl = true;
		while(votefl);
	}
}
void phase_defense(){//just chat for defense
	for(int i = 0; i < MAX_CLNT; i++) bqres[i] = false;
	bq[0] = 0;
	bq[1] = usernum();
	votefl = false;
	bqman = -1;
	deffl = true;
	vote_finish();
	if(bqman == -1){
		signal(SIGALRM, phase_night);
		alarm(1);
		while(deffl);
	}
	else{
		signal(SIGALRM, phase_bq);
		sendall(defense_msg);
		alarm(30);
		while(deffl);
	}
}
void phase_bq(){//get vote via handle
	//initiate bq by leftone
	signal(SIGALRM, phase_night);
	deffl = false;
	bqfl = true;
	sendall(bq_msg);
	alarm(30);
	while(bqfl);
}
void phase_night(){//bq execute and get ability usage via handle_clnt
	for(int i = 0; i < MAX_CLNT; i++) ability[i] = -1;
	signal(SIGALRM, phase_resulting);
	alarm(30);
	bq_exe();
	if(accounting(leftone) != 0) return;
	deffl = false;
	votefl = false;
	bqfl = false;
	nightfl = true;
	sendall(night_msg);
	while(nightfl);
}
void phase_resulting(){//cancel ticker and show result of night phase
	sendall(result_msg);
	function();
	turn++;
	nightfl = false;
	init = true;
	if(accounting(leftone) != 0) return;
	//phase_discuss();
	//process function.c
}
int set_ticker(int ms, int next){
	long s, us;
	s = ms/1000;
	us = (ms%1000)*1000L;
	long ns, nus;
	ns = next/1000;
	nus = (next%1000)*1000L;

	nset.it_interval.tv_sec = ns;
       	nset.it_value.tv_sec = s;
	nset.it_interval.tv_usec = nus;
       	nset.it_value.tv_usec = us;

	return setitimer(ITIMER_REAL, &nset, NULL);
}
void sendall(char* msg){
	pthread_mutex_lock(&mutx);
	for(int i = 0; i < MAX_CLNT; i++) write(clnt_socks[i], msg, strlen(msg));
	pthread_mutex_unlock(&mutx);
}
char* inputcheck(char* input, char* res){
	char *next;
	char* ptr = strtok_r(input, "#", &next);
	ptr = strtok_r(NULL, "#", &next);
	if(ptr == NULL) return NULL;
	strcpy(res, ptr);
	return res;
}
int usernum(){
	int num = 0;
	for(int i = 0; i < MAX_CLNT; i++)
		if(table[i].life) num++;

	return num;
}
void vote_finish(){
	char result[1024] = {0};
	char* headline = "투표 완료\n";
	int total = usernum()/2 + usernum()%2;
	int voted = -1;
	int votes = 0;
	for(int i = 0; i < MAX_CLNT; i++){
		if(vote[i] >= total && vote[i] > votes){
			voted = i;
			votes = vote[i];
			bqman = voted;
		}
	}
	sendall(headline);
	if(voted != -1){
		sprintf(result, "투표 결과 : %s, %d표\n", table[voted].name, votes);
		sendall(result);

		//table[voted].life = false;
		//leftone[table[voted].job]--;
	//	sprintf(result, "%s는 %s였습니다!", table[voted].name, jobname[table[voted].job]);
	//	sendall(result);
		sprintf(result, "남은 인원 : %d명\n", usernum());
		sendall(result);
	}
	else{
		for(int i = 0; i < MAX_CLNT; i++){
			if(vote[i] != 0){
				sprintf(result, "%s : %d표\n", table[i].name, vote[i]);
				sendall(result);
			}
		}
		sendall(vote_none);
	}
	return;
}
void function(){
	int len = leftone[0];
	int kill[len];
	for(int i = 0; i < len; i++) kill[i] = -1;
	int idx = 0;
	for(int i = 0; i < MAX_CLNT; i++) printf("%d번 참가자 : %d번에게 사용\n", i+1, ability[i]+1);
	for(int i = 0; i < MAX_CLNT; i++){//hacker abil
		if(table[i].job == 0 && table[i].life){
			if(ability[i] == -1) continue;
			else if(table[ability[i]].life){
				kill[idx] = ability[i];
				table[kill[idx]].life = false;
				idx++;
			}
		}
	}
	for(int i = 0; i < MAX_CLNT; i++){//v3 abil
		if(table[i].job == 2){
			if(table[i].life){
				if(ability[i] != -1){
					for(int j = 0; j < len; j++){
						int t = kill[j];
						if(t != -1 && t == ability[i]){
							table[t].life = true;
							kill[j] = -1;
							break;
						}
					}
				}
			}
			else{
				if(ability[i] == -1);
				else if(ability[i] == i){
					for(int j = 0; j < len; j++) if(kill[j] == i) kill[j] = -1;
					table[i].life = true;
				}
				else;
			}
			break;
		}
	}
	for(int i = 0; i < MAX_CLNT; i++){
		if(table[i].job == 1 && ability[i] != -1){
			char msg[BUF_SIZE];
			sprintf(msg, "%s님은 [%s]입니다.\n", table[ability[i]].name, jobname[table[ability[i]].job]);
			pthread_mutex_lock(&mutx);
			write(clnt_socks[i], msg, strlen(msg));
			pthread_mutex_unlock(&mutx);
			break;
		}
	}
	for(int i = 0; i < len; i++) printf("kill : %d\n", kill[i]+1);
	for(int i = 0; i < len; i++){
		if(kill[i] != -1){
			char msg[BUF_SIZE];
			sprintf(msg, "%s님이 무력화되었습니다.\n", table[kill[i]].name);
			sendall(msg);
			leftone[table[kill[i]].job]--;
		}
	}
	return;
}
void bq_exe(){
	float total = usernum()/2;
	float kill = (float)bq[0];
	if(bqman != -1)printf("%f\n", kill);
	if(bqman != -1 && kill>total){
		sendall("과반수 이상의 찬성입니다. 무력화를 진행합니다.\n");
		table[bqman].life = false;
		leftone[table[bqman].job]--;
		char tmp[BUF_SIZE];
		sprintf(tmp, "%s님의 정체는 %s였습니다!\n", table[bqman].name, jobname[table[bqman].job]);
		sendall(tmp);
	}
	else if(bqman != -1 && kill <= total){
		sendall("과반수 이상의 반대입니다. 무력화를 진행하지 않습니다.\n");
	}
}
