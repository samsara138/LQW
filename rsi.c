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
		if(bglist[i]!=-1){ //we don't print the process that has been done or terminated
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
			char *temp_pid_num=strtok(NULL, " ");
			int pid=atoi(temp_pid_num);
			int flag_pid=0;
			for(int i=0;i<max_index_bglist;i++){
				if(bglist[i]==pid){
					flag_pid=1;
				}
			}
			if(flag_pid==0){
				printf("Error: Process %d does not exist.\n",pid);
			}else{
			char stat[256];
			sprintf(stat, "/proc/%d/stat", pid);
			FILE* store = fopen(stat, "r"); //read the /proc/%d/stat first to get information
			if(store != NULL){
				char read = 0;
				char data[100];
				int command_position = 1;
				read = fscanf(store, "%s", data);
				while(read != EOF){ //in this file,we know the exact number icon of each option, so we could use a index++ coming with the read to know when is the time to print data
				if(command_position == 2){
					printf("comm:%s\n", data);
				}
				if(command_position == 3){
					printf("state:%s\n", data);
				}
				if(command_position == 14){
					float utime = atof(data)/sysconf(_SC_CLK_TCK);
					printf("utime:%f\n", utime);
				}
				if(command_position == 15){
					float stime = atof(data)/sysconf(_SC_CLK_TCK);
					printf("stime:%f\n", stime);
				}
				if(command_position == 24){
					printf("rss:%s\n", data);
				}
				command_position++;
				read = fscanf(store, "%s", data);
			}
			fclose(store);
			}
			//////////
			store = NULL;  //now we need the ctxt information , we reset it first
			char status[256];
			sprintf(status, "/proc/%d/status", pid);
			store = fopen(status, "r");
			if(store != NULL){
				char data[100];
				while(fgets(data, sizeof(data), store)){
					char* part = strtok(data, "\t");
					if(strcmp(part, "voluntary_ctxt_switches:") == 0){
						printf("voluntary_ctxt_switches:\t%s\n", strtok(strtok(NULL, "\t"), "\n"));
					}
					else if(strcmp(part, "nonvoluntary_ctxt_switches:") == 0){
						printf("nonvoluntary_ctxt_switches:\t%s\n", strtok(strtok(NULL, "\t"), "\n"));
					}
				}
				fclose(store);
			}
		}
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
						bglist[i]=-1; // indicate that the process is killed
					}
			 	 if(WIFEXITED(status)){
			 	 		bglist[i]=-1; // indicate that the process is done
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
