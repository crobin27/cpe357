#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*strcpy takes first as destination and second is source*/
void push();
void printstr();
void delete();

struct listelement
{
    struct listelement *next, *prev;
    int front;
    char text[1000];
};

struct listelement *head = NULL;


int main(int argc, char *argv[])
{
    /*this firstPush variable is used to determine if it is the first item being added to the list*/
    /*allocate 100 listelements, should be sufficient for this lab*/
    head = (struct listelement*)malloc(100 * sizeof(struct listelement));
    if(head == NULL)
        return -1;
    int operation = 0;
    while (operation != 4)
    {
        printf("Choose one of the following\n");
        printf("1 push string\n2 print list\n3 delete item\n4 end program\n");
        int valid = scanf("%d", &operation);
        if (valid != 1 || operation > 4 || operation < 1)
        {   
            printf("Value must be within 1 and 4\n");
            operation = 0;
        }
        else if (operation == 1)
        {
            push();
        }
        else if (operation == 2)
            printstr();
        else if(operation == 3)
            delete();
    }
    free(head);
    return 0;
}

void push()
{
    printf("Enter a string: ");
    char text[1000];
    int x = scanf("%s", text);
    /*create and allocate memory for new node*/
    struct listelement *node = NULL;
    node = (struct listelement*)malloc(sizeof(struct listelement));
    /*make next node null and put text in node*/
    node->next = NULL;
    strcpy(node->text, text);
    node->front = 0;
    /*check if this is going to be the first node*/
    if (head->next == NULL && head->front == 0)
    {
        node->prev = NULL;
        node->front = 1;
        head = node;
    }
    else
    {
        /*traverse to end of list*/
        while (head->next != NULL)
        {
            head = head->next;
        }
        /*assign node to end of list*/
        node->prev = head;
        head->next = node;
        while (head->front != 1)
        {
            head = head->prev;
        }
    }
}

void printstr()
{   
    if (head->prev == NULL && head->front == 0)
    {
        printf("List is empty, push some items on first\n");
        return;
    }
    while (head->next != NULL)
    {
        printf("%s\n", head->text);
        head = head->next;
    }
    printf("%s\n", head->text);
    while (head->front != 1)
    {
        head = head->prev;
    }
}

void delete()
{
    //index is holding the node to be removed
    //count is holding the current node that we're traveling through
    int index = 0;
    int count = 0;
    printf("what element would you like to delete: ");
    int valid = scanf("%d", &index);
    index--;
    if (index < 0)
    {
        printf("value must be a positive number\n");
        return;
    }
    else if (index == 0)
    {
        //this edge case covers if the list has only one item to be removed (first item)
        if (head->front == 0)
            return;
        if (head->next == NULL)
        {
            free(head);
            head = (struct listelement*)malloc(100*sizeof(struct listelement));
        }
        else
        {
        head = head->next;
        head->front = 1;
        }
    }
    else
    {   
        /*stop right before*/
        while (index - count > 1)
        {
            
            if (head->next == NULL)
            {
                //this breaks out of the while loop
                count = index;
            }
            else
            {
                count++;
                head = head->next;
            }
        }
        if (head->next != NULL)
        {
            if (head->next->next == NULL)
                head->next = NULL;
            else
            {
                head->next = head->next->next;
                head->next->prev = head;
            }
        }
        else
            printf("list element does not exist\n");
    }
    if (head->prev != NULL)
    {
    while (head->front != 1)
        head = head->prev;
    }
}