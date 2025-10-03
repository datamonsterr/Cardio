#include <string.h>
#include "card.h"

void card_init(Card *aCardPtr, int suit, int rank)
{
    aCardPtr->suit = suit;
    aCardPtr->rank = rank;
}

char *card_toString(Card *aCardPtr)
{
    char *str = (char *)malloc(sizeof(char) * 20);
    char *tmp = (char *)malloc(sizeof(char) * 5);
    if (aCardPtr == NULL)
    {
        free(str);
        free(tmp);
        return NULL;
    }
    if (aCardPtr->rank >= 14 || aCardPtr->rank <= 0)
    {
        free(str);
        free(tmp);
        return NULL;
    }
    switch (aCardPtr->rank)
    {
    case 1:
        strcpy(str, "A");
        break;
    case 11:
        strcpy(str, "J");
        break;
    case 12:
        strcpy(str, "Q");
        break;
    case 13:
        strcpy(str, "K");
        break;
    default:
        sprintf(tmp, "%d", aCardPtr->rank);
        strcpy(str, tmp);
        break;
    }

    strcat(str, " of ");

    switch (aCardPtr->suit)
    {
    case 1:
        strcat(str, "Spades");
        break;
    case 2:
        strcat(str, "Hearts");
        break;
    case 3:
        strcat(str, "Diamonds");
        break;
    case 4:
        strcat(str, "Clubs");
        break;
    default:
        free(str);
        free(tmp);
        fprintf(stderr, "Invalid card with suit %d, rank %d. Suit is in range(1,4), ranks is in range(1,13)", aCardPtr->suit, aCardPtr->rank);
        return NULL;
    }

    free(tmp);

    return str;
}

int card_compare(const void *a, const void *b)
{
    return ((*(Card **)(a))->rank) - ((*(Card **)(b))->rank);
}
void card_destroy(Card *aCardPtr)
{
    free(aCardPtr);
}
