SHELL = /bin/bash

install.sh: install.tar.gz install.sh.source
	cp install.sh.source install.sh
	chmod +x install.sh
	truncate install.sh -s 4096
	cat install.tar.gz >> install.sh

install.tar.gz:
	cd .. ; tar cvzf install/install.tar.gz etc/ bin/
