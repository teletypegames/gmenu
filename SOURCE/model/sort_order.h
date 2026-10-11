#ifndef GMENU_MODEL_SORT_ORDER_H
#define GMENU_MODEL_SORT_ORDER_H

/* The orders of the game list (F9). */
enum SortOrder { SortTitle, SortYear, SortGenre, SortPlays, SortOrderCount };

const char *sortOrderName( int order );

#endif
