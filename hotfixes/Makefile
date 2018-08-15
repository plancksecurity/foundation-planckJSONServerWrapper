

ENGINE_INC = ../../pEpEngine/src
ENGINE_SRC = ../../pEpEngine/src

all: pep-hotfix

pep-hotfix: $(ENGINE_SRC)/platform_unix.c $(ENGINE_SRC)/stringlist.c main.c
	$(CC) -I. -I$(ENGINE_INC) -o pep-hotfix $^

test: pep-hotfix
	@rm -rf gnupg/ ; mkdir gnupg ; cp gpg.conf.broken gnupg/gpg.conf
	@head gnupg/gpg.conf
	@echo ==================================================
	#GNUPGHOME=$(PWD)/gnupg ./pep-hotfix || true
	@GNUPGHOME=gnupg ./pep-hotfix
	@echo ==================================================
	-@diff -Naur gpg.conf.broken gnupg/gpg.conf
	@head -n30 gnupg/gpg.conf.* /dev/null
	@GNUPGHOME=gnupg gpg --gpgconf-test
