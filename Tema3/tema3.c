#include<mpi.h>
#include<stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

// afiseaza intre cine a fost trimis mesajul si trimite mesajul cu MPI_Send
void my_send(int *to_send, int count, int sender, int receiver) {
	printf("M(%d,%d)\n", sender, receiver);
	MPI_Send(to_send, count, MPI_INT, receiver, 0, MPI_COMM_WORLD);
}

int main(int argc, char * argv[]) {
	int rank, nProcesses;
	int n, *v;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);

	int **topology, *workers, *nr_workers, leader;
	topology = (int**) malloc(3 * sizeof(int*));
	workers = (int*) malloc(3 * sizeof(int));
	nr_workers = (int*) malloc(3 * sizeof(int));
	// partea 1, am considerat ca nu exista conexiune intre 0 si 1
	if (rank == 0) {
		FILE *f;
		f = fopen("cluster0.txt", "r");
		fscanf(f, "%d", &nr_workers[0]);
		workers = (int*) malloc(nr_workers[0] * sizeof(int));
		for (int i = 0; i < nr_workers[0]; i++)
			fscanf(f, "%d", &workers[i]);
		fclose(f);
		/* procesul 0 trimite cluster-ul sau procesului 2, iar acesta il va trimite
		 si procesului 1 */
		my_send(&nr_workers[0], 1, rank, 2);
		my_send(workers, nr_workers[0], rank, 2);
		/* primeste configuratiile proceselor 1 si 2 de la procesul 2 */
		MPI_Recv(&nr_workers[1], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, NULL);
		topology[1] = (int*) malloc(nr_workers[1] * sizeof(int));
		MPI_Recv(topology[1], nr_workers[1], MPI_INT, 2, 0, MPI_COMM_WORLD, NULL);
		MPI_Recv(&nr_workers[2], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, NULL);
		topology[2] = (int*) malloc(nr_workers[2] * sizeof(int));
		MPI_Recv(topology[2], nr_workers[2], MPI_INT, 2, 0, MPI_COMM_WORLD, NULL);
		topology[0] = workers;
		// afiseaza topologia finala si o trimite mai departe proceselor worker proprii
		printf("%d -> ", rank);
		for (int i = 0; i < 3; i++) {
			printf("%d:", i);
			for (int j = 0; j < nr_workers[i]; j++) {
				if (j == nr_workers[i] - 1)
					printf("%d ", topology[i][j]);
				else
					printf("%d,", topology[i][j]);
			}
		}
		printf("\n");
		for (int i = 0; i < nr_workers[0]; i++) {
			my_send(&rank, 1, rank, topology[0][i]);
			my_send(nr_workers, 3, rank, topology[0][i]);
			my_send(topology[0], nr_workers[0], rank, topology[0][i]);
			my_send(topology[1], nr_workers[1], rank, topology[0][i]);
			my_send(topology[2], nr_workers[2], rank, topology[0][i]);
		}
		// partea 2
		// creeaza vectorul v
		n = atoi(argv[1]);
		v = (int*) malloc(n * sizeof(int));
		for (int i = 0; i < n; i++) {
			v[i] = i;
		}
		// imparte vectorul in parti egale pentru fiecare proces worker
		int n_0 = (double) n / (nProcesses - 3) * nr_workers[0];
		int n_1 = (double) n / (nProcesses - 3) * nr_workers[1];
		int n_2 = n - n_0 - n_1;
		/* trimite procesului 2 partea din vector pe care trebuie sa o modifice 
		 procesele worker din clusterul 1, dar si cea pe care trebuie sa o modifice cele din clusterul 2*/
		my_send(&n_1, 1, rank, 2);
		my_send(v + n_0, n_1, rank, 2);
		my_send(&n_2, 1, rank, 2);
		my_send((v + n_1 + n_0), n_2, rank, 2);
		// calculeaza numarul de elemente pe care le va procesa fiecare proces worker
		int n_worker = n_0 / nr_workers[0];
		// trimite partea corespunzatoare fiecarui proces worker
		for (int i = 0; i < nr_workers[0]; i++) {
			my_send(&n_worker, 1, rank, workers[i]);
			my_send((v + n_worker * i), n_worker, rank, workers[i]);
		}
		// primeste portiunea de vector actualizata
		for (int i = 0; i < nr_workers[0]; i++) {
			MPI_Recv(v + n_worker * i, n_worker, MPI_INT, workers[i], 0, MPI_COMM_WORLD, NULL);
		}
		// primeste portiunile de vector modificate de clusterele 1 si 2
		MPI_Recv((v + n_0), n_1, MPI_INT, 2, 0, MPI_COMM_WORLD, NULL);
		MPI_Recv((v + n_0 + n_1), n_2, MPI_INT, 2, 0, MPI_COMM_WORLD, NULL);
		
		printf("Rezultat: ");
		for (int i = 0; i < n - 1; i++) {
			printf("%d ", v[i]);
		}
		printf("%d\n", v[n-1]);
	} else if (rank == 1) {
		FILE *f;
		f = fopen("cluster1.txt", "r");
		fscanf(f, "%d", &nr_workers[1]);
		workers = (int*) malloc(nr_workers[1] * sizeof(int));
		for (int i = 0; i < nr_workers[1]; i++)
			fscanf(f, "%d", &workers[i]);
		fclose(f);
		// primeste topologia clusterelor 1 si 2 de la procesul 2
		MPI_Recv(&nr_workers[0], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, NULL);
		topology[0] = (int*) malloc(nr_workers[0] * sizeof(int));
		MPI_Recv(topology[0], nr_workers[0], MPI_INT, 2, 0, MPI_COMM_WORLD, NULL);
		MPI_Recv(&nr_workers[2], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, NULL);
		topology[2] = (int*) malloc(nr_workers[2] * sizeof(int));
		MPI_Recv(topology[2], nr_workers[2], MPI_INT, 2, 0, MPI_COMM_WORLD, NULL);
		topology[1] = workers;
		// afiseaza topologia completa si o trimite proceselor worker proprii
		printf("%d -> ", rank);
		for (int i = 0; i < 3; i++) {
			printf("%d:", i);
			for (int j = 0; j < nr_workers[i]; j++) {
				if (j == nr_workers[i] - 1)
					printf("%d ", topology[i][j]);
				else
					printf("%d,", topology[i][j]);
			}
		}
		printf("\n");
		my_send(&nr_workers[1], 1, rank, 2);
		my_send(workers, nr_workers[1], rank, 2);
		for (int i = 0; i < nr_workers[1]; i++) {
			my_send(&rank, 1, rank, topology[1][i]);
			my_send(nr_workers, 3, rank, topology[1][i]);
			my_send(topology[0], nr_workers[0], rank, topology[1][i]);
			my_send(topology[1], nr_workers[1], rank, topology[1][i]);
			my_send(topology[2], nr_workers[2], rank, topology[1][i]);
		}
		// partea2
		// primeste vectorul pe care trebuie sa il modifice de la procesul 2
		MPI_Recv(&n, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, NULL);
		int *v = (int*) malloc(n * sizeof(int));
		MPI_Recv(v, n, MPI_INT, 2, 0, MPI_COMM_WORLD, NULL);
		int n_worker = (double) n / nr_workers[1]; // numarul de elemente corespunzator fiecarui worker
		for (int i = 0; i < nr_workers[1]; i++) {
			// trimite vectorul care trebuie modificat proceselor worker
			my_send(&n_worker, 1, rank, workers[i]);
			my_send((v + n_worker * i), n_worker, rank, workers[i]);
		}
		for (int i = 0; i < nr_workers[1]; i++) {
			MPI_Recv(v + n_worker * i, n_worker, MPI_INT, workers[i], 0, MPI_COMM_WORLD, NULL);
		}
		// trimite vectorul modificat inapoi la procesul 2
		my_send(v, n, rank, 2);
	} else if (rank == 2) {
		FILE *f;
		f = fopen("cluster2.txt", "r");
		fscanf(f, "%d", &nr_workers[2]);
		workers = (int*) malloc(nr_workers[2] * sizeof(int));
		for (int i = 0; i < nr_workers[2]; i++)
			fscanf(f, "%d", &workers[i]);
		fclose(f);
		// primeste topologia clusterului 0
		MPI_Recv(&nr_workers[0], 1, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
		topology[0] = (int*) malloc(nr_workers[0] * sizeof(int));
		MPI_Recv(topology[0], nr_workers[0], MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
		// trimite topologiile clusterelor 0 si 2 procesului 1
		my_send(&nr_workers[0], 1, rank, 1);
		my_send(topology[0], nr_workers[0],rank, 1);
		my_send(&nr_workers[2], 1, rank, 1);
		my_send(workers, nr_workers[2],rank, 1);
		// primeste topologia clusterului 1
		MPI_Recv(&nr_workers[1], 1, MPI_INT, 1, 0, MPI_COMM_WORLD, NULL);
		topology[1] = (int*) malloc(nr_workers[1] * sizeof(int));
		MPI_Recv(topology[1], nr_workers[1], MPI_INT, 1, 0, MPI_COMM_WORLD, NULL);
		// afiseaza topologia completa si o trimite proceselor worker
		topology[2] = workers;
		printf("%d -> ", rank);
		for (int i = 0; i < 3; i++) {
			printf("%d:", i);
			for (int j = 0; j < nr_workers[i]; j++) {
				if (j == nr_workers[i] - 1)
					printf("%d ", topology[i][j]);
				else
					printf("%d,", topology[i][j]);
			}
		}
		printf("\n");
		// trimite topologiile clusterelor 1 si 2 procesului 0
		my_send(&nr_workers[1], 1, rank, 0);
		my_send(topology[1], nr_workers[1], rank, 0);
		my_send(&nr_workers[2], 1, rank, 0);
		my_send(workers, nr_workers[2], rank, 0);
		for (int i = 0; i < nr_workers[2]; i++) {
			my_send(&rank, 1, rank, topology[2][i]);
			my_send(nr_workers, 3, rank, topology[2][i]);
			my_send(topology[0], nr_workers[0], rank, topology[2][i]);
			my_send(topology[1], nr_workers[1], rank, topology[2][i]);
			my_send(topology[2], nr_workers[2], rank, topology[2][i]);
		}
		// partea2
		int n_1, *v_1;
		// primeste vectorul pentru procesul 1
		MPI_Recv(&n_1, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
		v_1 = (int*) malloc(n_1 * sizeof(int));
		MPI_Recv(v_1, n_1, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
		my_send(&n_1, 1, rank, 1);
		my_send(v_1, n_1, rank, 1);
		// primeste vectorul care trebuie modificat de workerii sai
		MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
		int *v = (int*) malloc(n * sizeof(int));
		MPI_Recv(v, n, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
		int n_worker = (double) n / nr_workers[2];
		int remaining = n;
		for (int i = 0; i < nr_workers[2] - 1; i++) {
			// trimite vectorul workerilor
			my_send(&n_worker, 1, rank, workers[i]);
			my_send(v + n_worker * i, n_worker, rank, workers[i]);
			remaining -= n_worker;
		}
		// portiunea de vector ramasa(daca nu se imparte egal) este trimita ultimului worker
		my_send(&remaining, 1, rank, workers[nr_workers[2] - 1]);
		my_send(v + n_worker * (nr_workers[2] - 1), remaining, rank, workers[nr_workers[2] - 1]);
		for (int i = 0; i < nr_workers[2] - 1; i++) {
			MPI_Recv(v + n_worker * i, n_worker, MPI_INT, workers[i], 0, MPI_COMM_WORLD, NULL);
		}
		// primeste vectorul de la procesul 1 si trimite ambii vectori procesului 0
		MPI_Recv(v + n_worker * (nr_workers[2] - 1), remaining, MPI_INT, workers[nr_workers[2] - 1], 0, MPI_COMM_WORLD, NULL);
		MPI_Recv(v_1, n_1, MPI_INT, 1, 0, MPI_COMM_WORLD, NULL);
		my_send(v_1, n_1, rank, 0);
		my_send(v, n, rank, 0);
	} else {
		// primesc leaderul si topologia completa de la leader
		MPI_Recv(&leader, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, NULL);
		MPI_Recv(nr_workers, 3, MPI_INT, leader, 0, MPI_COMM_WORLD, NULL);
		topology[0] = (int*) malloc(nr_workers[0] * sizeof(int));
		topology[1] = (int*) malloc(nr_workers[1] * sizeof(int));
		topology[2] = (int*) malloc(nr_workers[2] * sizeof(int));
		MPI_Recv(topology[0], nr_workers[0], MPI_INT, leader, 0, MPI_COMM_WORLD, NULL);
		MPI_Recv(topology[1], nr_workers[1], MPI_INT, leader, 0, MPI_COMM_WORLD, NULL);
		MPI_Recv(topology[2], nr_workers[2], MPI_INT, leader, 0, MPI_COMM_WORLD, NULL);
		printf("%d -> ", rank);
		for (int i = 0; i < 3; i++) {
			printf("%d:", i);
			for (int j = 0; j < nr_workers[i]; j++) {
				if (j == nr_workers[i] - 1)
					printf("%d ", topology[i][j]);
				else
					printf("%d,", topology[i][j]);
			}
		}
		printf("\n");
		//partea2
		int n, *v;
		// primesc portiunea de vector proprie, o inmultesc cu 2 si o trimit inapoi
		MPI_Recv(&n, 1, MPI_INT, leader, 0, MPI_COMM_WORLD, NULL);
		v = (int*) malloc(n * sizeof(int));
		MPI_Recv(v, n, MPI_INT, leader, 0, MPI_COMM_WORLD, NULL);
		for (int i = 0; i < n; i++)
			v[i] *= 2;
		my_send(v, n, rank, leader);
	}

	MPI_Finalize();
	return 0;
}
