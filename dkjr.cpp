#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <SDL/SDL.h>
#include "./presentation/presentation.h"

#define VIDE        		0
#define DKJR       		1
#define CROCO       		2
#define CORBEAU     		3
#define CLE 			4

#define AUCUN_EVENEMENT    	0

#define LIBRE_BAS		1
#define LIANE_BAS		2
#define DOUBLE_LIANE_BAS	3
#define LIBRE_HAUT		4
#define LIANE_HAUT		5

void* FctThreadEvenements(void *);
void* FctThreadCle(void *);
void* FctThreadDK(void *);
void* FctThreadDKJr(void *);
void* FctThreadScore(void *);
void* FctThreadEnnemis(void *);
void* FctThreadCorbeau(void *);
void* FctThreadCroco(void *);

void initGrilleJeu();
void setGrilleJeu(int l, int c, int type = VIDE, pthread_t tid = 0);
void afficherGrilleJeu();

void HandlerSIGUSR1(int);
void HandlerSIGUSR2(int);
void HandlerSIGALRM(int);
void HandlerSIGINT(int);
void HandlerSIGQUIT(int);
void HandlerSIGCHLD(int);
void HandlerSIGHUP(int);

void DestructeurVS(void *p);

pthread_t threadCle;
pthread_t threadDK;
pthread_t threadDKJr;
pthread_t threadEvenements;
pthread_t threadScore;
pthread_t threadEnnemis;

pthread_cond_t condDK;
pthread_cond_t condScore;

pthread_mutex_t mutexGrilleJeu;
pthread_mutex_t mutexDK;
pthread_mutex_t mutexEvenement;
pthread_mutex_t mutexScore;

pthread_key_t keySpec;

bool MAJDK = false;
int  score = 0;
bool MAJScore = true;
int  delaiEnnemis = 4000;
int  positionDKJr = 1;
int  evenement = AUCUN_EVENEMENT;
int etatDKJr;

typedef struct
{
  int type;
  pthread_t tid;
} S_CASE;

S_CASE grilleJeu[4][8];

typedef struct
{
  bool haut;
  int position;
} S_CROCO;

// ------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	int evt, vie = 3;

	ouvrirFenetreGraphique();

	sigset_t mask;

	sigfillset(&mask);

	sigprocmask(SIG_SETMASK, &mask, NULL);


	struct sigaction A;
	A.sa_handler = HandlerSIGQUIT;
	sigemptyset(&A.sa_mask);
	A.sa_flags = 0;
	sigaction(SIGQUIT, &A, NULL);



	pthread_mutex_init(&mutexGrilleJeu, NULL);
	pthread_mutex_init(&mutexEvenement, NULL);
	pthread_mutex_init(&mutexDK, NULL);

	pthread_create(&threadCle, NULL, FctThreadCle, NULL);

	pthread_create(&threadEvenements, NULL, FctThreadEvenements, NULL);

	pthread_create(&threadDKJr, NULL, FctThreadDKJr, NULL);

	pthread_create(&threadDK, NULL, FctThreadDK, NULL);
	
	while(vie > 0)
	{	
		int nberror = 3;
		pthread_join(threadDKJr, NULL);

		vie--;

		nberror -= vie;

		afficherEchec(nberror);

		pthread_create(&threadDKJr, NULL, FctThreadDKJr, NULL);
		
	}
	
	
	pthread_join(threadCle, NULL);
	pthread_join(threadEvenements, NULL);
	

	
}

// -------------------------------------

void initGrilleJeu()
{
  int i, j;   
  
  pthread_mutex_lock(&mutexGrilleJeu);

  for(i = 0; i < 4; i++)
    for(j = 0; j < 7; j++)
      setGrilleJeu(i, j);

  pthread_mutex_unlock(&mutexGrilleJeu);
}

// -------------------------------------

void setGrilleJeu(int l, int c, int type, pthread_t tid)
{
  grilleJeu[l][c].type = type;
  grilleJeu[l][c].tid = tid;
}

// -------------------------------------

void afficherGrilleJeu()
{   
   for(int j = 0; j < 4; j++)
   {
       for(int k = 0; k < 8; k++)
          printf("%d  ", grilleJeu[j][k].type);
       printf("\n");
   }

   printf("\n");   
}

