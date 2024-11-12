#include <stdio.h>
#include "awale.h"

// Initialisation du plateau
void initialiser_plateau(PlateauAwale *plateau) {
    for (int i = 0; i < NBR_CASES; i++) {
        plateau->cases[i] = GRAINES_INITIALES;
    }
    plateau->score_joueur1 = 0;
    plateau->score_joueur2 = 0;
}

// Fonction pour jouer un coup
int jouer_coup(PlateauAwale *plateau, int joueur, int case_selectionnee) {
    int graines = plateau->cases[case_selectionnee];
    plateau->cases[case_selectionnee] = 0;
    int index = case_selectionnee;
    
    // Distribution des graines (sens anti-horaire)
    while (graines > 0) {
        index = (index + 1) % NBR_CASES;
        if (index != case_selectionnee) {  // Sauter la case de départ
            plateau->cases[index]++;
            graines--;
        }
    }

    // Capture des graines si possible
    int graines_capturees = capturer_graines(plateau, joueur, index);

    if (joueur == 1) plateau->score_joueur1 += graines_capturees;
    else plateau->score_joueur2 += graines_capturees;

    // Vérifie si l'adversaire est "affamé" et doit être nourri
    if (!nourrir_adversaire(plateau, joueur % 2 + 1)) {
        return 0; // Coup non valide, il faut nourrir l'adversaire
    }

    return 1; // Coup valide
}

// Capture des graines dans le camp adverse
int capturer_graines(PlateauAwale *plateau, int joueur, int case_finale) {
    int capture = 0;
    int adversaire = (joueur == 1) ? 7 : 0;  // Les cases de l'adversaire

    // Vérifier si la dernière graine est tombée dans le camp adverse
    if (case_finale >= adversaire && case_finale < adversaire + 6) {
        // Capturer à partir de la case finale
        while (case_finale >= adversaire && case_finale < adversaire + 6 &&
               (plateau->cases[case_finale] == 2 || plateau->cases[case_finale] == 3)) {
            capture += plateau->cases[case_finale];
            plateau->cases[case_finale] = 0;
            case_finale--;
        }
    }

    return capture;
}

// Vérifie si la partie doit s'arrêter
int est_fin_de_partie(PlateauAwale *plateau) {
    return plateau->score_joueur1 >= SCORE_VICTOIRE || plateau->score_joueur2 >= SCORE_VICTOIRE || 
           (compter_graines(plateau, 1) == 0 && !nourrir_adversaire(plateau, 1)) ||
           (compter_graines(plateau, 2) == 0 && !nourrir_adversaire(plateau, 2));
}

// Vérifie s'il est possible de nourrir l'adversaire
int nourrir_adversaire(PlateauAwale *plateau, int joueur) {
    int adversaire = (joueur == 1) ? 2 : 1;
    return compter_graines(plateau, adversaire) > 0;
}

// Compte le nombre de graines dans le camp d'un joueur
int compter_graines(const PlateauAwale *plateau, int joueur) {
    int debut = (joueur == 1) ? 0 : 6;
    int fin = debut + 6;
    int total = 0;

    for (int i = debut; i < fin; i++) {
        total += plateau->cases[i];
    }
    return total;
}

// Affichage du plateau
void afficher_plateau(const PlateauAwale *plateau) {
    printf("Joueur 2: ");
    for (int i = 11; i >= 6; i--) {
        printf("[%d] ", plateau->cases[i]);
    }
    printf("\n");

    printf("Joueur 1: ");
    for (int i = 0; i < 6; i++) {
        printf("[%d] ", plateau->cases[i]);
    }
    printf("\n");

    printf("Score - Joueur 1: %d, Joueur 2: %d\n", plateau->score_joueur1, plateau->score_joueur2);
}
