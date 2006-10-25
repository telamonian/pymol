
include Rules.make

all: unix contrib

PRIME = ls *.c | sed 's/.c$$/.o/'| awk 'BEGIN{printf("OBJS=")}{printf("%s ",$$1)}END{print}'>.files;ls *.c | sed 's/.c$$/.p/'| awk 'BEGIN{printf("DEPS=")}{printf("%s ",$$1)}END{print}'>>.files; touch .depends; cat .files .depends > .includes

MINDEP=$(PYMOL_PATH)/products/unix-mindep
MDP=$(MINDEP)/pymol

.includes:
	cd ov/src;$(PRIME)
	cd layer0;$(PRIME)
	cd layer1;$(PRIME)
	cd layer2;$(PRIME)
	cd layer3;$(PRIME)
	cd layer4;$(PRIME)
	cd layer5;$(PRIME)
	touch .includes

.update:
	cd ov/src;$(MAKE)
	cd layer0;$(MAKE)
	cd layer1;$(MAKE)
	cd layer2;$(MAKE)
	cd layer3;$(MAKE)
	cd layer4;$(MAKE)
	cd layer5;$(MAKE)
	touch .update

ov: 
	cd ov/src;$(MAKE)

0:
	cd layer0;$(MAKE)

1:
	cd layer1;$(MAKE)

2:
	cd layer2;$(MAKE)

3:
	cd layer3;$(MAKE)

4:
	cd layer4;$(MAKE)

5:
	cd layer5;$(MAKE)

.depends: 
	/bin/rm -f .includes
	cd ov/src;$(MAKE) depends
	cd layer0;$(MAKE) depends
	cd layer1;$(MAKE) depends
	cd layer2;$(MAKE) depends
	cd layer3;$(MAKE) depends
	cd layer4;$(MAKE) depends
	cd layer5;$(MAKE) depends

.contrib:
	cd contrib;$(MAKE)
	touch .contrib

contrib: .contrib

lib:  .includes .depends .update 
	/bin/rm -f .update .includes
	ar crv libPyMOL.a */*.o ov/src/*.o 
	ranlib libPyMOL.a

unix: .includes .depends .update 
	/bin/rm -f .update .includes
	$(CC) $(BUILD) $(DEST) */*.o ov/src/*.o contrib/uiuc/plugins/molfile_plugin/src/*.o $(CFLAGS)  $(LIB_DIRS) $(LIBS)	

semistatic: .includes .depends .update
	/bin/rm -f .update .includes
	cd contrib;$(MAKE) static
	cd contrib/uiuc/plugins/molfile_plugin/src;$(MAKE)
	$(CC) $(BUILD) $(DEST) */*.o ov/src/*.o contrib/uiuc/plugins/molfile_plugin/src/*.o $(CFLAGS) $(LIB_DIRS) $(LIBS)	
	$(PYTHON_EXE) modules/compile_pymol.py

# need to be root to do this...

unix-mindep: semistatic
	/bin/rm -rf $(MINDEP)
	install -d $(MDP)/ext/lib
	cp -r modules $(MDP)
	cp -r test $(MDP)
	cp -r data $(MDP)	
	cp -r examples $(MDP)
	/bin/rm -rf $(MDP)/examples/package
	cp -r scripts $(MDP)
	cp -r pymol.exe $(MDP)
	cp -r ext/lib/python2.4 $(MDP)/ext/lib
	cp -r ext/lib/tcl8.4 $(MDP)/ext/lib
	cp -r ext/lib/tk8.4 $(MDP)/ext/lib
	/bin/rm -f $(MDP)/ext/lib/python2.4/config/libpython2.4.a
	/bin/rm -rf $(MDP)/ext/lib/python2.4/test
	cp LICENSE $(MDP)
	cp README $(MDP)
	cp setup/INSTALL.unix-mindep $(MDP)/INSTALL
	cp setup/setup.sh.unix-mindep $(MDP)/setup.sh

unix-beta: unix-mindep
	cp epymol/data/pymol/beta/splash.png $(MDP)/data/pymol/splash.png
	cd $(MINDEP);chown -R nobody pymol
	cd $(MINDEP);chgrp -R nobody pymol
	cd $(MINDEP);tar -cvf - pymol | gzip > ../pymol-0_xx-bin-xxxxx-mindep.tgz

unix-product: unix-mindep
	cp epymol/data/pymol/splash.png $(MDP)/data/pymol/splash.png
	cp epymol/LICENSE.txt $(MDP)/LICENSE
	cp -r epymol/modules/epymol $(MDP)/modules/
	/bin/rm $(MDP)/modules/epymol/*.py $(MDP)/modules/epymol/*/*.py
	find $(MINDEP)/pymol -type d -name CVS | awk '{printf "/bin/rm -rf ";print;}' | sh
	find $(MINDEP)/pymol -type d -name '\.svn' | awk '{printf "/bin/rm -rf ";print;}' | sh
	/bin/rm -f $(MINDEP)/pymol/test/pdb
	cd $(MINDEP);chown -R nobody pymol
	cd $(MINDEP);chgrp -R nobody pymol
	cd $(MINDEP);tar -cvf - pymol | gzip > ../pymol-0_xx-bin-xxxxx-mindep.tgz

