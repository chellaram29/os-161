/*
 * Copyright (c) 2001, 2002, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Driver code for whale mating problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include<synch.h>

#define NMATING 10
struct cv *cv_male,*cv_female,*cv_mm;
struct lock *lk_male,*lk_female,*lk_mm;


int no_males=0,no_females=0,no_mm=0;
int ismwaiting=0,isfwaiting=0,ismmwaiting=0;

static
void
male(void *p, unsigned long which)
{

	(void)p;
	kprintf("male whale #%ld starting\n", which);
lock_acquire(lk_male);

no_males++;


if(no_females==0 || no_mm ==0 ){
	ismwaiting=1;
	cv_wait(cv_male,lk_male);
	ismwaiting=0;
}

if(isfwaiting==0)
	cv_signal(cv_female,lk_female);
if(ismmwaiting==0)
	cv_signal(cv_mm,lk_mm);


kprintf("male whale #%ld ending\n", which);
lock_release(lk_male);


}

static
void
female(void *p, unsigned long which)
{
	(void)p;
	kprintf("female whale #%ld starting\n", which);
	lock_acquire(lk_female);
no_females++;

if(no_males==0 || no_mm==0)
	cv_wait(cv_female,lk_female);

if(ismwaiting==0)
	cv_signal(cv_male,lk_male);
if(ismmwaiting==0)
	cv_signal(cv_mm,lk_mm);

	kprintf("female whale #%ld ending\n", which);
	lock_release(lk_female);


}

static
void
matchmaker(void *p, unsigned long which)
{
	(void)p;
	kprintf("matchmaker whale #%ld starting\n", which);

	lock_acquire(lk_mm);

no_mm++;
	if(no_females==0 || no_males==0)
		cv_wait(cv_mm,lk_mm);
	if(ismwaiting==0)
		cv_signal(cv_male,lk_male);
	if(isfwaiting==0)
		cv_signal(cv_female,lk_female);

	lock_release(lk_mm);


kprintf("matchmaker whale #%ld ending\n", which);

}


// Change this function as necessary
int
whalemating(int nargs, char **args)
{

	int i, j, err=0;

	cv_male=cv_create("male");
	cv_female=cv_create("female");
	cv_mm=cv_create("mm");


	lk_male=lock_create("male");
	lk_female=lock_create("female");
	lk_mm=lock_create("mm");



	
	(void)nargs;
	(void)args;

	for (i = 0; i < 3; i++) {
		for (j = 0; j < NMATING; j++) {
#ifdef UW
			switch(i) {
			    case 0:
				err = thread_fork("Male Whale Thread", NULL,
						  male, NULL, j);
				break;
			    case 1:
				err = thread_fork("Female Whale Thread", NULL,
						  female, NULL, j);
				break;
			    case 2:
				err = thread_fork("Matchmaker Whale Thread", NULL,
						  matchmaker, NULL, j);
				break;
			}
#else
			switch(i) {
			    case 0:
				err = thread_fork("Male Whale Thread",
						  male, NULL, j, NULL);
				break;
			    case 1:
				err = thread_fork("Female Whale Thread",
						  female, NULL, j, NULL);
				break;
			    case 2:
				err = thread_fork("Matchmaker Whale Thread",
						  matchmaker, NULL, j, NULL);
				break;
			}
#endif /* UW */
			if (err) {
				panic("whalemating: thread_fork failed: %s)\n",
				      strerror(err));
			}
		}
	}

	return 0;
}

