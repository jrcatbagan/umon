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
 * date.h
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
struct todinfo {
	char day;		/* 1-7 (sun = 1) */
	char date;		/* 1-31 */
	char month;		/* 1-12 */
	char hour;		/* 0-23 */
	char minute;	/* 0-59 */
	char second;	/* 0-59 */
	short year;		/* full number (i.e. 2006) */
};

extern int	showDate(int center);
extern int	timeofday(int cmd, void *arg);
extern int	setTimeAndDate(struct todinfo *now);
extern int	getTimeAndDate(struct todinfo *now);

/* #defines used by the uMon API mon_timeofday()...
 */
#define TOD_ON		1
#define TOD_OFF		2
#define TOD_SET		3
#define TOD_GET		4
