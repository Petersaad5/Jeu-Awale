#include <stdio.h>
#include <string.h> // Include the necessary header file for strlen
#include "awale.h"

// Initialisation du plateau
void initialiser_plateau(PlateauAwale *plateau) {
    for (int i = 0; i < NBR_CASES; i++) {
        plateau->cases[i] = GRAINES_INITIALES;
    }
    plateau->score_joueur1 = 0;
    plateau->score_joueur2 = 0;
}

// Jouer un coup
int jouer_coup(PlateauAwale *plateau, int joueur, int case_selectionnee) {
    int graines = plateau->cases[case_selectionnee];
    plateau->cases[case_selectionnee] = 0;
    int index = case_selectionnee;

    while (graines > 0) {
        index = (index + 1) % NBR_CASES;
        if (index != case_selectionnee) {
            plateau->cases[index]++;
            graines--;
        }
    }

    int graines_capturees = capturer_graines(plateau, joueur, index);
    if (joueur == 1) {
        plateau->score_joueur1 += graines_capturees;
    } else {
        plateau->score_joueur2 += graines_capturees;
    }

    // Check if the game should end after this move
    if (est_fin_de_partie(plateau)) {
        return 1; // Game over
    }

    return 0; // Game continues
}

// Vérifier si la partie est terminée
int est_fin_de_partie(PlateauAwale *plateau) {
    if (plateau->score_joueur1 >= SCORE_VICTOIRE || plateau->score_joueur2 >= SCORE_VICTOIRE) {
        return 1;
    }

    int graines_joueur1 = compter_graines(plateau, 1);
    int graines_joueur2 = compter_graines(plateau, 2);

    if (graines_joueur1 == 0 || graines_joueur2 == 0) {
        return 1;
    }

    return 0;
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

// Capturer des graines
int capturer_graines(PlateauAwale *plateau, int joueur, int case_finale) {
    int graines_capturees = 0;
    int index = case_finale;

    while (index >= 0 && index < NBR_CASES && (plateau->cases[index] == 2 || plateau->cases[index] == 3)) {
        graines_capturees += plateau->cases[index];
        plateau->cases[index] = 0;
        index = (index - 1 + NBR_CASES) % NBR_CASES;
    }

    return graines_capturees;
}

// Nourrir l'adversaire
int nourrir_adversaire(PlateauAwale *plateau, int joueur) {
    int adversaire = (joueur == 1) ? 2 : 1;
    return compter_graines(plateau, adversaire) > 0;
}

// Compter les graines dans le camp d'un joueur
int compter_graines(const PlateauAwale *plateau, int joueur) {
    int debut = (joueur == 1) ? 0 : 6;
    int fin = debut + 6;
    int total = 0;
    for (int i = debut; i < fin; i++) {
        total += plateau->cases[i];
    }
    return total;
}

// Get the board state as a string
void get_board_state(const PlateauAwale *plateau, char *buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "Joueur 2: ");
    for (int i = 11; i >= 6; i--) {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "[%d] ", plateau->cases[i]);
    }
    snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "\nJoueur 1: ");
    for (int i = 0; i < 6; i++) {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "[%d] ", plateau->cases[i]);
    }
    snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "\nScore - Joueur 1: %d, Joueur 2: %d\n", plateau->score_joueur1, plateau->score_joueur2);
}
