/*
 * File:    proj2.c
 * Date:    April 2022
 * Author:  xvalac12, xvalac12@stud.fit.vutbr.cz
 * Project: Project 2: Building H2O
*/

// Used libraries
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>

// Semaphore state macros
#define LOCKED 0
#define UNLOCKED 1
#define DOUBLED 2

// Semaphore name macros
#define FIRST "/sem_output"
#define SECOND "/sem_reaction_o"
#define THIRD "/sem_reaction_h"
#define FOURTH "/sem_wait_others"
#define FIFTH "/sem_reaction_complete_o"
#define SIXTH "/sem_reaction_complete_h"
#define SEVENTH "/sem_synchro_h"
#define EIGHT "/sem_synchro_o"
#define MEMORY "/shared_memory"

//**********************|Variables of shared memory|*************************************//
typedef struct
{
    //name of .out file
	FILE *output_file;

    // vars for program arguments
    int num_O;
    int num_H;
    int reaction_time;
    int queue_time;

    // aux vars for outputs
    int action_num;         //count of actions
    int mol_num;            //count of molecule

    // counter vars for number of elements left
    int left_O;
    int left_H;

}
sm_var;


//*********************|Structure for semaphores|**********************************************//
typedef struct 
{
	sem_t *output;
    sem_t *enter_reaction_O;
    sem_t *enter_reaction_H;
    sem_t *wait_others;
    sem_t *reaction_complete_O;
    sem_t *reaction_complete_H;
    sem_t *synchro_O;
    sem_t *synchro_H;
}
semaphores;

/**
* Function for cleaning semaphores
*
* @param semaphores pointer to structure with semaphores
*/
void close_semaphores(semaphores *semaphore)
{
//*****************************|Closing the semaphores|********************************//
	sem_close(semaphore->output);
    sem_close(semaphore->enter_reaction_O);
    sem_close(semaphore->enter_reaction_H);
    sem_close(semaphore->wait_others);
    sem_close(semaphore->reaction_complete_O);
    sem_close(semaphore->reaction_complete_H);
    sem_close(semaphore->synchro_O);
    sem_close(semaphore->synchro_H);

//******************************|Unlink the semaphores|******************************//
	sem_unlink(FIRST);
    sem_unlink(SECOND);
    sem_unlink(THIRD);
    sem_unlink(FOURTH);
    sem_unlink(FIFTH);
    sem_unlink(SIXTH);
    sem_unlink(SEVENTH);
    sem_unlink(EIGHT);
}

/**
* Function for cleaning shared_memory
*
* @param shared_memory
* @param s_memory variables from shared memory
*/
void clean_function(int shared_memory, sm_var *s_memory)
{
	fclose(s_memory->output_file);
	munmap(s_memory, sizeof(sm_var));
	shm_unlink(MEMORY);
	close(shared_memory);
}

/**
* Function for actions of Hydrogen
*
* @param h_cnt number of element H
* @param s_memory variables from shared memory
* @param semaphore pointer to structure with semaphores
*/
void h_act(int h_cnt, sm_var *s_memory, semaphores *semaphore) {
    sem_wait(semaphore->output);
    fprintf(s_memory->output_file, "%d: H %d: started\n", ++(s_memory->action_num), h_cnt);
    sem_post(semaphore->output);

    usleep((rand() % (s_memory->queue_time + 1)) * 1000);
    sem_wait(semaphore->output);
    fprintf(s_memory->output_file, "%d: H %d: going to queue\n", ++(s_memory->action_num), h_cnt);
    sem_post(semaphore->output);

    sem_wait(semaphore->enter_reaction_H);
    if (s_memory->left_O == 0 || s_memory->left_H <= 1)
    {
        sem_wait(semaphore->output);
        fprintf(s_memory->output_file, "%d: H %d: not enough O or H\n", ++(s_memory->action_num), h_cnt);
        sem_post(semaphore->output);
        sem_post(semaphore->reaction_complete_O);
        sem_post(semaphore->enter_reaction_H);
    }
    else
    {
        // synchronization of processes for creating molecule
        sem_post(semaphore->synchro_O);
        sem_wait(semaphore->synchro_H);


        sem_wait(semaphore->output);
        fprintf(s_memory->output_file, "%d: H %d: creating molecule %d\n", ++(s_memory->action_num), h_cnt,
                (s_memory->mol_num));
        sem_post(semaphore->output);

        sem_wait(semaphore->wait_others);

        // synchronization of processes for creating molecule
        sem_post(semaphore->synchro_O);
        sem_wait(semaphore->synchro_H);

        sem_wait(semaphore->output);
        fprintf(s_memory->output_file, "%d: H %d: molecule %d created\n", ++(s_memory->action_num), h_cnt,
                (s_memory->mol_num));
        sem_post(semaphore->output);

        s_memory->left_H--;

        sem_post(semaphore->reaction_complete_O);
        sem_wait(semaphore->reaction_complete_H);

        sem_post(semaphore->enter_reaction_H);
    }
}

