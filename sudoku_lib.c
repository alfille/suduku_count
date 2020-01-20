// Sudoku_count
// Attempt to count the number of legal sudoku positions
// Based on Discussion between Kendrick Shaw MD PhD and Paul Alfille MD
// MIT license 2019
// by Paul H Alfille

#include "sudoku_count.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// SIZE x SIZE sudoku board
#ifndef SUBSIZE
#define SUBSIZE 3
#endif
#define SIZE (SUBSIZE*SUBSIZE)
# define TOTALSIZE (SIZE*SIZE)

#define Zero(array) memset( array, 0, sizeof(array) ) ;

#define VAL2FREE( v ) ( -((v)+1) )
#define FREE2VAL( f ) ( -((f)+1) )

// Special
int Xpattern ;
int Wpattern ;



// bit pattern
int pattern[SIZE] ;
int full_pattern ;

// Make numbers int bit pattern (1=0x1 2=0x2 3=0x4...)
void make_pattern(void) {
    int i ;
    full_pattern = 0 ;
    for (i=0;i<SIZE;++i) {
        pattern[i] = 1<<i ;
        full_pattern |= pattern[i] ;
    }
}

// Find number from bit pattern (for printing)
int reverse_pattern( int pat ) {
    int i ;
    for (i=0 ; i<SIZE;++i) {
        if ( pattern[i] == pat ) {
            return i+1 ;
        }
    }
    // should never happen
    return 0 ;
}

// For backtracking state
#define MAXTRACK TOTALSIZE

struct FillState {
    int mask_bits[SIZE][SIZE] ;
    int free_state[SIZE][SIZE] ;
} State[MAXTRACK] ;

struct StateStack {
    int size ;
    int start ;
    int end ;
} statestack = { 0,0,0 };

struct FillState * StateStackInit ( void ) {
	int i,j ;
    statestack.size = TOTALSIZE ;
    statestack.start = statestack.end = 0 ;
	for ( i=0 ; i<SIZE ; ++i ) {
		for ( j=0 ; j<SIZE ; ++j ) {
			State[0].mask_bits[i][j] = 0 ;
			State[0].free_state[i][j] = SIZE ;
		}
	}
    return & State[0] ;
}


struct FillState * StateStackPush( void ) {
    if ( statestack.size == 0 ) {
        return & State[0] ;
    } else {
        int oldend = statestack.end ;
        statestack.end = (statestack.end+1) % statestack.size ;
        if ( statestack.end == statestack.start ) {
            // overfilled -- move the start
            statestack.start = (statestack.start+1) % statestack.size ;
        }
        memcpy( & State[statestack.end], & State[oldend], sizeof( struct FillState ) ) ;
        return & State[statestack.end] ;
    }
}

struct FillState * StateStackPop( void ) {
    if ( statestack.start == statestack.end ) {
        // empty
        return NULL ;
    } else {
        statestack.end = ( statestack.end + statestack.size - 1 ) % statestack.size ; // make sure positive modulo
        return & State[statestack.end] ;
    }
}

void print_square( struct FillState * pFS ) {
	int i ;
	int j ;
	
	// Initial blank
	fprintf(stderr,"\n");

	// top line
	for ( j=0 ; j<SIZE ; ++j ) {
		fprintf(stderr,"+---");
	}
	// end of top line
	fprintf(stderr,"+\n");
	
	for (i=0 ; i<SIZE ; ++i ) { // each row
		for ( j=0 ; j<SIZE ; ++j ) {
			int c = (j%SUBSIZE)?':':'|' ;
            int v = FREE2VAL( pFS->free_state[i][j] );
			fprintf(stderr,"%c%2X ",c,v>=0?v:0xA);
		}
		// end of row
		fprintf(stderr,"|\n");
		
		// Separator line
		for ( j=0 ; j<SIZE ; ++j ) {
			int c = ((i+1)%SUBSIZE)?' ':'-';
			fprintf(stderr,"+%c-%c",c,c);
		}
		// end of separator
		fprintf(stderr,"+\n");
	} 

	// Final blank
	fprintf(stderr,"\n");
}   

// Enter with current position set in free_state
struct FillState * Set_Square( struct FillState * pFS, int testi, int testj ) {
	int si, sj, k ;
	int val = FREE2VAL( pFS->free_state[testi][testj] );
	int b ;
	//char debug ;
	//printf("Set %d %d bit=%X mask=%X\n",testi,testj,pFS->free_state[testi][testj],pFS->mask_bits[testi][testj]);
	//debug = getchar() ;

