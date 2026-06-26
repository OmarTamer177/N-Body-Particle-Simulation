all:
	mingw32-make -C sequential
	mingw32-make -C openmp
	mingw32-make -C mpi

clean:
	mingw32-make -C sequential clean
	mingw32-make -C openmp clean
	mingw32-make -C mpi clean