/**
* Function for actions of Oxygen
*
* @param o_cnt number of element O
* @param s_memory variables from shared memory
* @param semaphore pointer to structure with semaphores
*/
void o_act(int o_cnt, sm_var *s_memory, semaphores *semaphore)
{
    sem_wait(semaphore->output);
    fprintf(s_memory->output_file,"%d: O %d: started\n", ++(s_memory->action_num), o_cnt);
    sem_post(semaphore->output);
    usleep((rand() % (s_memory->queue_time + 1)) * 1000); // wait before going to queue

    sem_wait(semaphore->output);
    fprintf(s_memory->output_file,"%d: O %d: going to queue\n", ++(s_memory->action_num), o_cnt);
    sem_post(semaphore->output);


    sem_wait(semaphore->enter_reaction_O);
    if (s_memory->left_H <= 1)
    {
        sem_wait(semaphore->output);
        fprintf(s_memory->output_file,"%d: O %d: not enough H\n", ++(s_memory->action_num), o_cnt);
        sem_post(semaphore->output);
        sem_post(semaphore->reaction_complete_H);
        sem_post(semaphore->reaction_complete_H);
        sem_post(semaphore->enter_reaction_O);
    }
    else
    {
        // synchronization of processes for creating molecule
        sem_wait(semaphore->synchro_O);
        sem_wait(semaphore->synchro_O);
        sem_post(semaphore->synchro_H);
        sem_post(semaphore->synchro_H);

        sem_wait(semaphore->output);
        fprintf(s_memory->output_file, "%d: O %d: creating molecule %d\n", ++(s_memory->action_num), o_cnt, (s_memory->mol_num));
        sem_post(semaphore->output);


        usleep((rand() % (s_memory->reaction_time + 1)) * 1000);
        sem_post(semaphore->wait_others);
        sem_post(semaphore->wait_others);

        // synchronization of processes for creating molecule
        sem_wait(semaphore->synchro_O);
        sem_wait(semaphore->synchro_O);
        sem_post(semaphore->synchro_H);
        sem_post(semaphore->synchro_H);

        sem_wait(semaphore->output);
        fprintf(s_memory->output_file, "%d: O %d: molecule %d created\n", ++(s_memory->action_num), o_cnt, (s_memory->mol_num));
        sem_post(semaphore->output);

        s_memory->left_O--;

        sem_wait(semaphore->reaction_complete_O);
        sem_wait(semaphore->reaction_complete_O);
        s_memory->mol_num++;
        sem_post(semaphore->reaction_complete_H);
        sem_post(semaphore->reaction_complete_H);

        sem_post(semaphore->enter_reaction_O);
    }
}

/**
* Function for checking correctness of program arguments
*
* @param argc number of arguments
* @param argv individual arguments
* @param s_memory variables from shared memory
* @return False for error, otherwise True
*/
bool check_set_args(int argc,char *argv[], sm_var *s_memory)
{
    char *aux;
    if (argc != 5)
    {
        fprintf(stderr, "Bad number of arguments.\n");
        return false;
    }
    for (int cnt = 1; cnt < argc; cnt ++)	//argument must be integer number
    {
        int aux_int;
        char aux_char;
        if((sscanf(argv[cnt], "%d%c", &aux_int, &aux_char)) != 1)
        {
            fprintf(stderr, "argv[%d] must be an integer number.\n", cnt);
            return false;
        }
    }

    s_memory->num_O = (int) strtol(argv[1], &aux, 10);
    s_memory->num_H = (int) strtol(argv[2], &aux, 10);
    s_memory->queue_time = (int) strtol(argv[3], &aux, 10);
    s_memory->reaction_time = (int) strtol(argv[4], &aux, 10);
    if (s_memory->num_O <= 0)
    {
        fprintf(stderr, "Bad first argument.\n");
        return false;
    }
    if (s_memory->num_H <= 0)
    {
        fprintf(stderr, "Bad second argument.\n");
        return false;
    }

    if ((s_memory->queue_time < 0) || (s_memory->queue_time > 1000))
    {
        fprintf(stderr, "Bad third argument.\n");
        return false;
    }

    if ((s_memory->reaction_time < 0) || (s_memory->reaction_time > 1000))
    {
        fprintf(stderr, "Bad fourth argument.\n");
        return false;
    }
    return true;
}

