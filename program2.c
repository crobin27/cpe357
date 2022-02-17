#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

//Cole Robinson, CPE357

#define PAGESIZE 4096;
typedef unsigned char byte;
typedef struct chunkhead
{
    unsigned int size;
    unsigned int info;
    unsigned char *next, *prev;
} chunkhead;
void *mymalloc(unsigned int size);
void myfree(unsigned char *address);
chunkhead *get_last_chunk();
void analyse();
chunkhead *bestfit(chunkhead *head, unsigned int dataSize);

chunkhead *head = NULL; //global linked list

int main(int argc, char *argv[])
{
    //main test
    byte *a[100];
    analyse(); //50% points
    for (int i = 0; i < 100; i++)
        a[i] = mymalloc(1000);
    for (int i = 0; i < 90; i++)
        myfree(a[i]);
    analyse(); //50% of points if this is correct
    myfree(a[95]);
    a[95] = mymalloc(1000);
    analyse(); //25% points, this new chunk should fill the smaller free one
               //(best fit)
    for (int i = 90; i < 100; i++)
        myfree(a[i]);
    analyse(); // 25% should be an empty heap now with the start address
               //from the program start


    /*
    //clocktest
    byte *a[100];
    clock_t ca, cb;
    ca = clock();
    for (int i = 0; i < 100; i++)
        a[i] = mymalloc(1000);
    for (int i = 0; i < 90; i++)
        myfree(a[i]);
    myfree(a[95]);
    a[95] = mymalloc(1000);
    for (int i = 90; i < 100; i++)
        myfree(a[i]);
    cb = clock();
    printf("\n duration % f\n", (double)(cb - ca));
    */
    return 0;
}

void *mymalloc(unsigned int size)
{
    if (size == 0)
        return NULL;
    //this will be the pointer that is returned
    void *finalptr = NULL;
    //determine how many pages will be needed for this chunk
    int numpages = (size + sizeof(chunkhead)) / PAGESIZE;
    numpages++;
    unsigned int dataNeeded = numpages * PAGESIZE;

    //create first chunkhead if null
    if (head == NULL)
    {
        head = sbrk(dataNeeded);
        if (head == NULL)
        {
            printf("Memory failed to be allocated\n");
            return NULL;
        }
        finalptr = (void *)head + sizeof(chunkhead);
        head->info = 1;
        head->next = 0;
        head->prev = 0;
        head->size = dataNeeded;
    }
    //if not first chunk
    else
    {
        chunkhead *temp = head; //used to get back to start at end
        //move through until an empty chunk of correct size is found or reach the end
        head = bestfit(head, dataNeeded);

        //this is if there is a chunk created already with available space
        if ((head->info == 0) && (head->size >= dataNeeded))
        {
            head->info = 1;
            finalptr = (void *)head + sizeof(chunkhead);
        }
        //when we need to create a new chunk
        else if (head->next == 0)
        {
            //move program counter and save previous
            void *prev = sbrk(dataNeeded);
            if (prev == NULL)
            {
                printf("Memory failed to be allocated\n");
                return NULL;
            }
            //assign return value to final ptr
            finalptr = (void *)prev + sizeof(chunkhead);
            //make next chunk
            head->next = (unsigned char *)prev;
            ((chunkhead *)head->next)->prev = (unsigned char *)head;

            head = (chunkhead *)head->next;

            head->info = 1;
            head->size = dataNeeded;
            head->next = 0;
        }

        else //shouldn't get here
        {
            printf("Memory Failed to be allocated\n");
            return 0;
        }
        head = temp; //reset head to beginning chunk
    }

    return finalptr;
}
chunkhead *bestfit(chunkhead *head, unsigned int dataSize)
{
    chunkhead *ch = head;
    chunkhead *finalptr = get_last_chunk();
    unsigned int best = PAGESIZE + 1;
    while (ch->next != 0)
    {
        if (ch->info == 0 && ch->size >= dataSize)
        {
            if ((ch->size - dataSize) < best)
            {
                best = ch->size - dataSize;
                finalptr = ch;
            }
        }
        ch = (chunkhead *)ch->next;
    }
    //check last one
    if (ch->info == 0 && ch->size >= dataSize)
    {
        if ((ch->size - dataSize) < best)
        {
            best = ch->size - dataSize;
            finalptr = ch;
        }
    }

    return finalptr;
}

void myfree(unsigned char *address)
{

    //this is used to determine the location of the chunk
    chunkhead *temp = head; //store current location of head (front)
    //loop through and find the correct address to free
    if (address == NULL)
        return;
    head = (chunkhead *)(address - sizeof(chunkhead));
    if (head->info == 1)
    {
        head->info = 0;
    }
    else
    {
        printf("no memory to be freed at this location\n");
        return;
    }
    //now check for empty neighbor cells
    //check for empty neighbor cell ahead
    if (head->next != 0 && ((chunkhead *)head->next)->info == 0)
    {
        //add sizes together
        unsigned int newSize = head->size + ((chunkhead *)head->next)->size;
        head->size = newSize;
        //check if the next next node is the last, in which case this is new last node
        if (((chunkhead *)head->next)->next == 0)
        {
            head->next = 0;
        }
        else //link next of next back
        {
            //set next to two nodes ahead
            head->next = ((chunkhead *)head->next)->next;
            //set the new next node's prev to head
            ((chunkhead *)head->next)->prev = (unsigned char *)head;
        }
    }
    //check for empty cell behind
    if (head->prev != 0 && ((chunkhead *)head->prev)->info == 0)
    {
        unsigned int newSize = head->size + ((chunkhead *)head->prev)->size;
        //this will deal with if we don't have to link back (ie last node)
        if (head->next == 0)
        {
            head = (chunkhead *)head->prev;
            head->size = newSize;
            head->next = 0;
        }
        else
        {
            unsigned char *newNext = head->next;
            head = (chunkhead *)head->prev; //this will be the new larger node, we'll forget about the one we're on
            head->size = newSize;
            head->next = newNext;
            //now we must link back the new next node
            ((chunkhead *)head->next)->prev = (unsigned char *)head;
        }
    }

    if (head->info == 0 && head->next == 0 && head->prev == 0)
    {
        head = NULL;
    }
    else
    {
        head = temp;
    } //restore original head
}

chunkhead *get_last_chunk() //you can change it when you aim for performance
{
    if (!head) //I have a global void *head = NULL;
        return NULL;
    chunkhead *ch = (chunkhead *)head;
    for (; ch->next; ch = (chunkhead *)ch->next)
        ;
    return ch;
}

void analyse()
{
    printf("\n--------------------------------------------------------------\n");
    if (!head)
    {
        printf("no heap, program break on address: %p\n", sbrk(0));
        return;
    }
    chunkhead *ch = head;
    for (int no = 0; ch; ch = (chunkhead *)ch->next, no++)
    {
        printf("%d | current addr: %p |", no, ch);
        printf("size: %d | ", ch->size);
        printf("info: %d | ", ch->info);
        if (ch->next == 0)
            printf("next: %d | ", 0);
        else
            printf("next: %p | ", ch->next);
        if (ch->prev == 0)
            printf("prev: %d", 0);
        else
            printf("prev: %p", ch->prev);
        printf("      \n");
    }
    printf("program break on address: %p\n", sbrk(0));
}