#include "card.h"
#include "testing.h"
#include <string.h>

// Test card_init function
TEST(test_card_init_basic)
{
    Card card;
    card_init(&card, SUIT_SPADE, 5);
    ASSERT(card.suit == SUIT_SPADE);
    ASSERT(card.rank == 5);
}

TEST(test_card_init_different_suits)
{
    Card card1, card2, card3, card4;
    card_init(&card1, SUIT_SPADE, 2);
    card_init(&card2, SUIT_HEART, 3);
    card_init(&card3, SUIT_DIAMOND, 4);
    card_init(&card4, SUIT_CLUB, 5);

    ASSERT(card1.suit == SUIT_SPADE);
    ASSERT(card2.suit == SUIT_HEART);
    ASSERT(card3.suit == SUIT_DIAMOND);
    ASSERT(card4.suit == SUIT_CLUB);
}

TEST(test_card_init_face_cards)
{
    Card jack, queen, king, ace;
    card_init(&jack, SUIT_HEART, 11);
    card_init(&queen, SUIT_DIAMOND, 12);
    card_init(&king, SUIT_CLUB, 13);
    card_init(&ace, SUIT_SPADE, 1);

    ASSERT(jack.rank == 11);
    ASSERT(queen.rank == 12);
    ASSERT(king.rank == 13);
    ASSERT(ace.rank == 1);
}

// Test card_toString function
TEST(test_card_toString_number_cards)
{
    Card card;
    card_init(&card, SUIT_SPADE, 2);
    char* str = card_toString(&card);
    ASSERT_STR_EQ(str, "2 of Spades");
    free(str);

    card_init(&card, SUIT_HEART, 7);
    str = card_toString(&card);
    ASSERT_STR_EQ(str, "7 of Hearts");
    free(str);
}

TEST(test_card_toString_face_cards)
{
    Card card;
    card_init(&card, SUIT_DIAMOND, 11);
    char* str = card_toString(&card);
    ASSERT_STR_EQ(str, "J of Diamonds");
    free(str);

    card_init(&card, SUIT_CLUB, 12);
    str = card_toString(&card);
    ASSERT_STR_EQ(str, "Q of Clubs");
    free(str);

    card_init(&card, SUIT_SPADE, 13);
    str = card_toString(&card);
    ASSERT_STR_EQ(str, "K of Spades");
    free(str);

    card_init(&card, SUIT_HEART, 1);
    str = card_toString(&card);
    ASSERT_STR_EQ(str, "A of Hearts");
    free(str);
}

TEST(test_card_toString_all_suits)
{
    Card card;
    card_init(&card, SUIT_SPADE, 5);
    char* str = card_toString(&card);
    ASSERT_STR_EQ(str, "5 of Spades");
    free(str);

    card_init(&card, SUIT_HEART, 5);
    str = card_toString(&card);
    ASSERT_STR_EQ(str, "5 of Hearts");
    free(str);

    card_init(&card, SUIT_DIAMOND, 5);
    str = card_toString(&card);
    ASSERT_STR_EQ(str, "5 of Diamonds");
    free(str);

    card_init(&card, SUIT_CLUB, 5);
    str = card_toString(&card);
    ASSERT_STR_EQ(str, "5 of Clubs");
    free(str);
}

// Test deck_init function
TEST(test_deck_init)
{
    Deck deck;
    int result = deck_init(&deck);
    ASSERT(result == 0);
    ASSERT(deck.topcardindex == FIRSTCARD);
    ASSERT(deck.cards != NULL);

    // Cleanup
    for (int i = 0; i < DECK_SIZE; i++)
    {
        free(deck.cards[i]);
    }
    free(deck.cards);
}

TEST(test_deck_init_allocates_52_cards)
{
    Deck deck;
    deck_init(&deck);

    // Verify all 52 card pointers are allocated
    for (int i = 0; i < DECK_SIZE; i++)
    {
        ASSERT(deck.cards[i] != NULL);
    }

    // Cleanup
    for (int i = 0; i < DECK_SIZE; i++)
    {
        free(deck.cards[i]);
    }
    free(deck.cards);
}

