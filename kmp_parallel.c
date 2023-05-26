
#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include<string.h>
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
                   int *prefixTable, int rank, int offset, int targetOffSet)
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

size_t s_strlcpy(char *dest, const char *src, size_t len)
{
    char *d = dest;
    char *e = dest + len; /* end of destination buffer */
    const char *s = src;

    /* Insert characters into the destination buffer
       until we reach the end of the source string
       or the end of the destination buffer, whichever
       comes first. */
    while (*s != '\0' && d < e)
        *d++ = *s++;

    /* Terminate the destination buffer, being wary of the fact
       that len might be zero. */
    if (d < e)        // If the destination buffer still has room.
        *d = 0;
    else if (len > 0) // We ran out of room, so zero out the last char
                      // (if the destination buffer has any items at all).
        d[-1] = 0;

    /* Advance to the end of the source string. */
    while (*s != '\0')
        s++;

    /* Return the number of characters
       between *src and *s,
       including *src but not including *s . 
       This is the length of the source string. */
    return s - src;
}

int main(int argc, char **argv)
{
    int i, j;
    int n;
    int m;

    struct timespec start1, end1;
    double diff;

    char pattern[] = "RM50YHB31VUHN9QQ2VF8TIZWEBB3455S76KDUC1T14550EFIJW7H4442BDBZ81QS512HZLAH4GJL0I1XEPMK399Q5T9WB40VVDPU";

    n = strlen(target);
    m = strlen(pattern);
    int broadCastTag = 1;
    int cumulativeKMPTag = 2;
    int nonCumulativeKMPTag = 0;

    int *kmp_table = kmptable(pattern, m);
    int myrank, nproc;
    MPI_Init(&argc, &argv);                 // initialize the MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); // number of processes that given in command line
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);  // ranks the process rank

    int start_pos = n / nproc;
    int str_per_proc = n / nproc;
    int end = str_per_proc + n % nproc;
   // printf("str_per_proc %d\n", str_per_proc);
    char send_msg[MAX(m, end) + 1]; // 4-1 = 3
    char recv_msg[MAX(m, end) + 1];
    char end_msg[MAX(m, end) + 1];
    // printf("%d",MAX(m, end)+1);
    double start, stop;
    MPI_Status msg_recv_status;

     start = MPI_Wtime();

    if (myrank == master)
    {
        printf("Result KMP Sequential\n");
        /**
         * Broadcasting to other processes
         */
        for (i = 1; i < nproc; i++)
        {
            if (i == (nproc - 1))
            {
                /**
                 * The last process will recieve the odd offset
                 */
                s_strlcpy(send_msg, target + i * start_pos, end + 1);
               // printf("proc:%d and pattern:%s and len %d\n", i, send_msg, strlen(send_msg));

                MPI_Send(send_msg, end + 1, MPI_CHAR, i, broadCastTag, MPI_COMM_WORLD);
            }
            else
            {
                s_strlcpy(send_msg, target + i * start_pos, str_per_proc + 1);
                //printf("proc:%d and pattern:%s and len %d\n", i, send_msg, strlen(send_msg));

                MPI_Send(send_msg, str_per_proc + 1, MPI_CHAR, i, broadCastTag, MPI_COMM_WORLD);
            }
            // printf("my rank %d and txt: %s\n",*send_msg);
        }
        s_strlcpy(send_msg, target, str_per_proc + 1);
        // printf("My rank is %d and target %s\n", myrank, send_msg);
        // The last process
    }
    else if (myrank == nproc - 1)
    {
        MPI_Recv(recv_msg, end + 1, MPI_CHAR, master, broadCastTag, MPI_COMM_WORLD, &msg_recv_status);
        //printf("My rank is %d and txt %s and len is %d \n", myrank, recv_msg, strlen(recv_msg));
    }
    else
    {
        //printf("before %d\n", strlen(recv_msg));
        MPI_Recv(recv_msg, str_per_proc + 1, MPI_CHAR, master, broadCastTag, MPI_COMM_WORLD, &msg_recv_status);
       // printf("My rank is %d and txt %s and len is %d\n", myrank, recv_msg, strlen(recv_msg));
    }

    MPI_Barrier(MPI_COMM_WORLD);
    /**
     *  ^^ Broadcasting and recieving done
     */
    if (myrank == master)
    {
        MPI_Request master_send_request;
        int *prefixTable = (int *)malloc(str_per_proc * sizeof(int));
        int sizeOfPrefixTable = str_per_proc;
        processTarget(send_msg, pattern, kmp_table, prefixTable, myrank, 0, 0);
        int maxMatchedPatLen = prefixTable[sizeOfPrefixTable - 1];
        for (int i = 0; i < sizeOfPrefixTable; i++)
        {
            if (prefixTable[i] == strlen(pattern))
            {
                stop = MPI_Wtime();
                double timeDifference = (stop - start) * MILLION;
                printf("Time Elapsed: %0.05f\n", timeDifference);
                printf("Pattern found at %d\n", (myrank * str_per_proc + i) - strlen(pattern));
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
        // printf(" maxMatchedPatLen %d\n", maxMatchedPatLen);
        MPI_Isend(&maxMatchedPatLen, 1, MPI_INT, myrank + 1, cumulativeKMPTag, MPI_COMM_WORLD, &master_send_request);
        free(prefixTable);
    }
    else
    {
        // process and send to next process
        MPI_Request cur_send_request, cumulative_recv_request;
        MPI_Status cum_recv_status;
        int *currentPrefixTable = (int *)malloc(strlen(recv_msg) * sizeof(int));
        processTarget(recv_msg, pattern, kmp_table, currentPrefixTable, myrank, 0, 0);
        int sizeOfPrefixTable = strlen(recv_msg);
        int currentMaxMatchedPatLen = currentPrefixTable[sizeOfPrefixTable - 1];
        for (int i = 0; i < sizeOfPrefixTable; i++)
        {
            if (currentPrefixTable[i] == strlen(pattern))
            {
                stop = MPI_Wtime();
                double timeDifference = (stop - start) * MILLION;
                printf("Time Elapsed: %0.05f\n", timeDifference);
                printf("Pattern found at %d\n", (myrank * str_per_proc + i) - strlen(pattern));
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
        if (myrank != (nproc - 1))
            MPI_Isend(&currentMaxMatchedPatLen, 1, MPI_INT, myrank + 1, nonCumulativeKMPTag, MPI_COMM_WORLD, &cur_send_request);

        // Cumulative Async Recv Request

        int cumulativePartialMatchFromProc, nonCumulativePartialMatch = -1;
        int resultForNonCumulativePartialMatch = currentMaxMatchedPatLen;
        int reqStatus;
        MPI_Irecv(&cumulativePartialMatchFromProc, 1, MPI_INT, myrank - 1, cumulativeKMPTag, MPI_COMM_WORLD, &cumulative_recv_request);
        MPI_Test(&cumulative_recv_request, &reqStatus, &cum_recv_status);

        while (!reqStatus)
        {
            // printf("my rank is %d\n", myrank);
            MPI_Request nc_send_request, nc_recv_request;
            MPI_Status nc_recv_status;
            int partialMatchFromProc;
            int ncReqStatus;
            MPI_Irecv(&partialMatchFromProc, 1, MPI_INT, myrank - 1, nonCumulativeKMPTag, MPI_COMM_WORLD, &nc_recv_request);
            MPI_Test(&nc_recv_request, &ncReqStatus, &nc_recv_status);
            if (ncReqStatus)
            {
                if (partialMatchFromProc != 0)
                {
                    nonCumulativePartialMatch = partialMatchFromProc;
                    int *prefixTable = (int *)malloc(strlen(recv_msg) * sizeof(int));
                    processTarget(recv_msg, pattern, kmp_table, prefixTable, myrank, partialMatchFromProc, 0);
                    int sizeOfPrefixTable = strlen(recv_msg);
                    for (int i = 0; i < sizeOfPrefixTable; i++)
                    {
                        if (prefixTable[i] == strlen(pattern))
                        {
                            stop = MPI_Wtime();
                            double timeDifference = (stop - start) * MILLION;
                            printf("Time Elapsed: %0.05f\n", timeDifference);
                            printf("Pattern found at %d\n", (myrank * str_per_proc + i) - strlen(pattern));
                            MPI_Abort(MPI_COMM_WORLD, 1);
                        }
                    }
                    resultForNonCumulativePartialMatch = prefixTable[sizeOfPrefixTable - 1];
                    free(prefixTable);
                }
                if (myrank != (nproc - 1))
                    MPI_Isend(&resultForNonCumulativePartialMatch, 1, MPI_INT, myrank + 1, nonCumulativeKMPTag, MPI_COMM_WORLD, &nc_send_request);
            }
            MPI_Test(&cumulative_recv_request, &reqStatus, &cum_recv_status);
        }
        if (reqStatus)
        {
           // printf("cum send rank is %d and size is %d\n", myrank, strlen(recv_msg));
            // recieved cumulative
            MPI_Request cum_send_request;
            int *prefixTable = (int *)malloc(strlen(recv_msg) * sizeof(int));
            processTarget(recv_msg, pattern, kmp_table, prefixTable, myrank, cumulativePartialMatchFromProc, 0);
            int sizeOfPrefixTable = strlen(recv_msg);
            for (int i = 0; i < sizeOfPrefixTable; i++)
            {

                if (prefixTable[i] == strlen(pattern))
                {
                    stop = MPI_Wtime();
                    double timeDifference = (stop - start) * MILLION;
                    printf("Time Elapsed: %0.05f\n", timeDifference);
                    printf("Pattern found at %d\n", (myrank * str_per_proc + i) - strlen(pattern));
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
            }
            if (myrank != (nproc - 1))
                MPI_Isend(&prefixTable[sizeOfPrefixTable - 1], 1, MPI_INT, myrank + 1, cumulativeKMPTag, MPI_COMM_WORLD, &cum_send_request);
            free(prefixTable);
        }
    }
    printf("Processor ended: %d\n", myrank);
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