void* FctThreadEvenements(void *)
{	
	int evt;
	struct timespec temps;
	temps.tv_nsec = 10000000;
	temps.tv_sec = 0;

	while (1)
	{
	    evt = lireEvenement();
		pthread_mutex_lock(&mutexEvenement);
	    
		switch (evt)
	    {
			case SDL_QUIT:
				pthread_mutex_unlock(&mutexEvenement);
				exit(0);
			
			case SDLK_UP:
				printf("KEY_UP\n");
				evenement = SDLK_UP;

				break;
			case SDLK_DOWN:
				printf("KEY_DOWN\n");
				evenement = SDLK_DOWN;
				break;
			case SDLK_LEFT:
				printf("KEY_LEFT\n");
				evenement = SDLK_LEFT;
				
				break;
			case SDLK_RIGHT:
				printf("KEY_RIGHT\n");
				evenement = SDLK_RIGHT;
				
	    }
		pthread_mutex_unlock(&mutexEvenement);
		kill(getpid(), SIGQUIT);
		nanosleep(&temps, NULL);


		pthread_mutex_lock(&mutexEvenement);
		evenement = AUCUN_EVENEMENT;
		pthread_mutex_unlock(&mutexEvenement);
	}

}









void* FctThreadCle(void *)
{
	int i = 1, j = 1;
	

	timespec t;

	t.tv_nsec = 700000000;
	t.tv_sec = 0;
	
	
	while (1)
	{	
		pthread_mutex_lock(&mutexGrilleJeu);
		effacerCarres(3, 12, 2, 4);
		afficherCle(i);
		pthread_mutex_unlock(&mutexGrilleJeu);

		nanosleep(&t, NULL);

		if(i == 4)
			j = -1;
		
		if(i == 2)
		{
			grilleJeu[0][1].type = CLE;
		}
		else
		{
			grilleJeu[0][1].type = VIDE;
		}

		if(i == 1)
			j = 1;
		
		i += j;

		
	}

}

