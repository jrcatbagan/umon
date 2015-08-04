/*
 * Copyright (c) 2015 Jarielle Catbagan <jcatbagan93@gmail.com>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

int mmc(int argc, char *argv[]);
int mmcInit(int interface, int verbose);
int mmcRead(int interface, char *buf, int blknum, int blkcnt);
int mmcWrite(int interface, char *buf, int blknum, int blkcnt);
int mmcInstalled(int interface);
