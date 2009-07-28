#!/bin/sh

if [ \'`uname`\' = \'FreeBSD\' ]
then cat Makefile.in | sed -e 's/__OS_BSD__//; s/__OS_OSX__/# /; s/__OS_LINUX__/# /;' > Makefile
fi

if [ \'`uname`\' = \'Darwin\' ]
then cat Makefile.in | sed -e 's/__OS_BSD__/# /; s/__OS_OSX__//; s/__OS_LINUX__/# /;' > Makefile
fi

if [ \'`uname`\' = \'Linux\' ]
then cat Makefile.in | sed -e 's/__OS_BSD__/# /; s/__OS_OSX__/# /; s/__OS_LINUX__//;' > Makefile
fi
