#include <stdio.h> 
#include <stdlib.h> 
#include <bits/stdc++.h> 
#include <unistd.h> 
#include <pthread.h>
#include <fstream>
#include <iostream> 
#include <string> 
#include <cstdlib> 
using namespace std; 
fstream outputFile;
pthread_mutex_t locks[10];//mutex for ATM-Customer txn
pthread_mutex_t locks2[10];//mutex for ATM-Customer txn
pthread_mutex_t outLock = PTHREAD_MUTEX_INITIALIZER;//mutex for output
pthread_mutex_t locksBill[5];//mutex for each type of bill
pthread_t ATMthreads[10];//ATM threads
pthread_cond_t emptyATM[10];//condition about if ATM is EMPTY
pthread_cond_t fullATM[10];//condition about if ATM is FULL
struct customerData{ int customerId, sleepTime, ATMorder,billType, amount;	 };
struct customerData ATMUsers[10];//txn ways between ATMs and Customers
int totalBills[5];//total spend of bills
int remainingCustomer[10];//number of remaining customers for each ATM
pthread_t *customerThreads;
struct customerData* values;

void *customer(void* values){
	// sleep part
	struct customerData* cust = (struct customerData* ) values ;
	struct timespec time={cust->sleepTime / 1000,  cust->sleepTime  * 1000000L};
	nanosleep(&time,NULL);
	
	//txn part
    pthread_mutex_lock(&locks[cust->ATMorder]); 
    pthread_mutex_lock(&locks2[cust->ATMorder]); 
	ATMUsers[cust->ATMorder] = *cust;
	pthread_cond_signal(&emptyATM[cust->ATMorder]);
	pthread_cond_wait(&fullATM[cust->ATMorder],&locks2[cust->ATMorder]);
	pthread_mutex_unlock(&locks2[cust->ATMorder]); 
	pthread_mutex_unlock(&locks[cust->ATMorder]); 
	
	pthread_exit(NULL);
}

void *ATM(void* ID){
	int* id = (int*) ID;
	/* prepare part of payment info to write into output file*/
	string billT;//bill type

	while(remainingCustomer[*id]!=0){
		pthread_cond_wait(&emptyATM[*id],&locks2[*id]);
		remainingCustomer[*id]--;
		pthread_mutex_lock(&locksBill[ATMUsers[*id].billType]); 
		totalBills[ATMUsers[*id].billType]+= ATMUsers[*id].amount;
		if(ATMUsers[*id].billType==2)
			billT="gas";
		else if(ATMUsers[*id].billType==1)
			billT="water";
		else if(ATMUsers[*id].billType==0)
			billT="electricity";
		else if(ATMUsers[*id].billType==4)
			billT="cableTV";
		else 
			billT="telecommunication";
		//output part
		pthread_mutex_lock(&outLock);
		string payment = "Customer";
		payment.append(to_string(ATMUsers[*id].customerId));
		payment.append(",");
		payment.append(to_string(ATMUsers[*id].amount));
		payment.append("TL,");
		payment.append(billT);
		outputFile <<payment<<endl;
		pthread_mutex_unlock(&outLock);
		pthread_cond_signal(&fullATM[*id]);
		pthread_mutex_unlock(&locksBill[ATMUsers[*id].billType]); 
	}
	
	pthread_exit(NULL);	//terminate the thread
}

int main(int argc, char **argv){
	void *status;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	//open inputFile
	fstream inputFile;
	inputFile.open(argv[1]);
	string out(argv[1]);
	//if(out.find(".")>out.length())  
	out= out.substr(0,out.find("."))+"_log.txt";
/*	else
		out= out.substr(0,out.find("."))+"_log"+out.substr(out.find("."));*/
	
	outputFile.open(out,ios::out);//open outputfile
	int numCustomer;
	inputFile >> numCustomer;//take the number of customer
	//create/assign customer related structures
	values = new struct customerData[numCustomer];
	customerThreads = new pthread_t [numCustomer];
	
	//prepare customer info in inputfile to store 
	string line,type;
	int billT;
	for(int i=0;i<numCustomer;i++){
		inputFile >> line;
		values[i].customerId=(i+1);
		values[i].sleepTime= stoi(line);
		line = line.substr(line.find(",")+1);
		values[i].ATMorder= stoi(line)-1;
		line = line.substr(line.find(",")+1);
		type=line.substr(0,line.find(","));
		remainingCustomer[values[i].ATMorder]++;
		
		if(type.compare("gas")==0)
			billT=2;
		else if(type.compare("water")==0)
			billT=1;
		else if(type.compare("electricity")==0)
			billT=0;
		else if(type.compare("cableTV")==0)
			billT=4;
		else 
			billT=3;
				
		values[i].billType= billT;
		line = line.substr(line.find(",")+1);
		values[i].amount= stoi(line);
	}
	
	inputFile.close();
	//create ATMs and assign mutex about them
	int id[10];
	for(int i =0;i<10;i++){
		id[i]=i;
		pthread_mutex_init(&locks[i], NULL);
		pthread_cond_init( &emptyATM[i], NULL);
		pthread_cond_init( &fullATM[i], NULL);
		pthread_create(&ATMthreads[i], &attr, ATM,(void*)&id[i]);
	}
	//create customer threads
	for(int i=0;i<numCustomer;i++)
		pthread_create(&customerThreads[i], &attr, customer,(void*)&values[i]);	
	
	/*for(int i=0;i<numCustomer;i++)
		pthread_join(customerThreads[i], &status);*/
	//wait ATM threads  until they are terminated
	for(int i=0;i<10;i++){
		pthread_join(ATMthreads[i], &status);
	}
	//ultimate output part
	pthread_mutex_lock(&outLock);
	outputFile<<"All payments are completed."<<endl;
	outputFile<<"CableTV: "<<totalBills[4]<<"TL"<<endl;
	outputFile<<"Electricity: "<<totalBills[0]<<"TL"<<endl;
	outputFile<<"Gas: "<<totalBills[2]<<"TL"<<endl;
	outputFile<<"Telecommunication: "<<totalBills[3]<<"TL"<<endl;
	outputFile<<"Water: "<<totalBills[1]<<"TL"<<endl;
	pthread_mutex_unlock(&outLock);
	outputFile.close();
	//clean some mutexes
	for(int i =0;i<10;i++){
		pthread_cond_destroy(&emptyATM[i]);
		pthread_mutex_destroy(&locks[i]);
		pthread_mutex_destroy(&locks2[i]);
	}
	
	return 0;
}
