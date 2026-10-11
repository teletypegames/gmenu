/*
   The values of the pickers: the genres, years (with their decades) and player counts
   the games have, and the recently played games.
*/

#ifndef GMENU_SERVICE_PICKER_SERVICE_H
#define GMENU_SERVICE_PICKER_SERVICE_H

class GameLibrary;
struct PickList;
struct RecentList;

class PickerService {
public:
  enum { MaxPlayerCount = 16 };

  explicit PickerService( const GameLibrary &library );
  /* "(all)" and the values of the field, sorted: the index of current (0 when empty). */
  int filterValues( int field, const char *current, PickList &picks );
  /* The titles of the recently played games that are still there: their count. */
  int recentGames( const RecentList &recent, PickList &picks );

private:
  void addPlayerCounts( PickList &picks );
  void addDecades( PickList &picks );

  const GameLibrary &library_;
};

#endif