void* FctThreadDKJr(void* p)
{

	struct timespec temps;
	temps.tv_sec = 0;
	temps.tv_nsec = 500000000;

	sigset_t mask_DKJr;

	sigfillset(&mask_DKJr);

	sigdelset(&mask_DKJr, SIGQUIT);

	sigprocmask(SIG_SETMASK, &mask_DKJr, NULL);


	bool on = true; 
	pthread_mutex_lock(&mutexGrilleJeu);
	
	setGrilleJeu(3, 1, DKJR); 
	afficherDKJr(11, 9, 1); 
	etatDKJr = LIBRE_BAS; 
	positionDKJr = 1;


	
	pthread_mutex_unlock(&mutexGrilleJeu);
	while(on)
	{
		pause();
		printf("gauche");
		pthread_mutex_lock(&mutexEvenement);
		pthread_mutex_lock(&mutexGrilleJeu);
		switch (etatDKJr)
		{
			case LIBRE_BAS:
				switch (evenement)
				{
					case SDLK_LEFT:
					if (positionDKJr > 1)
					{	
						printf("gauche");
						setGrilleJeu(3, positionDKJr);
						effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
						positionDKJr--;
						setGrilleJeu(3, positionDKJr, DKJR);
						afficherDKJr(11, (positionDKJr * 2) + 7, 
						((positionDKJr - 1) % 4) + 1);
					}
					break;
					case SDLK_RIGHT:
						if (positionDKJr < 7)
						{	

							if(positionDKJr == 0)
							{
								effacerCarres(11, 7, 2, 2);
							}
							printf("droit");
							setGrilleJeu(3, positionDKJr);
							effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);
							positionDKJr++;
							setGrilleJeu(3, positionDKJr, DKJR);
							afficherDKJr(11, (positionDKJr * 2) + 7, 
							((positionDKJr - 1) % 4) + 1);
						}
						break;
					case SDLK_UP:

							if( (positionDKJr >= 2 && positionDKJr <= 4) || positionDKJr == 6)
							{
								setGrilleJeu(3, positionDKJr);
								effacerCarres(11, (positionDKJr * 2) + 7, 2, 2); //On efface DKJR de l'emplacement en bas

								setGrilleJeu(2, positionDKJr, DKJR);
								afficherDKJr(10, (positionDKJr * 2) + 7, 8); //on actualise sa position quand il a sauté
								
								printf("State: LIBRE_BAS --- Event: KEY_UP\n");
								afficherGrilleJeu();

								pthread_mutex_unlock(&mutexGrilleJeu);
								nanosleep(&temps, NULL);
								pthread_mutex_lock(&mutexGrilleJeu);

								setGrilleJeu(2, positionDKJr);
								effacerCarres(10, (positionDKJr * 2) + 7, 2, 2); //On efface DKJR d'en haut pour ensuite le refaire tomber (effet de gravité)

								setGrilleJeu(3, positionDKJr, DKJR);
								afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
								
								printf("State: LIBRE_BAS --- Event: Gravity after KEY_UP\n");
								afficherGrilleJeu();
							}
							else
							{
								setGrilleJeu(3, positionDKJr);
								effacerCarres(11, (positionDKJr * 2) + 7, 2, 2);

								setGrilleJeu(2, positionDKJr, DKJR);
								if(positionDKJr == 7)
								{
									etatDKJr = DOUBLE_LIANE_BAS;
									afficherDKJr(10, (positionDKJr * 2) + 7, 5); //On affiche l'image 8 car DKJR se tient aux deux lianes
								}
								else
								{
									etatDKJr = LIANE_BAS;
									afficherDKJr(10, (positionDKJr * 2) + 7, 7);
								}
								
								printf("State: LIBRE_BAS --- Event: KEY_UP LIANE\n");
								afficherGrilleJeu();
							}
						break;
					
				}
			case LIANE_BAS:

				switch(evenement)
				{
					case SDLK_DOWN:
					{
						etatDKJr = LIBRE_BAS;
						setGrilleJeu(2, positionDKJr);
						effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

						setGrilleJeu(3, positionDKJr, DKJR);
						afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

						printf("State: LIANE_BAS --- Event: KEY_DOWN\n");
						afficherGrilleJeu();
					}
					break;

				}
				break;
			
			case DOUBLE_LIANE_BAS:

				switch(evenement)
				{
					case SDLK_DOWN:
					{
						etatDKJr = LIBRE_BAS;
						setGrilleJeu(2, positionDKJr);
						effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

						setGrilleJeu(3, positionDKJr, DKJR);
						afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

						printf("State: LIANE_BAS --- Event: KEY_DOWN\n");
						afficherGrilleJeu();
					}
					break;

					case SDLK_UP:
					{
						etatDKJr = LIBRE_HAUT;
						setGrilleJeu(2, positionDKJr);
						effacerCarres(10, (positionDKJr * 2) + 7, 2, 2);

						setGrilleJeu(1, positionDKJr, DKJR);
						afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

						printf("State: DOUBLE_LIANE_BAS --- Event: KEY_UP\n");
						afficherGrilleJeu();
					}
					break;

				}
			
				break;
			case LIBRE_HAUT:

				switch(evenement)
				{
					case SDLK_DOWN:
						if(positionDKJr == 7)
						{
							etatDKJr = DOUBLE_LIANE_BAS;
							setGrilleJeu(1, positionDKJr);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
							
							setGrilleJeu(2, positionDKJr, DKJR);
							afficherDKJr(10, (positionDKJr * 2) + 7, 5);

							printf("State: LIBRE_HAUT --- Event: KEY_DOWN\n");
							afficherGrilleJeu();
						}
						break;

					case SDLK_LEFT:
						if(positionDKJr > 3)
						{
							setGrilleJeu(1, positionDKJr);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
							positionDKJr--;
							setGrilleJeu(1, positionDKJr, DKJR);
							afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

							printf("State: LIBRE_HAUT --- Event: KEY_LEFT\n");
							afficherGrilleJeu();
						}
						else
						{
							if(positionDKJr == 3)
							{
								if(grilleJeu[0][1].type == CLE)
								{
									setGrilleJeu(1, positionDKJr);
									effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
									etatDKJr = LIBRE_BAS;
									afficherDKJr(5, 12, 9);
									nanosleep(&temps, NULL);
									effacerCarres(5, 12, 3, 2);

									effacerCarres(3, 12, 2, 4);
									
									afficherDKJr(3, 11, 10);

									nanosleep(&temps, NULL);
									effacerCarres(3, 11, 3, 2);
									afficherCage(4);
									MAJDK = true;
									pthread_cond_signal(&condDK);
									positionDKJr = 1;
									setGrilleJeu(3, positionDKJr, DKJR);
									afficherDKJr(11, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
									afficherGrilleJeu();

								}
								else
								{
									setGrilleJeu(1, positionDKJr);
									effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);
									afficherGrilleJeu();
									etatDKJr = LIBRE_BAS;

									
									afficherDKJr(14, 9 , 9);
									nanosleep(&temps, NULL);
									effacerCarres(5, 12, 3, 3);

									afficherDKJr(6, 11 , 12);
									nanosleep(&temps, NULL);
									effacerCarres(6, 11, 3, 3);

									afficherDKJr(14, 9 , 13);
									nanosleep(&temps, NULL);

									positionDKJr = 0;
									setGrilleJeu(3, positionDKJr, DKJR);

									afficherGrilleJeu();
									pthread_mutex_unlock(&mutexGrilleJeu);
									pthread_mutex_unlock(&mutexEvenement);
									pthread_exit(0);



								}
							}
						}
						break;

					case SDLK_RIGHT:
						if(positionDKJr < 7)
						{
							setGrilleJeu(1, positionDKJr);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

							if(evenement == SDLK_LEFT)
								positionDKJr--;
							else
								positionDKJr++;

							setGrilleJeu(1, positionDKJr, DKJR);
							afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

							printf("State: LIBRE_HAUT --- Event: KEY_RIGHT\n");
							afficherGrilleJeu();
						}
						break;

					case SDLK_UP:
						if(positionDKJr == 3 || positionDKJr == 4)
						{
							setGrilleJeu(1, positionDKJr);
							effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

							setGrilleJeu(0, positionDKJr, DKJR);
							afficherDKJr(6, (positionDKJr * 2) + 7, 8);

							printf("State: LIBRE_HAUT --- Event: KEY_UP\n");
							afficherGrilleJeu();

							pthread_mutex_unlock(&mutexGrilleJeu);
							nanosleep(&temps, NULL);
							pthread_mutex_lock(&mutexGrilleJeu);

							setGrilleJeu(0, positionDKJr);
							effacerCarres(6, (positionDKJr * 2) + 7, 2, 2); //On efface DKJR d'en haut pour ensuite le refaire tomber (effet de gravité)

							setGrilleJeu(1, positionDKJr, DKJR);
							afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);
							
							printf("State: LIBRE_HAUT --- Event: Gravity after KEY_UP\n");
							afficherGrilleJeu();
						}
						else
						{
							if(positionDKJr == 6)
							{
								etatDKJr = LIANE_HAUT;
								setGrilleJeu(1, positionDKJr);
								effacerCarres(7, (positionDKJr * 2) + 7, 2, 2);

								setGrilleJeu(0, positionDKJr, DKJR);
								afficherDKJr(6, (positionDKJr * 2) + 7, 7);

								printf("State: LIBRE_HAUT --- Event: KEY_UP LIANE\n");
								afficherGrilleJeu();
							}
						}
						break;
				}
			
			
				break;
			case LIANE_HAUT:

				switch(evenement)
				{
					case SDLK_DOWN:
					{
						etatDKJr = LIBRE_HAUT;
						setGrilleJeu(0, positionDKJr);
						effacerCarres(6, (positionDKJr * 2) + 7, 2, 2);

						setGrilleJeu(1, positionDKJr, DKJR);
						afficherDKJr(7, (positionDKJr * 2) + 7, ((positionDKJr - 1) % 4) + 1);

						printf("State: LIANE_HAUT --- Event: KEY_DOWN\n");
						afficherGrilleJeu();
					}
					break;
				}

				break;


		}
			pthread_mutex_unlock(&mutexGrilleJeu);
			pthread_mutex_unlock(&mutexEvenement);
	}
		pthread_exit(0);
}

void* FctThreadDK(void *)
{	
	int i = 0;
	struct timespec temps;
	temps.tv_nsec = 700000000;
	temps.tv_sec = 0;
	while(i < 5)
	{
		afficherCage(i);
		i++;
	}
	i = 1;
	while(1)
	{
		
		
	
		pthread_cond_wait(&condDK, &mutexDK);

		if(i < 4)
		{
			switch (i)
			{
				case 1:
					effacerCarres(2, 7, 2, 2);
					break;
				
				case 2:
					effacerCarres(4, 7, 2, 2);
					break;

				case 3:
					effacerCarres(2, 9, 2, 2);
					break;
			
				
			}
			i++;
		}
		else
		{
			effacerCarres(4, 9, 2, 2);
			afficherRireDK();
			nanosleep(&temps, NULL);

			effacerCarres(3, 8, 2, 2);
			i = 0;
			while(i < 5)
			{
				afficherCage(i);
				i++;
			}
			i = 1;
		}

	}

	pthread_exit(0);
}



void HandlerSIGQUIT(int)
{
	printf("coucou ");
}