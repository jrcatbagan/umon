/**************************************************************************
 *
 * Copyright (c) 2013 Alcatel-Lucent
 * 
 * Alcatel Lucent licenses this file to You under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except in
 * compliance with the License.  A copy of the License is contained the
 * file LICENSE at the top level of this repository.
 * You may also obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************
 *
 * password:
 *
 * Code that supports the use of a password in the monitor.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "stddefs.h"
#include "genlib.h"
#include "ether.h"
#include "tfs.h"
#include "tfsprivate.h"

#if INCLUDE_USRLVL

extern	char *crypt(char *,char *,char *);

/* PASSWORD_FILE:
 *	The name of the file that is used to store passwords.
 */
#ifndef PASSWORD_FILE
#define PASSWORD_FILE	".monpswd"
#endif

#define SALT0	'T'
#define SALT1	'J'

int
passwordFileExists(void)
{
	if (tfsstat(PASSWORD_FILE))
		return(1);
	return(0);
}

/* backdoor():
 *	A mechanism that allows a password to be generated based on the
 *	MAC address of the board...
 *
 *	Return 1 if the password matches the return of crypt(KEY,SALT).
 *	Where:
 *		KEY is an 8-character string created from the MAC address
 *			stored in the ETHERADD environment variable;
 *		SALT is a 2-character string made up of the first and last
 *			character of the ETHERADD env var;
 */
int
backdoor(char *password)
{
	int		i;
	char	*etherstr, salt[3], encryption[32];
	unsigned char etherbin[9];

	etherstr = getenv("ETHERADD");	
	if (!etherstr)	/* just in case... */
		etherstr = "00:00:00:00:00:00";

	/* 2 salt characters are the first and last characters of etherstr: */
	salt[0] = etherstr[0];
	salt[1] = etherstr[strlen(etherstr)-1];
	salt[2] = 0;

	EtherToBin(etherstr,etherbin);
	etherbin[6] = 'A';
	etherbin[7] = 'a';
	etherbin[8] = 0;

	for(i=0;i<6;i++) {
		while (etherbin[i] > 0x7e) {
			etherbin[i] -= 0x20;
			etherbin[6]++;
		}
		while (etherbin[i] < 0x20) {
			etherbin[i] += 0x8;
			etherbin[7]++;
		}
	}
		
	crypt((char *)etherbin,salt,encryption);

	/* Note that the first two characters of the return from crypt() */
	/* are not used in the comparison... */
	if (!strcmp(password,encryption+2))
		return(1);
	else
		return(0);
}

/* validPassword():
 *	Called with a password and user level.	There are a few different ways
 *	in which this can pass...
 *	1. If there is no PASSWORD_FILE in TFS, return true.
 *	2. If the password matches the backdoor entry, return true.
 *	3. If the password matches appropriate entry in the password file,
 *	   return  true.
 */

