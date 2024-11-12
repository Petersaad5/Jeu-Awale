#include <stdio.h>
#include "awale.h"

int main() {
    PlateauAwale plateau;
    initialiser_plateau(&plateau);
    
    // Affichage initial du plateau
    afficher_plateau(&plateau);

    // Joueur 1 joue depuis la case 5
    printf("Joueur 1 joue le coup depuis la case 5\n");
    jouer_coup(&plateau, 1, 5);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 7
    printf("Joueur 2 joue le coup depuis la case 7\n");
    jouer_coup(&plateau, 2, 7);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 4 (cette action devrait entraîner une capture)
    printf("Joueur 1 joue le coup depuis la case 4\n");
    jouer_coup(&plateau, 1, 4);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 11
    printf("Joueur 2 joue le coup depuis la case 11\n");
    jouer_coup(&plateau, 2, 11);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    printf("Joueur 1 joue le coup depuis la case 2\n");
    jouer_coup(&plateau, 1, 2);
    afficher_plateau(&plateau); // Affiche le plateau après le coup


    // Joueur 2 joue depuis la case 6
    printf("Joueur 2 joue le coup depuis la case 6\n");
    jouer_coup(&plateau, 2, 6);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

     // Joueur 1 joue depuis la case 0 (cette action devrait entraîner une capture)
    printf("Joueur 1 joue le coup depuis la case 0\n");
    jouer_coup(&plateau, 1, 0);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 11
    printf("Joueur 2 joue le coup depuis la case 11\n");
    jouer_coup(&plateau, 2, 11);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 0 (cette action devrait entraîner une capture)
    printf("Joueur 1 joue le coup depuis la case 0\n");
    jouer_coup(&plateau, 1, 0);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 6
    printf("Joueur 2 joue le coup depuis la case 6\n");
    jouer_coup(&plateau, 2, 6);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 4 (cette action devrait entraîner une capture)
    printf("Joueur 1 joue le coup depuis la case 4\n");
    jouer_coup(&plateau, 1, 4);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 8
    printf("Joueur 2 joue le coup depuis la case 8\n");
    jouer_coup(&plateau, 2, 8);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 2 
    printf("Joueur 1 joue le coup depuis la case 2\n");
    jouer_coup(&plateau, 1, 2);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 10
    printf("Joueur 2 joue le coup depuis la case 10\n");
    jouer_coup(&plateau, 2, 10);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 2 
    printf("Joueur 1 joue le coup depuis la case 2\n");
    jouer_coup(&plateau, 1, 2);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 9
    printf("Joueur 2 joue le coup depuis la case 9\n");
    jouer_coup(&plateau, 2, 9);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 2 
    printf("Joueur 1 joue le coup depuis la case 2\n");
    jouer_coup(&plateau, 1, 2);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 6
    printf("Joueur 2 joue le coup depuis la case 6\n");
    jouer_coup(&plateau, 2, 6);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 10
    printf("Joueur 2 joue le coup depuis la case 10\n");
    jouer_coup(&plateau, 2, 10);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 7
    printf("Joueur 2 joue le coup depuis la case 7\n");
    jouer_coup(&plateau, 2, 7);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 8
    printf("Joueur 2 joue le coup depuis la case 8\n");
    jouer_coup(&plateau, 2, 8);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 9
    printf("Joueur 2 joue le coup depuis la case 9\n");
    jouer_coup(&plateau, 2, 9);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 10
    printf("Joueur 2 joue le coup depuis la case 10\n");
    jouer_coup(&plateau, 2, 10);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 11
    printf("Joueur 2 joue le coup depuis la case 11\n");
    jouer_coup(&plateau, 2, 11);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 2 
    printf("Joueur 1 joue le coup depuis la case 2\n");
    jouer_coup(&plateau, 1, 2);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 3 
    printf("Joueur 1 joue le coup depuis la case 3\n");
    jouer_coup(&plateau, 1, 3);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 4 
    printf("Joueur 1 joue le coup depuis la case 4\n");
    jouer_coup(&plateau, 1, 4);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 1
    printf("Joueur 1 joue le coup depuis la case 1\n");
    jouer_coup(&plateau, 1, 1);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 2 joue depuis la case 11
    printf("Joueur 2 joue le coup depuis la case 11\n");
    jouer_coup(&plateau, 2, 11);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 0
    printf("Joueur 1 joue le coup depuis la case 0\n");
    jouer_coup(&plateau, 1, 0);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 5
    printf("Joueur 1 joue le coup depuis la case 5\n");
    jouer_coup(&plateau, 1, 5);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 1
    printf("Joueur 1 joue le coup depuis la case 1\n");
    jouer_coup(&plateau, 1, 1);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 0
    printf("Joueur 1 joue le coup depuis la case 0\n");
    jouer_coup(&plateau, 1, 0);
    afficher_plateau(&plateau); // Affiche le plateau après le coup

    // Joueur 1 joue depuis la case 1
    printf("Joueur 1 joue le coup depuis la case 1\n");
    jouer_coup(&plateau, 1, 1);
    afficher_plateau(&plateau); // Affiche le plateau après le coup


    // Joueur 1 joue depuis la case 2 
    printf("Joueur 1 joue le coup depuis la case 2\n");
    jouer_coup(&plateau, 1, 2);
    afficher_plateau(&plateau); // Affiche le plateau après le coup




    // Vérification des scores
    if(est_fin_de_partie(&plateau)) {
        printf("La partie est terminée\n");
        printf("Score final - Joueur 1: %d, Joueur 2: %d\n", plateau.score_joueur1, plateau.score_joueur2);
        return 0;
    } else {
        printf("La partie n'est pas terminée\n");
    }
    

    
}
