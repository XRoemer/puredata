#include "m_pd.h"

//#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

//#include "r_delay~.h"
//#include "r_sort~.h"
//#include "r_maximum~.h"
#include "r_average~.h"
#include "meanblock~.h"

void helloworld_setup(void) {
	//init_delay();
	//init_sort();
	init_average();
	init_meanblock();
	//init_maximum();
}