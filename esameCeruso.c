/*
 ============================================================================
 Name        : NBodyParallel.c
 Author      : Raffaele
 Version     :
 Copyright   : copyright Raffaele
 Description : NBody MPI in C
 ============================================================================
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include<mpi.h>

#define SOFTENING 1e-9f

typedef struct { float x, y, z, vx, vy, vz; } Body;
typedef struct { int startRange, endRange; } Range;

void randomizeBodies(float *data, int n) {
	int i;
	for (i = 0; i < n; i++) {
		data[i] = 2.0f * (rand() / (float)RAND_MAX) - 1.0f;
	}
}
void bodyForce(Body *p, float dt, int n, int start, int end) {
	int i,j;
	for (i = start; i < end; i++) {
		float Fx = 0.0f; float Fy = 0.0f; float Fz = 0.0f;

		for (j = 0; j < n; j++) {
			float dx = p[j].x - p[i].x;
			float dy = p[j].y - p[i].y;
			float dz = p[j].z - p[i].z;
			float distSqr = dx*dx + dy*dy + dz*dz + SOFTENING;
			float invDist = 1.0f / sqrtf(distSqr);
			float invDist3 = invDist * invDist * invDist;

			Fx += dx * invDist3; Fy += dy * invDist3; Fz += dz * invDist3;
		}

		p[i].vx += dt*Fx; p[i].vy += dt*Fy; p[i].vz += dt*Fz;
	}
}

int main(int argc,char** argv) {

	int  my_rank,process,tag=0,flag;
	int nBodies = 10000;
	if (argc > 1) nBodies = atoi(argv[1]);
	int i,x;

	const float dt = 0.01f; // time step
	int nIters = 10;  // simulation iterations
	if (argc > 2) nIters = atoi(argv[2]);
	flag=1;
	if (argc > 3) flag = atoi(argv[3]);
	MPI_Status status ;

	MPI_Init(&argc, &argv);

	/* find out process rank */
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	/* find out number of processes */
	MPI_Comm_size(MPI_COMM_WORLD, &process);

	/* create a type for struct Body */
	const int nitems=6;
	int          blocklengths[6] = {1,1,1,1,1,1};
	MPI_Datatype types[6] = {MPI_FLOAT, MPI_FLOAT,MPI_FLOAT,MPI_FLOAT,MPI_FLOAT,MPI_FLOAT};
	MPI_Datatype mpi_body_type;
	MPI_Aint     offsets[6];

	offsets[0] = offsetof(Body, x);
	offsets[1] = offsetof(Body, y);
	offsets[2] = offsetof(Body, z);
	offsets[3] = offsetof(Body, vx);
	offsets[4] = offsetof(Body, vy);
	offsets[5] = offsetof(Body, vz);

	MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_body_type);
	MPI_Type_commit(&mpi_body_type);

	int bytes = nBodies*sizeof(Body);
	int bytesRange = process*sizeof(Range);

	float *buf = (float*)malloc(bytes);
	float *my_buf = (float*)malloc(bytes);
	int *bufRange = (int*)malloc(bytesRange);

	Body *p = (Body*)buf;
	Body *my_p = (Body*)my_buf;

	//A Range struct used to store start,end of each process(slave)
	Range *range = (Range*)bufRange;

	if(my_rank==0)
	{
		double startTime, endTime;
		// Init pos / vel data
		randomizeBodies(buf, 6*nBodies);

		//Start time of computation
		startTime = MPI_Wtime();

		int iter;

		//Case when there is only 1 process => Sequencial

		//Portion = Calculate portion that every process have to compute
		//Portion_more = portion with one more slot for every process < remaind
		int portion = nBodies/(process), remaind = nBodies%(process),start=0,portion_more,end=0,portionMaster;

		portion_more = portion+1;

		//Get portion of master if there is only one process
		if(process==1)portionMaster = nBodies;

		//Calculate range start for first slave and range for master
		if(remaind > 1)
		{
			start = portion_more;
			portionMaster=portion_more;
		}
		//calculate range start for first slave and range for master
		if(remaind==1)
		{
			start = portion_more;
			portionMaster=portion_more;
		}
		//calculate range start for first slave and range for master
		if(remaind==0 && process >1)
		{
			start = portion;
			portionMaster=portion;
		}

		remaind--;

		//Send data with start and end
		for(x=1;x<process;x++)
		{
			//Send start and end+1 to process x, who is lesser then remaind
			if(remaind>0)
			{
				end = start+portion_more;

				MPI_Send(&start, 1, MPI_INT,x, tag, MPI_COMM_WORLD);
				MPI_Send(&end, 1, MPI_INT,x, tag, MPI_COMM_WORLD);

				//Store the value of start and end of process x in the struct Range
				range[x].startRange = start;
				range[x].endRange = end;

				remaind--;
				start=start+portion_more;

			}
			else
			{
				//Send start and end to process x

				end = start+portion;

				MPI_Send(&start, 1, MPI_INT,x, tag, MPI_COMM_WORLD);
				MPI_Send(&end, 1, MPI_INT,x, tag, MPI_COMM_WORLD);

				//Store the value of start and end of process x in the struct Range
				range[x].startRange = start;
				range[x].endRange = end;
				start=start+portion;

			}
		}

		for (iter = 1; iter <= nIters; iter++) {

			//Start to get time
			startTime = MPI_Wtime();

			for(x=1;x<process;x++)
			{
				//Send struct to every process
				MPI_Send(p, nBodies, mpi_body_type, x, tag, MPI_COMM_WORLD);
			}
			//Call bodyForce for master with its range
			bodyForce(p, dt, nBodies,0,portionMaster);
			//Integration values
			for (i = 0 ; i < portionMaster; i++) {
				p[i].x += p[i].vx*dt;
				p[i].y += p[i].vy*dt;
				p[i].z += p[i].vz*dt;
			}
			//Time to get results from slave
			for(x=1;x<process;x++)
			{
				//Wait to recive struct updated from process x

				MPI_Recv(my_p, nBodies, mpi_body_type, x, tag, MPI_COMM_WORLD, &status);

				for(i=range[x].startRange;i<range[x].endRange;i++)
				{
					//Update struct from start and end of process x that i get from range[x]
					p[i].vx = my_p[i].vx;
					p[i].vy = my_p[i].vy;
					p[i].vz = my_p[i].vz;

					p[i].x += my_p[i].vx*dt;
					p[i].y += my_p[i].vy*dt;
					p[i].z += my_p[i].vz*dt;

				}
			}
		}

		//End time
		endTime = MPI_Wtime();

		//Flag for print or not the bodies' informations
		if(flag==1)
		{
			for(i=0;i<nBodies;i++)
			{
				printf("Particella %d con posizione %0.3f %0.3f %0.3f% 0.3f %0.3f %0.3f\n",i+1,p[i].x,p[i].y,p[i].z,p[i].vx,p[i].vy,p[i].vz);
			}
		}
		printf("Time: %f ms\n",(endTime-startTime)*1000);

		free(buf);
		free(bufRange);
	}
	else  //Slave
	{
		int my_start,my_end,my_iter=0;

		//Get start and end from master
		MPI_Recv(&my_start, 1, MPI_INT, 0, tag,MPI_COMM_WORLD, &status);
		MPI_Recv(&my_end, 1, MPI_INT, 0, tag,MPI_COMM_WORLD, &status);

		while(my_iter<nIters)   //For each iteration my_iter slave calls bodyForce always on the same range
		{
			//I get from master the struct updated because at iteration j slave needs values updated from master of iteration j-1
			MPI_Recv(my_p, nBodies, mpi_body_type, 0, tag, MPI_COMM_WORLD, &status);

			bodyForce(my_p, dt, nBodies, my_start, my_end);

			//I send struct to master
			MPI_Send(my_p, nBodies, mpi_body_type, 0, 0, MPI_COMM_WORLD);

			my_iter++;
		}

		free(my_buf);

	}

	/* shut down MPI */
	MPI_Finalize();
}
