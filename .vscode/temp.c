
#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define master 0
#define MAX(a, b) ((a) > (b) ? a : b)
#define MILLION 1000000L

#define bug printf("trust\n");

int *kmp(char *target, char *pattern, int *table, int myrank);
int *kmptable(char *pattern, int len);





// key func kmp
int *kmp(char *target, char *pattern, int *table, int myrank)
{
    int n = strlen(target);
    int m = strlen(pattern);
    int *answer = (int *)calloc(n - m + 1, sizeof(int));
    int j = 0;
    int i = 0;
    int index = 0;
    while (i < n)
    {
        if (pattern[j] == target[i])
        {
            j++;
            i++;
        }
        if (j == m)
        {
            printf("Start Point: %d.\n", i - j);
            answer[index++] = i - j;
            j = table[j - 1];
        }
        else if (i < n && pattern[j] != target[i])
        {
            if (j != 0)
                j = table[j - 1];
            else
                i++;
        }
    }
    return answer;
}


void processTarget(char target[], char pattern[], int *kmp_table, 
int *prefixTable, int rank, int offset,int targetOffSet)
{
    
    int n = strlen(target);
    int m = strlen(pattern);
    int i = 0;
    int j = offset;
    while (i < n && j < m)
    {
        if (target[i] == pattern[j])
        {
            j += 1;
            prefixTable[i + targetOffSet] = j;
            i += 1;
        }
        else if (j > 0)
        {
            j = kmp_table[j - 1];
        }
        else
        {
            prefixTable[i + targetOffSet] = 0;
            i += 1;
        }
    }
}

