#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#define MSG_CONFIRM 0
#define TRUE 1
#define FALSE 0


typedef struct list{
	int size;
	int arr[20];	
}list;


int connect_to_port (int connect_to)
{
  int sock_id;
  int opt = 1;
  struct sockaddr_in server;
  if ((sock_id = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
    {
      perror ("unable to create a socket");
      exit (EXIT_FAILURE);
    }
  setsockopt (sock_id, SOL_SOCKET, SO_REUSEADDR, (const void
						  *) &opt, sizeof (int));
  memset (&server, 0, sizeof (server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons (connect_to);
  if (bind (sock_id, (const struct sockaddr *) &server, sizeof (server)) < 0)
    {
      perror ("unable to bind to port");
      exit (EXIT_FAILURE);
    }
  return sock_id;
}


void send_to_id (int to, int id, list llist)
{
  struct sockaddr_in cl;
  memset (&cl, 0, sizeof (cl));
  cl.sin_family = AF_INET;
  cl.sin_addr.s_addr = INADDR_ANY;
  cl.sin_port = htons (to);
  sendto (id, &llist, sizeof(list), MSG_CONFIRM, (const struct sockaddr *) &cl, sizeof (cl));
}


void election (int id, int self,int next)
{
  list llist;
  llist.arr[0]=self;
  llist.size=1;
  printf ("Forwading to: %d\n", next);
  send_to_id (next, id, llist);
	 
}

void print(list l)
{
	for(int i=0;i<l.size;i++)
	{
		printf("%d ",l.arr[i]);
	}
}

int main (int argc, char *argv[])
{

	  int self = atoi (argv[1]);
	  int sock_id, bully_id;
	  int itr, len, n, start_at;  
	  int next=atoi(argv[3]);
	  
	  struct sockaddr_in from;
	  
	  start_at = atoi (argv[2]) == 1 ? TRUE : FALSE;
	
	  printf ("creating a node at %d %d \n\n", self, start_at);
	  sock_id = connect_to_port (self);
		
		list l;	
		
	  if(start_at == TRUE)
	  {
		  election (sock_id,self,next);
	  }

	  while (TRUE)
	  {
		  memset (&from, 0, sizeof (from));
		  n = recvfrom (sock_id, &l, sizeof(list), MSG_WAITALL,(struct sockaddr *) &from, &len);
		  
		  if(l.arr[0]==-1)
		  {
		  		printf("New Co-ordinator: %d\n",l.arr[1]);
		  		if(l.arr[2]==next)
		  			break;
		  		send_to_id (next, sock_id, l);
		  		break;
		  }
		  else if(l.arr[0]==self)
		  {
		  		int max=l.arr[0];
		  		for(int i=1;i<l.size;i++)
		  		{
		  			if(l.arr[i]>max)
		  				max=l.arr[i];
		  		}
		  		l.arr[0]=-1;
		  		l.arr[1]=max;
		  		l.arr[2]=self;
		  		printf("New Co-ordinator: %d\n",l.arr[1]);
		  		send_to_id (next, sock_id, l);
		  		break;
		  }
		  
		  else{
			printf("Received list: ");
			print(l);
			printf("\n\n");
		  	l.arr[l.size]=self;
		  	l.size++;
		  	printf("Sending: ");
		  	print(l);
		  	printf(" To %d\n\n",next);
		  	send_to_id (next, sock_id, l);
		  }
		  
	  }
	  
}
