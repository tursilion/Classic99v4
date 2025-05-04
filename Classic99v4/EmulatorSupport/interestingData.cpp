// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#include <raylib.h>
#include <stdio.h>
#include "interestingData.h"

// This could have been a class, but there's no need for it to be
// No locking - one writer and many readers for each value

int InterestingData[MAX_INTERESTING_DATA];

void initInterestingData() {
    for (int i = 0; i < MAX_INTERESTING_DATA; ++i) {
        InterestingData[i] = DATA_UNSET;
    }
}
