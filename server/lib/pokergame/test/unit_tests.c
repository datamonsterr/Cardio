#include "pokergame.h"
#include "testing.h"
#include <string.h>

TEST(test_card_toString)
{
    Card card;
    card_init(&card, 1, 2);
    ASSERT(strcmp(card_toString(&card), "2 of Spades") == 0);
    card_init(&card, 2, 2);
    ASSERT(strcmp(card_toString(&card), "2 of Hearts") == 0);
    card_init(&card, 3, 2);
    ASSERT(strcmp(card_toString(&card), "2 of Diamonds") == 0);
    card_init(&card, 4, 13);
    ASSERT(strcmp(card_toString(&card), "K of Clubs") == 0);
    card_destroy(&card);
}

TEST(test_hand_toString)
{
    Deck mydeck;
    Hand myhand;
    Card *card1, *card2;
    hand_init(&myhand);
    deck_init(&mydeck);
    deck_fill(&mydeck);
    shuffle(&mydeck, 123);
    dequeue_card(&mydeck, &card1);
    dequeue_card(&mydeck, &card2);
    add_card(&myhand, card1);
    int res = hand_toString(&myhand);
    ASSERT(res == 0);
}

int main()
{
    RUN_TEST(test_hand_toString);
    return failed;
}