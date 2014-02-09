Pin assignments
===============

All pins are on GPIO2.

    BBB    GPIO   Function
    ---    ----   --------
    P8.8   2_3    Vertical
    P8.7   2_2    Horizontal
    P8.9   2_5    Video

Packages
========

Needs ubuntu/debian for the BBB. Required packages:

sudo apt-get install xvfb libxtst-dev xterm
sudo apt-get install libxcb-xtest0-dev make libc6-dev g++ gcc libext-dev