int
validPassword(char *password, int ulvl)
{
	char	line[80];
	int		(*fptr)(void);
	int		tfd, lno, pass, lsize;

	/* If there is a password file, then use it.
	 * If there is no password file, then call extValidPassword() to
	 * support systems that store the password some other way.
	 * The extValidPassword() function should return 0 if password check
	 * failed, 1 if the check succeeded and -1 if there is no external
	 * password storage hardware.
	 * Finally, if there is no password file, AND there is no external
	 * password storage hardware, then just return 1 to
	 * indicate that ANY password is a valid password.  In other words...
	 * the password protection stuff is only enabled if you have a password
	 * stored away somewhere.
	 */
	if (!passwordFileExists()) {
		static	int warning;

		switch(extValidPassword(password,ulvl)) {
			case 0:			/* Password check failed. */
				return(0);
			case 1:			/* Password check passed. */
				return(1);
			default:		/* No external password check in use. */
				break;
		}
		if (!warning) {
			printf("WARNING: no %s file, security inactive.\n",PASSWORD_FILE);
			warning = 1;
		}
		return(1);
	}

	/* First check for backdoor entry... */
	if (backdoor(password))
		return(1);

	/* Incoming user level must be in valid range... */
	if ((ulvl < MINUSRLEVEL) || (ulvl > MAXUSRLEVEL))
		return(0);

	/* If user level is MINUSRLEVEL, there is no need for a password. */
	if (ulvl == MINUSRLEVEL)
		return(1);

	/* Check for a match in the password file...
	 * The PASSWORD_FILE dedicates one line for each user level.
	 * Line1 is password for user level 1.
	 * Line2 is password for user level 2.
	 * Line3 is password for user level 3.
	 * User level 0 doesn't require a password.
	 */

	/* Force the getUsrLvl() function to return MAX: */
	fptr = (int(*)(void))setTmpMaxUsrLvl();

	tfd = tfsopen(PASSWORD_FILE,TFS_RDONLY,0);
	if (tfd < 0) {
		/* Restore the original getUsrLvl() functionality: */
		clrTmpMaxUsrLvl(fptr);
		printf("%s\n",(char *)tfsctrl(TFS_ERRMSG,tfd,0));
		return(0);
	}

	lno = 1;
	pass = 0;
	while((lsize=tfsgetline(tfd,line,sizeof(line)))) {
		char	encryption[32], salt[3];

		if (lno != ulvl) {
			lno++;
			continue;
		}

		line[lsize-1] = 0;			/* Remove the newline */

		salt[0] = SALT0;
		salt[1] = SALT1;
		salt[2] = 0;

		crypt(password,salt,encryption);

		if (!strncmp(line,encryption+2,strlen(line)-2))
			pass = 1;
		break;
	}
	tfsclose(tfd,0);
	
	/* Restore the original getUsrLvl() functionality: */
	clrTmpMaxUsrLvl(fptr);

	return(pass);
}

/* newPasswordFile():
 *	Prompt the user for three passwords.  One for each user level.
 *	Then, after retrieving them, make sure the user wants to update
 *	the password file; if yes, do it; else abort.
 */
int
newPasswordFile(void)
{
	int		err, i;
	char	pswd1[16], pswd2[16];
	char	salt[3], *epwp, buf[32], epswd[34*3], flags[8];

	/* For each of the three levels (1,2&3), get the password, encrypt it
	 * and concatenate newline for storate into the password file.
	 */

	epwp = epswd;
	for(i=1;i<4;i++) {
		while(1) {
			printf("Lvl%d ",i);
			getpass("passwd: ",pswd1,sizeof(pswd1)-1,0);
			if (strlen(pswd1) < 8) {
				printf("password must be >= 8 chars.\n");
				continue;
			}
			getpass("verify: ",pswd2,sizeof(pswd2)-1,0);
			if (strcmp(pswd1,pswd2))
				printf("Entries do not match, try again...\n");
			else
				break;
		}
		salt[0] = SALT0;
		salt[1] = SALT1;
		salt[2] = 0;
		crypt(pswd1,salt,buf);
		sprintf(epwp,"%s\n",&buf[2]);
		epwp += strlen(epwp);
	}

	if (!askuser("Are you sure?"))
		return(0);

	err = tfsunlink(PASSWORD_FILE);
	if ((err != TFS_OKAY) && (err != TFSERR_NOFILE)) {
		printf("%s\n",(char *)tfsctrl(TFS_ERRMSG,err,0));
		return(-1);
	}

	sprintf(flags,"u%d",MAXUSRLEVEL);
	err = tfsadd(PASSWORD_FILE,0, flags, (uchar *)epswd, strlen(epswd));
	if (err != TFS_OKAY) {
		printf("%s\n",(char *)tfsctrl(TFS_ERRMSG,err,0));
		return(-1);
	}
	return(0);
}


#ifndef EXT_VALID_PASSWORD
/* extValidPassword():
 * See validPassword() above for notes on the purpose of this function.
 *
 * This is the default stub, if a real extValidPassword() function is
 * provided by the port, then EXT_VALID_PASSWORD should be defined in
 * that port's config.h file.
*/
int
extValidPassword(char *password, int ulvl)
{
	return(-1);
}
#endif	/* EXT_VALID_PASSWORD */
#endif	/* INCLUDE_USRLVL */
