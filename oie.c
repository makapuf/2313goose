/* jeu de l'oie electronique */

#include <stdint.h>
#include <stdlib.h>
// hardware functions
#include "hw.h"

#define MAX_PLAYER 6

enum sons {

	DONTUSE,

	SND_01,SND_02,SND_03,SND_04,SND_05,SND_06,SND_07,SND_08,SND_09,SND_10,
	SND_11,SND_12,SND_13,SND_14,SND_15,SND_16,
	SND_20,SND_30,

	SND_ET,
	SND_INTRO,
	SND_JOUEUR,
	SND_APPUYER_PR_AJOUTER,SND_OK_JOUER_AVEC,
	SND_PASSE_SON_TOUR,SND_TU_PASSES, SND_TOUR,
	SND_APPUIE_PR_ARRETER_LA_ROUE,SND_ROUE,SND_FIN_ROUE,
	SND_TU_AS_FAIT,SND_TU_VAS_EN_CASE,
	SND_BRAVO_TU_AS_GAGNE,
	SND_AU_SUIVANT,
	SND_NOUVELLE_PARTIE,
	SND_ERROR,

	SND_VA_A_LA_CASE,
	SND_CHAUSSETTE,
	SND_TRAIN,
	SND_PRISON,
	SND_DEPART,
	SND_NOUNOURS,
	SND_TROISTOURS,
	SND_DESSINE_TINTIN,
	SND_GROSIMBECILE,
	SND_PAYE5,
	SND_DESSINE_VAISSEAU,
	SND_RECOIS_5,
	SND_FAISSINGE,
	SND_BOIS3ML,
	SND_PETOMANE,
	SND_MEGAFOURIRE,
	SND_DARKMAUL,
	SND_THCATCHATCHA,
	SND_PRENDS_5E,
	SND_TUAS_PERDU_TA_CHAUSSURE,

};

// should be exported by stdint
int rand (void); // returns a random value of 0x7fff max

uint8_t nb_joueurs;


void play_nb(uint8_t number)
{
    if (number<=16) 
    {
        play(number);
    }    
    else  if (number >21 && number <=29)
    { 
    	play(SND_20); 
    	play(number-20);
    }
    else 
    {
        switch (number)
        {
            case 17: play(SND_10); play(SND_07); break;
            case 18: play(SND_10); play(SND_08); break;
            case 19: play(SND_10); play(SND_09); break;
            case 20: play(SND_20); break;
            case 21: play(SND_20); play(SND_ET); play(SND_01);break;
            case 30: play(SND_30); break;
            case 31: play(SND_30); play(SND_ET); play(SND_01);break;
            case 32: play(SND_30); play(SND_02);break;
            default : play(SND_ERROR);
        } 
    }
}

void select_nbjoueurs()
// selectionne nb joueurs 1-6 , sur timeout
{
	uint8_t ret=0;
	play(SND_INTRO);
	nb_joueurs=0;
	while (nb_joueurs<MAX_PLAYER && ret!=BUT_TIMEOUT)
	{
		nb_joueurs ++;
		play_nb(nb_joueurs);
		play(SND_JOUEUR);
		if (nb_joueurs<MAX_PLAYER) play(SND_APPUYER_PR_AJOUTER);
		ret = wait_but(20); // attend un bouton, avec timeout en 1/10s sec.
	}
	play(SND_OK_JOUER_AVEC);
	play_nb(nb_joueurs);
	play(SND_JOUEUR);
}


