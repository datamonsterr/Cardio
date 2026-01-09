#pragma once
#include <stdio.h>
#include <stdlib.h>

#define CLASS                                                                                                          \
    8 /*number of classes: pair, two pair, three of a kind, straight, flush, full house, four of a kind, straight      \
         flush*/
#define RANKS 13     /*number of different cards (2 thru A)*/
#define DECK_SIZE 52 /*number of consecutive in-memory pointers to pointers of Card structs in a Deck struct*/
#define FIRSTCARD 0  /*index of the first card in a Deck struct*/
#define SUIT_SPADE 1
#define SUIT_HEART 2
#define SUIT_DIAMOND 3
#define SUIT_CLUB 4

/*defines a card. A card has one of four suits and 13 ranks (2-A)*/
typedef struct card
{
    int suit;
    int rank;
} Card;

typedef struct deck
{
    Card** cards; /*DECK_SIZE number of pointers to pointed to by Card **cards. Consecutive in-memory, so we can iterate
                     over them using array notation or pointer arithmetic.*/
    Card* topcard; /*card at the top of the deck*/
    int topcardindex;
} Deck;

// function prototypes for the Card struct
char* card_toString(Card* aCardPtr);
void card_init(Card* aCardPtr, int suit, int rank);
void card_destroy(Card* aCardPtr);
int card_compare(const void* a, const void* b);

void deck_toString(Deck* aDeckPtr);
int dequeue_card(Deck* aDeckPtr, Card** data);
int swap_card(Deck* aDeckPtr, int s, int t);
int deck_init(Deck* aDeckPtr);
void deck_fill(Deck* aDeckPtr);
void shuffle(Deck* aDeckPtr, int shuffles);
void deck_destroy(Deck* aDeckPtr);
void enqueue_deck(Deck* aDeckPtr);
int remove_card(Deck* aDeckPtr, Card* card);
int find_card(Deck* aDeckPtr, int suit, int rank);