	if ( val >= 0 ) {
		// already set (preset from GUI)
		//fprintf(stderr,"Set: Position %d,%d already set\n",testi,testj) ;
		b = pattern[val] ;
		if ( (b&pFS->mask_bits[testi][testj]) ) {
			return NULL ;
		}
		// point mask
		pFS->mask_bits[testi][testj] |= b ;
	} else {
		// Not already set (Part of solve search)
		// find clear bit
		int mask = pFS->mask_bits[testi][testj] ;
		for ( val = 0 ; val < SIZE ; ++val ) {
			b = pattern[val] ;
			//printf("val=%d b=%X mask=%X b&mask=%X\n",val,b,mask,b&mask ) ;
			if ( (b & mask) == 0 ) {
				//printf("Try val %d at %d,%d\n",val,testi,testj);
				break ;
			}
		}
		// should never fall though

		// point
		pFS->mask_bits[testi][testj] |= b ;
		--pFS->free_state[testi][testj] ;
		
		// Push state (with current choice masked out) if more than one choice
		if ( pFS->free_state[testi][testj] > 0 ) {
			printf("Push");
			pFS = StateStackPush() ;
		}
		// Now set this choice
		pFS->free_state[testi][testj] = VAL2FREE(val) ;
	
	}


	// mask out row everywhere else
	for( k=0 ; k<SIZE ; ++k ) {
		// row
		if ( pFS->free_state[testi][k] < 0 ) {
			// assigned
			continue ;
		}
		if ( pFS->mask_bits[testi][k] & b ) {
			// already masked
			continue ;
		}
		if ( pFS->free_state[testi][k] == 1 ) {
			// nothing left
			return NULL ;
		}
		pFS->mask_bits[testi][k] |= b ;
		--pFS->free_state[testi][k] ;
	}

	// mask out column everywhere else
	for( k=0 ; k<SIZE ; ++k ) {
		// row
		if ( pFS->free_state[k][testj] < 0 ) {
			// assigned
			continue ;
		}
		if ( pFS->mask_bits[k][testj] & b ) {
			// already masked
			continue ;
		}
		if ( pFS->free_state[k][testj] == 1 ) {
			// nothing left
			return NULL ;
		}
		pFS->mask_bits[k][testj] |= b ;
		--pFS->free_state[k][testj] ;
	}

	// subsquare
	for( si=0 ; si<SUBSIZE ; ++si ) {
		for( sj=0 ; sj<SUBSIZE ; ++sj ) {
			int i = SUBSIZE * (testi/SUBSIZE) + si ;
			int j = SUBSIZE * (testj/SUBSIZE) + sj ;
			// row
			if ( pFS->free_state[i][j] < 0 ) {
				// assigned
				continue ;
			}
			if ( pFS->mask_bits[i][j] & b ) {
				// already masked
				continue ;
			}
			if ( pFS->free_state[i][j] == 1 ) {
				// nothing left
				return NULL ;
			}
			pFS->mask_bits[i][j] |= b ;
			--pFS->free_state[i][j] ;
		}
	}
	
	// Xpattern
	if ( Xpattern ) {
		if ( testi==testj ) {
			for ( k=0 ; k<SIZE ; ++k ) {
				if ( pFS->free_state[k][k] < 0 ) {
					// assigned
					continue ;
				}
				if ( pFS->mask_bits[k][k] & b ) {
					// already masked
					continue ;
				}
				if ( pFS->free_state[k][k] == 1 ) {
					// nothing left
					return NULL ;
				}
				pFS->mask_bits[k][k] |= b ;
				--pFS->free_state[k][k] ;
			}
		} else if ( testi==SIZE-1-testj ) {
			for ( k=0 ; k<SIZE ; ++k ) {
				int l = SIZE - 1 - k ;
				if ( pFS->free_state[k][l] < 0 ) {
					// assigned
					continue ;
				}
				if ( pFS->mask_bits[k][l] & b ) {
					// already masked
					continue ;
				}
				if ( pFS->free_state[k][l] == 1 ) {
					// nothing left
					return NULL ;
				}
				pFS->mask_bits[k][l] |= b ;
				--pFS->free_state[k][l] ;
			}
		}
	}

	// Window Pane
	if ( Wpattern ) {
		if ( (testi%(SUBSIZE+1))!=0 && (testj%(SUBSIZE+1))!=0 ) {
			for( si=0 ; si<SUBSIZE ; ++si ) {
				int i = (testi / (SUBSIZE+1)) * (SUBSIZE+1) + 1 + si ;
				for( sj=0 ; sj<SUBSIZE ; ++sj ) {
					int j = (testj / (SUBSIZE+1)) * (SUBSIZE+1) + 1 + sj ;
					// row
					if ( pFS->free_state[i][j] < 0 ) {
						// assigned
						continue ;
					}
					if ( pFS->mask_bits[i][j] & b ) {
						// already masked
						continue ;
					}
					if ( pFS->free_state[i][j] == 1 ) {
						// nothing left
						return NULL ;
					}
					pFS->mask_bits[i][j] |= b ;
					--pFS->free_state[i][j] ;
				}
			}
		}
	}
	
