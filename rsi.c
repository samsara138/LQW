#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>


void bg_work(char *list[],int bglist[],int *index,char pathlist[][80]){
	pid_t pid;
  pid = fork(); //create 2 processes
	if (pid == 0) //child process
	{
		sleep(1);
    execvp(list[0], list);
		if (execvp(list[0], list) < 0) {
			printf("Error: execution of %s failed \n", list[0]);
			exit(-1);
		}
	}else
	{
    int status;
		char *last_part=NULL;
		bglist[*index]=pid;
		last_part = strrchr(list[0], '/');
		char buf[80];
 	 	getcwd(buf, sizeof(buf));
		strcat(buf,last_part);
		for(int i=0;i<80;i++){
			pathlist[*index][i]=buf[i];
		}
		*index=*index+1;
		waitpid(pid,&status,WNOHANG);
	}
}

void bg_list(int bglist[],char pathlist[][80],int index){
	int total_job=0;
	for(int i=0;i<index;i++){
		if(bglist[i]!=-1){
			printf("%d : %s\n",bglist[i],pathlist[i]);
			total_job++;
		}
	}
	printf("Total background jobs: %d\n",total_job);
}

void bg_signal_control(int pid,int bglist[],char *part,int index){
	int position;
	for(int i=0;i<index;i++){
		if(bglist[i]==pid){
			position=i;
			break;
		}
	}
	if(strcmp(part,"bgkill")==0){
		kill(bglist[position],SIGTERM);
	}else if(strcmp(part,"bgstop")==0){
		kill(bglist[position],SIGSTOP);
	}else{// the restart process
		kill(bglist[position],SIGCONT);
	}
}

int main(){
	int status;
  int promot_flag=0; //check out if the promote should be show
  char *input = NULL;
  char *prompt=NULL;
  char *lastest_pwd=NULL; //the val we use to store the current prompt
	char *part=NULL;
  char *temp_prompt=NULL;
	int bglist[1000];
	for(int i=0;i<1000;i++){
		bglist[i]=0;
	}
	int max_index_bglist=0;
	char pathlist[1000][80];
  while(1){
    if(promot_flag==0){
      prompt = "your command:";
    }else{
      prompt = temp_prompt;
    }
    input = readline(prompt);
    char* new_list[strlen(input)]; //the list that storing data for excep() used
    if(strcmp(input,"exit")==0){ // exit the progarm when needed
      exit(0);
    }
    lastest_pwd = strrchr(input, '/');
    part = strtok(input, " ");
    ///////////////////////////////////////////////////////////////
    if(part!=NULL&&strcmp(part,"bg")==0){// it is a bg work
			int index=0;
			// we need to strtok a more time to make sure we dont take  "bg" in our argument
			while(part!=NULL){
				part = strtok(NULL, " ");
				new_list[index]=part;
				index++;
			}
			new_list[index]=NULL;//last index NULL for termination
			bg_work(new_list,bglist,&max_index_bglist,pathlist);
    }
		else if((part!=NULL&&strcmp(part,"bglist")==0)){
			bg_list(bglist,pathlist,max_index_bglist);
		}else if(part!=NULL&&(strcmp(part,"bgkill")==0||strcmp(part,"bgstop")==0||strcmp(part,"bgstart")==0)){
			char *temp_pid_num=strtok(NULL, " ");
			int pid_num=atoi(temp_pid_num);
			bg_signal_control(pid_num,bglist,part,max_index_bglist);
		}
		else if((part!=NULL&&strcmp(part,"pstat")==0)){
			char *pstat_pid_number=strtok(NULL, " ");
			char* pstat_list[5];
			pstat_list[0]="ps";
			pstat_list[1]="-o";
			pstat_list[2]="comm,state,utime,stime,rss,nvcsw,nivcsw";
			pstat_list[3]=pstat_pid_number;
			pstat_list[4]=NULL;
			/////////

			pid_t pid;
			pid = fork(); //create 2 processes
			if (pid == 0) //child process
			{
				execvp(pstat_list[0], pstat_list);
			}else
			{
				waitpid(pid,0,0); // wait for the child process
			}
			/////////

		}else{
      if(lastest_pwd!=NULL)
      {
      strcat(lastest_pwd+1,": >");
      promot_flag=1;
      temp_prompt=lastest_pwd+1;
      }else{
      printf("command not found\n");
      }
      }
    ////////////////////////////////////////////////////////////////
		int status;
		int retVal = 0;
		while(1) {
			usleep(1000);
			retVal = waitpid(-1, &status, WNOHANG);
			if(retVal > 0) {
				for(int i=0;i<max_index_bglist;i++){
			  if(bglist[i]==retVal){
					if(WIFSIGNALED(status)){
						printf("Process %d is killed by signal\n",bglist[i]);
						bglist[i]=-1;
					}
			 	 if(WIFEXITED(status)){
			 	 		bglist[i]=-1;
			  	}
			  }
			}
		}
			else{
				break;
			}
		}
  }
    return 1;
}