void partie()
{
	uint8_t case_joueur[MAX_PLAYER];
	uint8_t joueur_passe[MAX_PLAYER]; // nb de tours à passer

	uint8_t i;
	uint8_t fini=0;
	uint8_t joueur; // a qui le tour,0-based
	uint8_t de; // dice ! 1-6

	for (i=0;i<MAX_PLAYER;i++) {
		case_joueur[i]=0;
		joueur_passe[i]=0;
	}

	joueur = 0;

	while (!fini)
	{
		play(SND_JOUEUR); play_nb(joueur+1);
		if (joueur_passe[joueur])
		{
			play(SND_PASSE_SON_TOUR);
			joueur_passe[joueur]-=1;
		}
		else
		{

			play_nowait(SND_APPUIE_PR_ARRETER_LA_ROUE);
			wait_but(100);
			stop();
			play(SND_FIN_ROUE);

			// lance le de
			de = (((rand()&0xff)*6)>>8)+1;

			play(SND_TU_AS_FAIT);
			play_nb(de);
			case_joueur[joueur]+=de;

			// cas dépasse 32
			if (case_joueur[joueur]>32) case_joueur[joueur] = 64-case_joueur[joueur];

			play(SND_TU_VAS_EN_CASE);
			play_nb(case_joueur[joueur]);
			wait_but(40);
			
			//

			switch(case_joueur[joueur])
			{
				case 1 : play(SND_VA_A_LA_CASE);play_nb(30);case_joueur[joueur]=30;break;
				case 2 : play(SND_VA_A_LA_CASE);play_nb(18);case_joueur[joueur]=18;break;
				case 3 : 
				case 4 : play(SND_VA_A_LA_CASE);play_nb(6);case_joueur[joueur]=6;break;
				case 5 : play(SND_VA_A_LA_CASE);play_nb(3);case_joueur[joueur]=3;break;
				case 6 : play(SND_CHAUSSETTE);break;
				case 7 : play(SND_VA_A_LA_CASE);play_nb(32);play(SND_BRAVO_TU_AS_GAGNE);fini=1;break; // Arrivee
				case 8 : play(SND_VA_A_LA_CASE);play_nb(12);case_joueur[joueur]=12;break;
				case 9 : play(SND_TRAIN);break;
				case 10 : play(SND_PRISON);play(SND_TU_PASSES);play_nb(2);play(SND_TOUR);joueur_passe[joueur]=2;break;
				case 20 : 
				case 11 : play(SND_VA_A_LA_CASE);play(SND_DEPART);break;
				case 12 : play(SND_NOUNOURS);break;
				case 13 : play(SND_TROISTOURS);break;
				case 14 : play(SND_DESSINE_TINTIN);break;
				case 15 : play(SND_VA_A_LA_CASE);play(SND_PRISON);break;
				case 16 : play(SND_GROSIMBECILE);break;
				case 17 : play(SND_PAYE5);break;	
				case 18 : play(SND_DESSINE_VAISSEAU);break;
				case 19 : play(SND_RECOIS_5);break;
				// 20 : cf 11
				case 26 : 
				case 21 : play(SND_TU_PASSES);play_nb(1);play(SND_TOUR);joueur_passe[joueur]=1;break;
				case 22 : play(SND_FAISSINGE);break;
				case 23 : play(SND_BOIS3ML);break;
				case 24 : break; // ???
				case 25 : play(SND_PETOMANE);break;
				// case 26 : idem 21
				case 27 : play(SND_MEGAFOURIRE);break;
				case 28 : play(SND_DARKMAUL);break;
				case 29 : play(SND_THCATCHATCHA);break;
				case 30 : play (SND_PRENDS_5E);break;
				case 31 : play(SND_TUAS_PERDU_TA_CHAUSSURE);play(SND_VA_A_LA_CASE);play_nb(6);case_joueur[joueur]=6;break;
				case 32 : play(SND_BRAVO_TU_AS_GAGNE);fini=1;break;
			}
		}

		if (!fini) 
		{
			joueur += 1; 
			if (joueur>=nb_joueurs) joueur=0;
			wait_but(200);
			play(SND_AU_SUIVANT); 
		}
	}
}

int main()
{
	setup();
	wait_but(1);
	while(1) 
	{
		select_nbjoueurs();
		partie();
		// sleep / interrupts & restart ? see http://playground.arduino.cc/Learning/arduinoSleepCode + http://www.nongnu.org/avr-libc/user-manual/group__avr__sleep.html
		play(SND_NOUVELLE_PARTIE);
	} // never returns
}