	return pFS ;
}
		


struct FillState * Next_move( struct FillState * pFS, int * done ) {
	int i ;
	int j ;
	int minfree = SIZE + 1 ;
	int fi, fj ;
	
	done[0] = 0 ; //assume not done
	
    //print_square( pFS ) ;
    for ( i=0 ; i < SIZE ; ++i ) {
		for ( j=0 ; j< SIZE ; ++j ) {
			int free_state = pFS->free_state[i][j] ;
			if ( free_state < 0 ) {
				// already set
				//printf("Slot %d,%d already set\n",i,j);
				continue ;		
			}
			if ( free_state == 0 ) {
				printf("0");
				//printf("Slot %d,%d exhausted\n",i,j);
				return NULL ;
			}
			if ( free_state == 1 ) {
				printf("1");
				//printf("Slot %d,%d exactly specified\n",i,j);
				return Set_Square( pFS, i, j ) ;
			}
			//printf("Minfree %d free_state %d\n",minfree,free_state);
			if ( free_state < minfree ) {
				minfree = free_state ;
				fi = i ;
				fj = j ;
			}
		}
	}
	if ( minfree == SIZE+1 ) {
		// only set squares -- Victory
		// only set squares -- Victory
		done[0] = 1 ; // all done
		return pFS ;
	}
	
	// Try smallest choice (most constrained)
	//printf("Slot %d,%d will be probed (free %d)\n",fi,fj,minfree);
	printf("%d",minfree);
	return Set_Square( pFS, fi, fj ) ;
}

struct FillState * SolveLoop( struct FillState * pFS ) {
	int done = 0 ;
	
	while ( done == 0 ) {
		//printf("Solveloop\n");
		pFS = Next_move( pFS , &done ) ;
		if ( pFS == NULL ) {
			printf("Pop");
			pFS = StateStackPop() ;
		}
		if ( pFS == NULL ) {
			return NULL ;
		}
	}
	//print_square( pFS ) ;
	return pFS ;
}



struct FillState * Setup_board( int * preset ) {
	// preset is an array of TOTALSIZE length
	// sent from python
	// only values on (0 -- SIZE-1) accepted
	// rest ignored
	// so use -1 for enpty cells
	
	int i, j ;
	int * set = preset ; // pointer though preset array
	struct FillState * pFS = StateStackInit( ) ;
	
	//printf("LIB\n") ;
	make_pattern() ; // set up bit pattern list
	
	for ( i=0 ; i<SIZE ; ++i ) {
		for ( j=0 ; j<SIZE ; ++j ) {
			if ( set[0] > -1 && set[0] < SIZE ) {
				pFS->free_state[i][j] = VAL2FREE( set[0] ) ;
				//print_square( pFS ) ;
				//printf("LIB: %d -> %d -> %d\n",set[0],VAL2FREE(set[0]),FREE2VAL(VAL2FREE(set[0])));
				pFS = Set_Square( pFS, i, j ) ;
				if ( pFS == NULL ) {
					// bad input 
					return NULL ;
				}
			}
			++set ; // move to next entry
		}
	}
	return pFS ;
}

int Return_board( int * preset, struct FillState * pFS ) {
	int i, j ;
	int * set = preset ; // pointer though preset array
	
	if ( pFS ) {
		// solved
		printf(" :)\n");
		for ( i=0 ; i<SIZE ; ++i ) {
			for ( j=0 ; j<SIZE ; ++j ) {
				set[0] = FREE2VAL( pFS->free_state[i][j] ) ;
				++set ; // move to next entry
			}
		}
		return 1 ;
	} else {
		printf(" :(\n");
		// unsolvable
		return 0 ;
	}
}
	
// return 1=solved, 0 not. data in same array
int Solve( int X, int Window, int * preset ) {
	struct FillState * pFS = Setup_board( preset ) ;
	
	Xpattern = X ;
	Wpattern = Window ;
	//printf("X=%d, W=%d\n",Xpattern,Wpattern) ;
	
	if ( pFS ) {
		return Return_board( preset, SolveLoop( pFS ) ) ;
	} else {
		// bad input (inconsistent soduku)
		return Return_board( preset, NULL ) ;
	}
}
	
