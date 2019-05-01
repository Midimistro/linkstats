#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

int main(int argc, char *argv[])
{    
	if(argc <= 2) { // if there are too few arguments
		printf("ERROR: Expected at least 2 arguments\n");
		return 0;
	}
	char *ipTo = argv[1];
	int samples = atoi(argv[2]); //int version
	char *sampleSTR = argv[2]; //string version
	printf("Sampling Size set to: %i\n",samples);
	int pingSize = strlen("/bin/ping")+strlen(" ")+strlen(ipTo)+strlen(" ")+strlen("-c ")+strlen(sampleSTR);
	char *pingStr = (char *)malloc(pingSize);
	strcat(pingStr, "/bin/ping");
	strcat(pingStr, " ");
	strcat(pingStr, ipTo);
	strcat(pingStr, " ");
	strcat(pingStr, "-c ");
	strcat(pingStr, sampleSTR);

	/*Debug
	printf("%s\n", pingStr);*/

	FILE *fp;
	char path[1035];

	/*Open the command for reading*/
	fp = popen(pingStr, "r");
	if(fp == NULL){
		printf("ERROR: Failed to run ping command\n");
		return 0;
	}
	
	char *findRTT = "rtt";
	char *findBytesFrom = "bytes from";
	char myRTT[100];
	float tRTT;
	float minRTT;
	float avgRTT;
	float maxRTT;
	float devRTT;
	float mytRTT[samples];	
	float myiQueue[samples];
	int ptCnt = 0;
	printf("instance\tRTT\tQueing Time\n");
	/*Get the min RTT time for queue time calculation*/
	while(fgets(path, sizeof(path)-1,fp) != NULL){
		if(strstr(path,findBytesFrom)!= NULL){
			sscanf (path,"%*s %*s %*s %*s %*s %*s %s",myRTT);
			//printf("My RTT = %s\n", myRTT);
			sscanf (myRTT,"%*c%*c%*c%*c%*c%6f", &tRTT);
			mytRTT [ptCnt] = tRTT;
			if(ptCnt==0){
				myiQueue[ptCnt]=0.00;
			} else {
				myiQueue[ptCnt]=fabs(mytRTT[ptCnt-1]-mytRTT[ptCnt]);
				//printf("%lf\n",myiQueue[ptCnt]);
			}
			printf("%i\t\t%.3f\t%lf\n", (ptCnt+1),mytRTT [ptCnt],myiQueue[ptCnt]);
			ptCnt = ptCnt+1;
		}
		//Get the overall RTT string
		if(strstr(path,findRTT)!=NULL){
			//printf("%s\n",path);
			sscanf (path,"%*s %*s = %s %*s",myRTT);
			//printf("My RTT = %s\n", myRTT);
			sscanf (myRTT,"%6f%*c%6f%*c%6f%*c%6f", &minRTT, &avgRTT, &maxRTT, &devRTT);
		}
	}
	printf("min RTT = %.3f ms\n", minRTT);
	printf("avg RTT = %.3f ms\n", avgRTT);
	printf("max RTT = %.3f ms\n", maxRTT);
	printf("RTT deviation = %.3f ms\n", devRTT);
	if(ptCnt<samples){
		//Notify user if packets dropped
		printf("Packets Dropped = %i of %i\n", samples-ptCnt,samples);
	}
	int i;
	float queueSum=0;
	float queueSize = (sizeof(myiQueue)/sizeof(myiQueue[0]));
	/*Calculate Queuing Delay for each*/
	for (i=1;i < ptCnt+1;i++) { //use ptCnt to only add up what was collected
    		//printf("%lf\n",mytRTT[i]);
		queueSum = queueSum+myiQueue[i];
	}
	float avgQueue = queueSum/(ptCnt-1);//Only calculate the average of what is collected, not size of the array
	printf("Queue Time Array Size = %lf\n",ptCnt); //debugging
	printf("Average Total Queuing Time = %lf ms\n",avgQueue);

	/*Calculate Propegation Delay between the two links*/
	float avgpropDelay = avgRTT - avgQueue;
	printf("Average Total Propegation Time in a single direction= %lf ms\n",avgpropDelay);

	/*traceroute*/
	int tracerouteSize = strlen("/usr/bin/traceroute")+strlen(" ")+strlen(ipTo)+strlen(" ")+strlen("-I 30 -m 100");
	char *tracerouteStr = (char *)malloc(tracerouteSize);
	strcat(tracerouteStr, "/usr/bin/traceroute");
	strcat(tracerouteStr, " ");
	strcat(tracerouteStr, ipTo);
	strcat(tracerouteStr, " ");
	strcat(tracerouteStr, "-I 30 -m 100");

	/*Open the command for reading*/
	fp = popen(tracerouteStr, "r");
	if(fp == NULL){
		printf("ERROR: Failed to run traceroute command\n");
		return 0;
	}
	int hopCount = 0;
	char *tracerouteTo = "traceroute to";
	while(fgets(path, sizeof(path)-1,fp) != NULL){
		if(strstr(path,tracerouteTo)!= NULL){
			//do nothing on trace route first line
		} else {
			//else start hop counting
			hopCount = hopCount+1;
		}	
	}
	printf("Hop Count = %i\n", hopCount);
	float estQueue = avgQueue/hopCount;
	float estpropDelay = avgpropDelay/hopCount;
	printf("Estimated Queue Time per hop = %f ms\n", estQueue);
	printf("Estimated Propegation Delay per hop = %f ms\n", estpropDelay);

	/*tcpstat*/
	int tstatSize = strlen("/usr/bin/tcpstat")+strlen(" -o '%p\n'");
	char *tstatStr = (char *)malloc(tstatSize);
	strcat(tstatStr, "/usr/bin/tcpstat");
	strcat(tstatStr, " -o '%p\n'");
	//printf("%s",tstatStr);


	/*Open the command for reading*/
	char tstat[100];
	int testCount = 0;
	char *listeningOn = "Listening on";
	float pAR;
	float pASS;
	float packetArrivalRate[samples];
	float packetAverageSize[samples];
	fp = popen(tstatStr, "r");
	if(fp == NULL){
		printf("ERROR: Failed to run traceroute command. Is it installed and located under /usr/bin?\n");
		return 0;
	}
	sleep(2); //allow "Listening on" message to post before next string
	printf("instance\tarrival rate (packets/second)\n");//\taverage packet size\n");
	while(fgets(path, sizeof(path)-1,fp) != NULL){
		//printf("path=%s",path);
		sscanf(path,"%4f",&pAR);
		//sscanf(path,"%s",tstat);
		//sscanf(tstat,"%4f %6f",&pAR, &pASS);
		packetArrivalRate[testCount]=pAR;
		packetAverageSize[testCount]=pASS;
		//printf("%i\t%.2f\tpath%i=%s\n",testCount+1,packetArrivalRate[testCount],testCount,path);
		printf("%i\t\t%.2f\n",testCount+1,packetArrivalRate[testCount]);//,packetAverageSize[testCount]);
		if(testCount>=samples-1){
			//close the stream after 20 tests
			pclose(fp);
		}
		testCount = testCount+1;
	}
	int j;
	float arrivalSum=0;
	float sizeSum=0;
	float pARsize = (sizeof(packetArrivalRate)/sizeof(packetArrivalRate[0]));
	/*Calculate the average packet arrival rate, or lambda*/
	for (j=0;j < pARsize;j++) {
		arrivalSum = arrivalSum+packetArrivalRate[j];
	}
	float avgArrivalRate = arrivalSum/samples;
	float avgPacketSize = sizeSum/samples; //in bytes
	printf("Average Arrival Rate (lambda) = %lf packets/second\n",avgArrivalRate);
	//h = lambda*k
	//we assume standard ethernet packet size, 1500 bytes, or 12000 bits
	//we assume the absolute limit of TCP packet size,65535 bytes, or 524280 bits
	float interarrivalRateTCP = avgArrivalRate*524280;
	float interarrivalRateMTU = avgArrivalRate*12000;
	float avgrateTotaldelay = (estQueue+estpropDelay)/1000;// divide by 1000 to convert to seconds
	float oneOverT = 1/avgrateTotaldelay;
	float capacityTCP = oneOverT+interarrivalRateTCP;	
	float capacityMTU = oneOverT+interarrivalRateMTU;
	printf("Estimated Connection MTU Capacity = %lf Mbps\n",capacityMTU/1000000);
	printf("Estimated Connection TCP Capacity = %lf Mbps\n",capacityTCP/1000000);	
}
