all:
	(cd Salmi; make)
	(cd XV6FS; make)
	(cd Fs_skel; rsync -a . ../Build)
	(cd Library/libs; make)
	(cd Cmds; make)
	(cd Tools; make)
	Tools/mkfs fs.img Build

clean:
	(cd Salmi; make clean)
	(cd XV6FS; make clean)
	(cd Library/libs; make clean)
	(cd Cmds; make clean)
	(cd Tools; make clean)
	rm -rf Build/*
	rm -f fs.img
