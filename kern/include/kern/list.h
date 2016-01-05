/*
 * list.h
 *
 *  Created on: Dec 13, 2015
 *      Author: trinity
 */

#ifndef LIST_H_
#define LIST_H_

struct node {
void *data;
struct node *next;

};

int insert(struct node **head, void *data);
void delete(struct node **head, void *ptr);

#endif /* LIST_H_ */
