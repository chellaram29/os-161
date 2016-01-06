#include<types.h>
#include<kern/errno.h>
#include<lib.h>
#include<kern/list.h>

struct node *head = NULL;

int insert(struct node **head, void *data) {

	struct node *newentry = (struct node *) kmalloc(sizeof(struct node));
	if (newentry == NULL )
		return ENOMEM;
	newentry->data = data;
	newentry->next = NULL;

	if (*head == NULL )
		*head = newentry;

	else {
		struct node *ptr = *head;
		while (ptr->next != NULL )
			ptr = ptr->next;
		ptr->next = newentry;

	}

	return 0;
}

void delete(struct node **head, void *ptr) {

	struct node *p = *head;
	struct node *p1 = p->next;
	struct node *temp;
	int nodenumber = 0;
	int position=0;

	while(p->data!=ptr){

		position++;
		p=p->next;
	}

	p=*head;

	if (position == 0) {
		temp = *head;
		*head = (*head)->next;
		kfree(temp);
	} else {
		while ((p1->next) != NULL ) {

			if (nodenumber == position) {
				temp = p1->next;
				p->next = temp;
				kfree(p1);
				break;
			}

			p1 = p1->next;
			p = p->next;
			nodenumber++;
		}
		nodenumber++;
		if (nodenumber == position) {
			kfree(p1);
			p->next = NULL;
		}

	}

}