TEST(test_deck_init_sets_topcard_to_zero)
{
    Deck deck;
    deck_init(&deck);
    ASSERT(deck.topcardindex == 0);

    // Cleanup
    for (int i = 0; i < DECK_SIZE; i++)
    {
        free(deck.cards[i]);
    }
    free(deck.cards);
}

// Test deck_fill function
TEST(test_deck_fill_creates_52_cards)
{
    Deck deck;
    deck_init(&deck);
    deck_fill(&deck);

    // Check that we have 52 unique cards
    int count = 0;
    for (int i = 0; i < DECK_SIZE; i++)
    {
        if (deck.cards[i]->rank >= 2 && deck.cards[i]->rank <= 14)
        {
            count++;
        }
    }
    ASSERT(count == 52);

    // Cleanup
    for (int i = 0; i < DECK_SIZE; i++)
    {
        free(deck.cards[i]);
    }
    free(deck.cards);
}

TEST(test_deck_fill_has_all_suits)
{
    Deck deck;
    deck_init(&deck);
    deck_fill(&deck);

    // Count cards per suit
    int spades = 0, hearts = 0, diamonds = 0, clubs = 0;
    for (int i = 0; i < DECK_SIZE; i++)
    {
        switch (deck.cards[i]->suit)
        {
        case SUIT_SPADE:
            spades++;
            break;
        case SUIT_HEART:
            hearts++;
            break;
        case SUIT_DIAMOND:
            diamonds++;
            break;
        case SUIT_CLUB:
            clubs++;
            break;
        }
    }

    ASSERT(spades == 13);
    ASSERT(hearts == 13);
    ASSERT(diamonds == 13);
    ASSERT(clubs == 13);

    // Cleanup
    for (int i = 0; i < DECK_SIZE; i++)
    {
        free(deck.cards[i]);
    }
    free(deck.cards);
}

TEST(test_deck_fill_has_all_ranks)
{
    Deck deck;
    deck_init(&deck);
    deck_fill(&deck);

    // Count each rank (2-14, where 14 is Ace)
    int rank_counts[15] = {0};
    for (int i = 0; i < DECK_SIZE; i++)
    {
        rank_counts[deck.cards[i]->rank]++;
    }

    // Each rank 2-14 should appear 4 times (once per suit)
    for (int rank = 2; rank <= 14; rank++)
    {
        ASSERT(rank_counts[rank] == 4);
    }

    // Cleanup
    for (int i = 0; i < DECK_SIZE; i++)
    {
        free(deck.cards[i]);
    }
    free(deck.cards);
}

// Test dequeue_card function
TEST(test_dequeue_card_removes_top_card)
{
    Deck deck;
    deck_init(&deck);
    deck_fill(&deck);

    Card* card;
    int result = dequeue_card(&deck, &card);

    ASSERT(result == 0);
    ASSERT(card != NULL);
    ASSERT(deck.topcardindex == 1);

    // Cleanup
    for (int i = 0; i < DECK_SIZE; i++)
    {
        free(deck.cards[i]);
    }
    free(deck.cards);
}

TEST(test_dequeue_card_sequence)
{
    Deck deck;
    deck_init(&deck);
    deck_fill(&deck);

    Card *card1, *card2, *card3;
    dequeue_card(&deck, &card1);
    dequeue_card(&deck, &card2);
    dequeue_card(&deck, &card3);

    ASSERT(deck.topcardindex == 3);
    ASSERT(card1 != card2);
    ASSERT(card2 != card3);

    // Cleanup
    for (int i = 0; i < DECK_SIZE; i++)
    {
        free(deck.cards[i]);
    }
    free(deck.cards);
}

TEST(test_dequeue_card_empty_deck)
{
    Deck deck;
    deck_init(&deck);
    deck_fill(&deck);

    // Dequeue all 52 cards
    Card* card;
    for (int i = 0; i < 52; i++)
    {
        dequeue_card(&deck, &card);
    }

    // Try to dequeue from empty deck
    int result = dequeue_card(&deck, &card);
    ASSERT(result == -1);

    // Cleanup
    for (int i = 0; i < DECK_SIZE; i++)
    {
        free(deck.cards[i]);
    }
    free(deck.cards);
}