int main(int argc, char **argv)
{
    int i, j;
    int n;
    int m;

    struct timespec start1, end1;
    double diff;

    char target[] = "ABABDABACDABABCABAB";
    char pattern[] = "ABABCABAB";

    n = strlen(target);
    m = strlen(pattern);
    int tag = 1;
    int tag2 = 2;

    int *kmp_table = kmptable(pattern, m);
    int myrank, nproc;
    MPI_Init(&argc, &argv);                 // initialize the MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); // number of processes that given in command line
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);  // ranks the process rank

    MPI_Status status;

    int start_pos = n / nproc;
    int str_per_proc = n / nproc;
    int end = str_per_proc + n % nproc;

    printf("process %d and startPos:%d,str_per_proc:%d,end:%d\n", myrank, start_pos, str_per_proc, end);

    char send_msg[MAX(m - 1, end)]; // 4-1 = 3
    char recv_msg[MAX(m - 1, end)];
    char end_msg[MAX(m - 1, end)];

    double start, stop;
    if(myrank==nproc-1){
       start = MPI_Wtime();
    }
    if (myrank == master)
    {
        printf("Result KMP Sequential\n");
        start = MPI_Wtime();
        /**
         * Broadcasting to other processes
         */
        for (i = 1; i < nproc; i++)
        {
            if (i == nproc - 1)
            {
                /**
                 * The last process will recieve the odd offset
                 */
                strlcpy(send_msg, target + i * start_pos, end + 1);
                //	printf("proc:%d and pattern:%s\n",i,send_msg);

                MPI_Send(send_msg, end, MPI_CHAR, i, tag, MPI_COMM_WORLD);
            }
            else
            {
                strlcpy(send_msg, target + i * start_pos, str_per_proc + 1);
                //	printf("proc:%d and pattern:%s\n",i,send_msg);

                MPI_Send(send_msg, str_per_proc, MPI_CHAR, i, tag, MPI_COMM_WORLD);
            }
        }
        strlcpy(send_msg, target, str_per_proc + 1);
       // printf("My rank is %d and target %s\n", myrank, send_msg);
        // The last process
    }
    else if (myrank == nproc - 1)
    {
        MPI_Recv(recv_msg, end, MPI_CHAR, master, tag, MPI_COMM_WORLD, &status);
       // printf("My rank is %d and target %s\n", myrank, recv_msg);
    }
    else
    {
        MPI_Recv(recv_msg, str_per_proc, MPI_CHAR, master, tag, MPI_COMM_WORLD, &status);
      //  printf("My rank is %d and target %s\n", myrank, recv_msg);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    /**
     *  ^^ Broadcasting and recieving done
     */


    int step = 1;
    int number;
    int sum= myrank;
    while (step < nproc) {
        int partner = myrank ^ step; // Bitwise XOR with step
        if(myrank<partner){
           MPI_Recv(&number, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
           sum+=number; 
        }else{
            MPI_Send(&sum, 1, MPI_INT, partner, 0, MPI_COMM_WORLD);
        }
        step = step * 2;
    }




    if (myrank == 0)
    {
        int *prefixTable = (int *)malloc(str_per_proc * sizeof(int));
        int sizeOfPrefixTable = str_per_proc;
        processTarget(send_msg, pattern, kmp_table, prefixTable, myrank, 0,0);
        MPI_Send(prefixTable, sizeOfPrefixTable, MPI_INT, myrank + 1, tag2, MPI_COMM_WORLD);
        free(prefixTable);
    }
    else if (myrank == nproc - 1)
    {

        int bufferSizeFromPrevProc;
        MPI_Probe(myrank - 1, tag2, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_INT, &bufferSizeFromPrevProc);
        int *prefixTable = (int *)malloc(((myrank)*str_per_proc + end) * sizeof(int));
        MPI_Recv(prefixTable, bufferSizeFromPrevProc, MPI_INT, myrank - 1, tag2, MPI_COMM_WORLD, &status);
        int indexFromPrevProc = prefixTable[bufferSizeFromPrevProc - 1];
        

        
       processTarget(recv_msg, pattern, kmp_table, prefixTable, myrank, indexFromPrevProc,bufferSizeFromPrevProc);
        int sizeOfPrefixTable = ((myrank)*str_per_proc + end);
      

        int i = 0;
        int value = -1;
        while (i < sizeOfPrefixTable)
        {
            if (prefixTable[i] == m)
            {
                value = i;
                break;
            }
            i++;
        }
        if (value != -1)
        {
            printf(" the pattern exists at %d\n", value+1-m);
        }
        stop = MPI_Wtime();
        double timeDifference = (stop-start) * MILLION;
        printf("Time Elapsed: %0.05f\n",timeDifference);
        free(prefixTable);
    }
    else if (myrank != nproc - 1)
    {
        int bufferSizeFromPrevProc;
        MPI_Probe(myrank - 1, tag2, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_INT, &bufferSizeFromPrevProc);
        int *prefixTable = (int *)malloc((myrank + 1) * str_per_proc * sizeof(int));
        int sizeOfPrefixTable = (myrank + 1) * str_per_proc;
        MPI_Recv(prefixTable, bufferSizeFromPrevProc, MPI_INT, myrank - 1, tag2, MPI_COMM_WORLD, &status);
        int indexFromPrevProc = prefixTable[bufferSizeFromPrevProc - 1];
        processTarget(recv_msg, pattern, kmp_table, prefixTable, myrank, indexFromPrevProc,bufferSizeFromPrevProc);
        MPI_Send(prefixTable, sizeOfPrefixTable, MPI_INT, myrank + 1, tag2, MPI_COMM_WORLD);
          free(prefixTable);
    }


    MPI_Barrier(MPI_COMM_WORLD);
    free(kmp_table);
    MPI_Finalize();

    return 0;
}

int *kmptable(char *pattern, int len)
{
    int k = 0;
    int i = 1;
    int *table = (int *)malloc(len * sizeof(int));
    table[0] = 0;
    while (i < len)
    {
        if (pattern[k] == pattern[i])
        {
            k += 1;
            table[i] = k;
            i++;
        }
        else if (k > 0)
        {
            k = table[k - 1];
        }
        else
        {
            table[i] = 0;
            i++;
        }
    }

    return table;
}