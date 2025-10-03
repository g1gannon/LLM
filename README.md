
The Linked list manager is a doubly linked list of memory data element and can be used for rapid in memory access to data control blocks.  
It provides direct access capabibilities which can store the direct access address in a transmisssion header.  This is useful in network or messaging application where the over head of 
searching the list adds overhead. (Especially useful in a large socket applicatioon). 
/**----------------------------------------------------------------
 * File:LinkList.cpp
 *
 * Author: Gregory Gannon
 * Created in 10-1995
 *
 * PURPOSE
 *
 *  This utility manages a linked list of size between
 *    ( 1 byte to 8192 bytes)
 *    Access to the list is allowed one element at a time
 *    through the public data area of the class.  When you register
 *    with the list utility a new element buffer is automatically
 *    generated to use as a work space to add elements.
 *    The buffer is added to the list on each add operation.
 *    The current USER DATA area for the element element
 *    is pointed to by the pUserCurrentElement Pointer.

 *
 * UPDATES:
 *
 * GMG   5-17-1996     Modified the logging routine for simplicity
 * ALS   5-29-1997     Fixed the ListDeregister method
 * GMG   08-09-2025    Changed status reporting to a called function, Return codes to bool T/F
 *                      Changed defines, split command and status tables. Changed the direct pointing 
 *                      capabilities and removed a bunch of junk. Added GetStatus().
 *                      fixed Delete_all routine.
 *-------------------------------------------------------------
*/