mac-product: unix-mindep
	cp setup/setup.sh.macosx-x11 $(MDP)/setup.sh
	cp epymol/data/pymol/splash.png $(MDP)/data/pymol/splash.png
	cp epymol/LICENSE.txt $(MDP)/LICENSE
	cd $(MINDEP);chown -R nobody pymol
	cd $(MINDEP);chgrp -R nobody pymol
	cd $(MINDEP);tar -cvf - pymol | gzip > ../pymol-0_xx-bin-xxxxx-mindep.tgz

unix-helper: unix-mindep-build
	cp setup/setup.sh.unix-helper $(MDP)/setup.sh
	cd $(MINDEP);tar -cvf - pymol | gzip > ../helperpymol-0_xx-bin-xxxxx-mindep.tgz

# Sharp3D display running under linux

unix-s3d: .includes .depends .update 
	cd sharp3d/src; $(MAKE)
	$(CC) $(BUILD) $(DEST) */*.o ov/src/*.o sharp3d/src/*.o $(CFLAGS) $(LIB_DIRS) $(LIBS) -Lsharp3d/lib -lsgl.1.3 -lspl.1.2 

unix-s3d-semistatic: .includes .depends .update
	/bin/rm -f .update .includes
	cd contrib;$(MAKE) static
	cd sharp3d/src; $(MAKE)
	$(CC) $(BUILD) $(DEST) */*.o ov/src/*.o sharp3d/src/*.o $(CFLAGS) $(LIB_DIRS) $(LIBS) -Lsharp3d/lib -lsgl.1.3 -lspl.1.2 

unix-s3d-build: unix-s3d-semistatic
	$(PYTHON_EXE) modules/compile_pymol.py
	/bin/rm -rf $(MINDEP)
	install -d $(MDP)/ext/lib
	cp -r modules $(MDP)
	cp -r test $(MDP)
	cp -r data $(MDP)	
	cp -r examples $(MDP)
	cp -r scripts $(MDP)
	cp -r pymol.exe $(MDP)
	cp -r ext/lib/python2.3 $(MDP)/ext/lib
	cp -r ext/lib/tcl8.4 $(MDP)/ext/lib
	cp -r ext/lib/tk8.4 $(MDP)/ext/lib
	cp sharp3d/lib/* $(MDP)/ext/lib/
	/bin/rm -f $(MDP)/ext/lib/python2.3/config/libpython2.3.a
	/bin/rm -rf $(MDP)/ext/lib/python2.3/test
	cp LICENSE $(MDP)
	cp README $(MDP)
	cp setup/INSTALL.unix-mindep $(MDP)/INSTALL
	cp setup/setup.sh.unix-s3d $(MDP)/setup.sh
	cd $(MINDEP);chown -R nobody pymol
	cd $(MINDEP);chgrp -R nobody pymol

unix-s3d-product: unix-s3d-build
	cd $(MINDEP);tar -cvf - pymol | gzip > ../pymol-0_xx-bin-sharp3d.tgz

fast: .update
	/bin/rm -f .update 
	$(CC) $(BUILD) */*.o ov/src/*.o $(CFLAGS) $(LIB_DIRS) $(LIBS)

depends: 
	/bin/rm -f */*.p
	$(MAKE) .depends

partial:
	touch layer5/main.c
	touch layer1/P.c
	touch layer4/Cmd.c
	/bin/rm -f modules/pymol/_cmd.so pymol.exe
	$(MAKE)

clean: 
	touch .no_fail
	/bin/rm -f layer*/*.o ov/src/*.o layer*/*.p modules/*/*.pyc modules/*/*/*.pyc \
	layer*/.files layer*/.depends layer*/.includes layerOSX*/src*/*.o \
	*.log core */core game.* log.* _cmd.def .update .contrib .no_fail* \
	libPyMOL.a
	cd contrib;$(MAKE) clean

distclean: clean
	touch .no_fail
	/bin/rm -f modules/*.pyc modules/*.so modules/*/*.so modules/*/*/*.so \
	modules/*/*/*/*.so pymol.exe \
	modules/*/*.pyc modules/*/*/*.pyc modules/*/*/*/*.pyc .no_fail* test/cmp/*
	/bin/rm -rf build
	/bin/rm -rf products/*.tgz products/unix-mindep
	cd contrib;$(MAKE) distclean

pyclean: clean
	/bin/rm -rf build
	/bin/rm -rf ext/lib/python2.1/site-packages/pymol
	/bin/rm -rf ext/lib/python2.1/site-packages/chempy
	/bin/rm -rf ext/lib/python2.1/site-packages/pmg_tk
	/bin/rm -rf ext/lib/python2.1/site-packages/pmg_wx

dist: distclean
	cd ..;tar -cvf - pymol | gzip > pymol.tgz

pmw: 
	cd modules; gunzip < ./pmg_tk/pmw.tgz | tar xvf -

compileall:
	$(PYTHON_EXE) modules/compile_pymol.py


