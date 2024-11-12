#ifndef AWALE_H
#define AWALE_H

#define NBR_CASES 12             // 6 cases par joueur
#define GRAINES_INITIALES 4      // Graines initiales par case
#define SCORE_VICTOIRE 25        // Nombre de graines nécessaires pour gagner

// Structure pour représenter le plateau et les scores
typedef struct {
    int cases[NBR_CASES];        // Cases du plateau
    int score_joueur1;           // Score du joueur 1
    int score_joueur2;           // Score du joueur 2
} PlateauAwale;

// Fonctions principales pour le jeu Awalé
void initialiser_plateau(PlateauAwale *plateau);
int jouer_coup(PlateauAwale *plateau, int joueur, int case_selectionnee);
int est_fin_de_partie(PlateauAwale *plateau);
void afficher_plateau(const PlateauAwale *plateau);
int capturer_graines(PlateauAwale *plateau, int joueur, int case_finale);
int nourrir_adversaire(PlateauAwale *plateau, int joueur);
int compter_graines(const PlateauAwale *plateau, int joueur);

#endif
