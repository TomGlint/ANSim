#dependencies
# gcc
# g++
# flex
# bison

make distclean
make -j8

./booksim example/8x8MeshAsync
