#include <stdio.h>
#include "awale.h"

int main() {
    PlateauAwale plateau;
    initialiser_plateau(&plateau);

    int joueur = 1;
    int case_selectionnee;

    // Simulate a series of moves
    int moves[] = {1, 7, 2, 8, 3, 9, 4, 10, 5, 11, 0, 6}; // Example moves
    int num_moves = sizeof(moves) / sizeof(moves[0]);

    for (int i = 0; i < num_moves; i++) {
        case_selectionnee = moves[i];
        printf("Joueur %d joue le coup depuis la case %d\n", joueur, case_selectionnee);
        if (jouer_coup(&plateau, joueur, case_selectionnee)) {
            afficher_plateau(&plateau); // Affiche le plateau après le coup
            printf("La partie est terminée\n");
            printf("Score final - Joueur 1: %d, Joueur 2: %d\n", plateau.score_joueur1, plateau.score_joueur2);
            return 0;
        }
        afficher_plateau(&plateau); // Affiche le plateau après le coup

        // Switch player
        joueur = (joueur == 1) ? 2 : 1;
    }

    printf("La partie n'est pas terminée\n");
    return 0;
}