// Test shuffle function
TEST(test_shuffle_changes_order)
{
    Deck deck;
    deck_init(&deck);
    deck_fill(&deck);

    // Store original order
    int original_ranks[5];
    for (int i = 0; i < 5; i++)
    {
        original_ranks[i] = deck.cards[i]->rank;
    }

    shuffle(&deck, 100);

    // Check that at least some cards changed position
    int changes = 0;
    for (int i = 0; i < 5; i++)
    {
        if (deck.cards[i]->rank != original_ranks[i])
        {
            changes++;
        }
    }

    ASSERT(changes > 0);

    // Cleanup
    for (int i = 0; i < DECK_SIZE; i++)
    {
        free(deck.cards[i]);
    }
    free(deck.cards);
}

TEST(test_shuffle_maintains_card_count)
{
    Deck deck;
    deck_init(&deck);
    deck_fill(&deck);

    shuffle(&deck, 100);

    // Verify still have 52 cards
    int count = 0;
    for (int i = 0; i < DECK_SIZE; i++)
    {
        if (deck.cards[i] != NULL)
        {
            count++;
        }
    }

    ASSERT(count == 52);

    // Cleanup
    for (int i = 0; i < DECK_SIZE; i++)
    {
        free(deck.cards[i]);
    }
    free(deck.cards);
}

TEST(test_shuffle_preserves_all_cards)
{
    Deck deck;
    deck_init(&deck);
    deck_fill(&deck);

    shuffle(&deck, 100);

    // Count cards per suit after shuffle
    int spades = 0, hearts = 0, diamonds = 0, clubs = 0;
    for (int i = 0; i < DECK_SIZE; i++)
    {
        switch (deck.cards[i]->suit)
        {
        case SUIT_SPADE:
            spades++;
            break;
        case SUIT_HEART:
            hearts++;
            break;
        case SUIT_DIAMOND:
            diamonds++;
            break;
        case SUIT_CLUB:
            clubs++;
            break;
        }
    }

    ASSERT(spades == 13);
    ASSERT(hearts == 13);
    ASSERT(diamonds == 13);
    ASSERT(clubs == 13);

    // Cleanup
    for (int i = 0; i < DECK_SIZE; i++)
    {
        free(deck.cards[i]);
    }
    free(deck.cards);
}

int main()
{
    printf("Running Card Library Unit Tests\n");
    printf("================================\n");

    // Card init tests
    RUN_TEST(test_card_init_basic);
    RUN_TEST(test_card_init_different_suits);
    RUN_TEST(test_card_init_face_cards);

    // Card toString tests
    RUN_TEST(test_card_toString_number_cards);
    RUN_TEST(test_card_toString_face_cards);
    RUN_TEST(test_card_toString_all_suits);

    // Deck init tests
    RUN_TEST(test_deck_init);
    RUN_TEST(test_deck_init_allocates_52_cards);
    RUN_TEST(test_deck_init_sets_topcard_to_zero);

    // Deck fill tests
    RUN_TEST(test_deck_fill_creates_52_cards);
    RUN_TEST(test_deck_fill_has_all_suits);
    RUN_TEST(test_deck_fill_has_all_ranks);

    // Dequeue card tests
    RUN_TEST(test_dequeue_card_removes_top_card);
    RUN_TEST(test_dequeue_card_sequence);
    RUN_TEST(test_dequeue_card_empty_deck);

    // Shuffle tests
    RUN_TEST(test_shuffle_changes_order);
    RUN_TEST(test_shuffle_maintains_card_count);
    RUN_TEST(test_shuffle_preserves_all_cards);

    printf("\n================================\n");
    if (failed)
    {
        printf("Some tests FAILED\n");
        return 1;
    }
    else
    {
        printf("All tests PASSED\n");
        return 0;
    }
}
