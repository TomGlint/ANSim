# ANSim
ANSim: A Fast and Versatile Asynchronous Network-On-Chip Simulator


If you use ANSim in your research, we would appreciate the following citation in any publications to which it has contributed:

T. Glint, J. Sah, M. Awasthi and J. Mekie, "ANSim: A Fast and Versatile Asynchronous Network-On-Chip Simulator," 2020 IEEE 38th International Conference on Computer Design (ICCD), Hartford, CT, USA, 2020, pp. 619-622, doi: 10.1109/ICCD50377.2020.00107.

## Setup

### To clean the code after major change involving configuration files

> make distclean

### To clean the code after minor change

> make clean

### To compile

> make

### To run the simulator

> booksim `<configuration file>` [(over ride configuration separated by space) eg wait_for_tail_credit=1]
  
or take a peek at run.sh for example simulation


  
