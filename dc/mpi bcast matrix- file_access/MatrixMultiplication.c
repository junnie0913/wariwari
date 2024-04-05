
#include <mpi.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int rank, size;

	int colA,rowA,colB,rowB;
	int partrow,fracrow;
	int *matrixbufA,*matrixbufC;
	int *matrixA,*matrixB,*matrixres;
	int posrowA,poscolB;
	int sum = 0;
	int count;
	int i,j;
	double startTime,endTime;
	FILE *fileA;
	FILE *fileB;
	FILE *resfile;
	char *fileAname= "matAlarge.txt",*fileBname= "matBlarge.txt",*resfilename= "Result-large.txt";

	MPI_Init(&argc, &argv); //to initial MPI with argument | ex. argument = 1, argc = 1, argv(vector) = "1"
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); //ID of processor
	MPI_Comm_size(MPI_COMM_WORLD, &size); //amount of processor

	if(rank==0){

		//open file
		fileA = fopen(fileAname,"r");
		fileB = fopen(fileBname,"r");
		if(!fileA || !fileB){
			printf("File Not Found!\n");
			return 0;
		}
		else{
			printf("Hooray!, File Founded.\n");

			fscanf(fileA, "%d %d",&rowA,&colA);
			fscanf(fileB, "%d %d",&rowB,&colB);

			if (colA!=rowB){
				printf("Sorry, It's can't multiply.\n");
				return 0;
			}
			else{
				printf("It's can multiplication!\n");
				partrow=rowA/size;
				fracrow=rowA%size;
				printf("partrow=%d fracrow=%d\n",partrow,fracrow);

				//allocate matrix
				matrixA = (int*)malloc(rowA * colA * sizeof(int));
				matrixB = (int*)malloc(rowB * colB * sizeof(int));
				printf("allocate A B successful\n");

				//read matrixA
				while (!feof(fileA))
					for(i=0; i<rowA; i++){
						for(j=0; j<colA; j++){
							fscanf(fileA, "%d", &matrixA[(i*colA)+j]);
						}
					}

				//read matrixB
				while (!feof(fileB))
					for(i=0; i<rowB; i++){
						for(j=0; j<colB; j++){
							fscanf(fileB, "%d", &matrixB[i*(colB)+j]);
						}
					}
				printf("read A B successful\n");

				//close file
				fclose(fileA);
				fclose(fileB);
			}
		}
	}

	startTime = MPI_Wtime();

	MPI_Bcast(&partrow,1,MPI_INT,0,MPI_COMM_WORLD);
	MPI_Bcast(&rowA,1,MPI_INT,0,MPI_COMM_WORLD);
	MPI_Bcast(&colA,1,MPI_INT,0,MPI_COMM_WORLD);
	MPI_Bcast(&rowB,1,MPI_INT,0,MPI_COMM_WORLD);
	MPI_Bcast(&colB,1,MPI_INT,0,MPI_COMM_WORLD);
	printf("bcast finish! from rank %d\n",rank);

	if (rank!=0)
	{
		matrixB = (int*)malloc(rowB * colB * sizeof(int));
		matrixbufA = (int*)malloc(partrow * colA * sizeof(int));
	}

	MPI_Bcast(&matrixB[0],rowB*colB,MPI_INT,0,MPI_COMM_WORLD);
	printf("Gotcha! matrix B from rank %d\n",rank);

	matrixbufC = (int*)malloc(partrow * colB * sizeof(int));

	if(rank==0){
		//send matrix A
		for (i=1; i<size; i++) {
			MPI_Send(&matrixA[partrow*colA*i],partrow*colA, MPI_INT, i, 0, MPI_COMM_WORLD);
		}
		printf("send A successful\n");
	}
	else{ //other rank

		MPI_Recv(&matrixbufA[0],partrow*colA, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		printf("received matrix A from rank %d\n",rank);

		//multiplication in other rank
		printf("Start multiplication!\n");
		for(posrowA=0;posrowA<partrow;posrowA++){ //shift down row in A
			for(poscolB=0; poscolB < colB; poscolB++){ //shift right column in B
				for(count = 0 ; count < colA; count++){
					sum+=matrixbufA[(posrowA*colA)+count]*matrixB[(colB*count)+poscolB];
				}
				matrixbufC[(posrowA*colB)+poscolB]=sum;
				sum = 0;
			}
		}
		printf("finish multiplication in rank %d\n",rank);

		MPI_Send(&matrixbufC[0],partrow*colB, MPI_INT, 0, 123, MPI_COMM_WORLD);
		printf("Send bufC! from rank %d\n",rank);

		free(matrixbufA);
		free(matrixbufC);
		free(matrixB);
	}

	if(rank==0) //rank 0
	{
		//allocate matrix result and assign 0 to whole matrix
		matrixres = (int*)malloc(rowA * colB * sizeof(int));

		//multiplication in rank 0
		printf("Start multiplication! rank 0\n");
		for(posrowA=0;posrowA<partrow;posrowA++){ //shift down row in A
			for(poscolB=0; poscolB < colB; poscolB++){ //shift right column in B
				for(count = 0 ; count < colA; count++){
					sum+=matrixA[(posrowA*colA)+count]*matrixB[(colB*count)+poscolB];
				}
				matrixbufC[(posrowA*colB)+poscolB]=sum;
				sum = 0;
			}
		}
		printf("finish multiplication in rank %d\n",rank);

		if (fracrow!=0)
		{
			//fraction multiplication in rank 0
			sum = 0;
			printf("Start frac mul\n");
			for(posrowA=partrow*size ; posrowA<(partrow*size)+fracrow ; posrowA++){ //shift down row in A
				for(poscolB=0; poscolB < colB; poscolB++){ //shift right column in B
					for(count = 0 ; count < colA; count++){
						sum+=matrixA[(posrowA*colA)+count]*matrixB[(colB*count)+poscolB];
					}
					matrixres[(posrowA*colB)+poscolB]=sum;
					sum = 0;
				}
			}
			printf("End frac mul\n");
		}

		free(matrixA);
		free(matrixB);

		for (i = 0; i < partrow*colB; i++) //PART 1 (rank 0)
			matrixres[i]=matrixbufC[i];

		for (i=1; i<size; i++) { //other PART from other rank
			MPI_Recv(&matrixres[partrow*colB*i],partrow*colB, MPI_INT, i, 123, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		}

		endTime = MPI_Wtime();

		//write file
		resfile = fopen(resfilename,"w+");
		fprintf(resfile,"%d %d",rowA,colB);
		fprintf(resfile, "\n");
		for(i=0;i<rowA;i++){
			//fprintf(resfile, "row: %d\t",i+1);
			for(j=0;j<colB;j++)
				fprintf(resfile, "%d ",matrixres[(i*colB)+j]);
			fprintf(resfile, "\n");
		}
		free(matrixres);

		fclose(resfile);
		printf("Successful! , Let's see result file.\n");

		printf("Timings : %f sec\n", endTime - startTime);
	}

	MPI_Finalize();
	return 0;
}
