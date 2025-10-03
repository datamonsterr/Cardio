#include "pokergame.h"

int player_init(Player * aPlayer){
  if((aPlayer->hand = (Hand *)malloc(sizeof(Hand)))==NULL)
     return -1;
  aPlayer->name = (char*)malloc(sizeof(char)*20);
  aPlayer->money = INITIALMONEY;
  aPlayer->bet =0; //amount the player is currently betting
  aPlayer->fold=0;//0 if the player is in the game, 1 if folded
  hand_init(aPlayer->hand);
  return 0;
}

int player_toString(Player * aPlayer){
  if(aPlayer==NULL)
    return -1;
  printf("%s",aPlayer->name);
  return 0;
}

int player_reset_hand(Player * aPlayer){
  // Properly destroy the existing hand first
  if (aPlayer->hand != NULL)
  {
    if (aPlayer->hand->cards != NULL)
    {
      for (int i = 0; i < HAND_SIZE; i++)
      {
        if (aPlayer->hand->cards[i] != NULL)
          free(aPlayer->hand->cards[i]);
      }
      free(aPlayer->hand->cards);
    }
    if (aPlayer->hand->class != NULL)
      free(aPlayer->hand->class);
    free(aPlayer->hand);
  }
  if((aPlayer->hand = (Hand *)malloc(sizeof(Hand)))==NULL)
     return -1;
  hand_init(aPlayer->hand);
  return 0;
}
void player_destroy(Player * aPlayer){
  if (aPlayer == NULL)
    return;
  // Free the name field
  if (aPlayer->name != NULL)
    free(aPlayer->name);
  // Properly destroy the hand
  if (aPlayer->hand != NULL)
  {
    if (aPlayer->hand->cards != NULL)
    {
      for (int i = 0; i < HAND_SIZE; i++)
      {
        if (aPlayer->hand->cards[i] != NULL)
          free(aPlayer->hand->cards[i]);
      }
      free(aPlayer->hand->cards);
    }
    if (aPlayer->hand->class != NULL)
      free(aPlayer->hand->class);
    free(aPlayer->hand);
  }
  free(aPlayer);
}