/**
* Main function of program, opening and closing shared memory and semaphores, creating processes O and H
*
* @param argc number of arguments
* @param argv individual arguments
* @return Program exit code
*/
int main(int argc,char *argv[])
{
    int shared_memory = shm_open(MEMORY, O_CREAT | O_EXCL | O_RDWR, 0644);
    if(shared_memory == -1)
    {
        fprintf(stderr,"Error occurred, while opening of shared memory.\n");
        close(shared_memory);
        exit(EXIT_FAILURE);
    }
    //creating space for structure with variables of shared memory
    ftruncate(shared_memory, sizeof(sm_var));
    sm_var *s_memory;

    //mmap for variables of shared memory
    s_memory = mmap(NULL, sizeof(sm_var), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory, 0);

    s_memory->output_file=fopen("proj2.out", "w"); //opening file for output
    setbuf(s_memory->output_file, NULL); // no need to write fflush after every fprintf

    bool arguments = check_set_args(argc, argv, s_memory);
	if(!arguments)
	{
		fprintf(stderr, "Program failed.\n");
        clean_function(shared_memory, s_memory);
		exit(EXIT_FAILURE);
	}

    s_memory->mol_num = 1;
    s_memory->left_H = s_memory->num_H;
    s_memory->left_O = s_memory->num_O;

//************************************|Semaphores opening|*************************************//
    semaphores semaphore;
    semaphore.output = sem_open(FIRST, O_CREAT | O_EXCL, 0644, UNLOCKED);
    semaphore.enter_reaction_O = sem_open(SECOND, O_CREAT | O_EXCL, 0644, UNLOCKED);
    semaphore.enter_reaction_H = sem_open(THIRD, O_CREAT | O_EXCL, 0644, DOUBLED);
    semaphore.wait_others = sem_open(FOURTH, O_CREAT | O_EXCL, 0644, LOCKED);
    semaphore.reaction_complete_O = sem_open(FIFTH, O_CREAT | O_EXCL, 0644, LOCKED);
    semaphore.reaction_complete_H = sem_open(SIXTH, O_CREAT | O_EXCL, 0644, LOCKED);
    semaphore.synchro_O = sem_open(SEVENTH, O_CREAT | O_EXCL, 0644, LOCKED);
    semaphore.synchro_H= sem_open(EIGHT, O_CREAT | O_EXCL, 0644, LOCKED);

    int array_of_OPID[s_memory->num_O];
    int array_of_HPID[s_memory->num_H];
    int o_cnt = 0;
    int h_cnt = 0;
    for(int cnt = 0; cnt < s_memory->num_O; cnt++)
    {
        o_cnt++;
        pid_t o_PID = fork();
        if (o_PID < 0)
        {
            for (int kill_counter = 0; kill_counter < cnt; kill_counter++)
            {
                kill(array_of_OPID[kill_counter], SIGTERM);	//kill child processes
            }
            fprintf(stderr,"ERROR in fork.\n");
            clean_function(shared_memory, s_memory);
            close_semaphores(&semaphore);
            exit(EXIT_FAILURE);
        }
        if (o_PID == 0)
        {
            srand(time(NULL) * getpid());
            o_act(o_cnt, s_memory, &semaphore);
            exit(EXIT_SUCCESS);
        }
        if (o_PID > 0)
        {
            array_of_OPID[cnt] = o_PID;
        }
    }

    for(int cnt = 0; cnt < s_memory->num_H; cnt++)
    {
        h_cnt++;
        pid_t h_PID = fork();
        if (h_PID < 0)
        {
            for (int kill_counter = 0; kill_counter < cnt; kill_counter++)
            {
                kill(array_of_HPID[kill_counter], SIGTERM);	//kill child processes
            }
            fprintf(stderr,"ERROR in fork.\n");
            clean_function(shared_memory, s_memory);
            close_semaphores(&semaphore);
            exit(EXIT_FAILURE);
        }
        if (h_PID == 0)
        {
            srand(time(NULL) * getpid());
            h_act(h_cnt, s_memory, &semaphore);
            exit(EXIT_SUCCESS);
        }
        if (h_PID > 0)
        {
            array_of_HPID[cnt] = h_PID;
        }
    }
    while(wait(NULL) > 0);

    clean_function(shared_memory, s_memory);
    close_semaphores(&semaphore);
    return EXIT_SUCCESS;
}