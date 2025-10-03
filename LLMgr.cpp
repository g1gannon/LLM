/**----------------------------------------------------------------
 * File:LinkList.cpp
 *
 * Author: Gregory Gannon
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
 *       int: Usually 4 bytes (32 bits) on most 64-bit systems.
 *       long: Often 8 bytes (64 bits) on 64-bit systems.
 *       long long: Typically 8 bytes (64 bits) as well.
 *       size_t: Matches the system's pointer size, so it's 8 bytes (64 bits) on a 64-bit system.
 *
 * UPDATES:
 *
 * GMG   5-17-1996     Modified the logging routine for simplicity
 * ALS   5-29-1997     Fixed the ListDeregister method
 * WWF   11-18-1997    Moved from server to common.
 * GMG   08-09-2025    Changed status reporting to a called function, Return codes to bool T/F
 *                      Changed defines, split command and status tables. Changed the direct pointing 
 *                      capabilities and removed a bunch of junk. Added GetStatus().
 *                      fixed Delete_all routine.
 *-------------------------------------------------------------
*/

#include <time.h>
#include <iostream>     // Include the iostream header and String class is in this under windows
#include "LLMgr.h"

/*--------------------------------------CLASS GLOBALS ------------------------------------------------------
*  This  table contains mapping to character versions of the numeric commands.
*    The table is used for look up to populate the status control block.
*    The GetStatusBlock() can be called for a program to get the details.
* -----------------------------------------------------------------------------------------------------------
*/
static CommandTable_t LL_CommandArray[] =
{
    { LL_ADDAFTER, "LL_ADDAFTER - Request add after current element" },
    { LL_ADDBEFORE, "LL_ADDBEFORE - Resuest add before current element"},
    { LL_ADDEND, "LL_ADDEND - Request add at end of the list" },
    { LL_DELETE, "LL_DELETE- Request delete current element"},
    { LL_DELETE_ALL, "LL_DELETE_ALL - Request delete all elements" },
    { LL_DEREGISTER, "LL_DEREGISTER - Request to deregister the list" },
    { LL_REGISTER, "LL_REGISTER - Request to register the list" },
    { LL_SETDIRECTPOINTER, "LL_SETDIRECTPOINTER - Set the current element with the passed token" },
    { LL_GETDIRECTTOKEN, "LL_GETDIRECTTOKEN - Return a token for the current element" },
    { LL_pBOTTOM, "LL_pBOTTOM - Request to point to the last element" },
    { LL_pLAST, "LL_pLAST - Request to point to the previous element" },
    { LL_pNEXT, "LL_pNEXT - Request to point to the next element" },
    { LL_pTOP, "LL_pTOP - Request - to point to the top of the list" },
    { -1,            "MNEMONIC_UNKNOWN"      }
};

static StatusTable_t LL_StatusArray[] =
{
    { LL_STATUS_ALLOCFAIL, "LL_STATUS_ALLOCFAIL - Panic! - malloc() failed - unrecoverable" },
    { LL_STATUS_ALREADYREGISTERED, "LL_STATUS_ALREADYREGISTERED - List can only be registered 1 time" },
    { LL_STATUS_INVALIDADDRESS, "LL_STATUS_INVALIDADDRESS - TOKEN Address or Random number  did not match pointer Address" },
    { LL_STATUS_INVALIDMAGICTOKEN, "LL_STATUS_INVALIDMAGICTOKEN - The token MAGIC number did not validate" },
    { LL_STATUS_LISTEMPTY, "LL_STATUS_LISTEMPTY - Invalid operation on an empty list" },
    { LL_STATUS_LISTEND, "LL_STATUS_LISTEND - First or Last entry in the list reached" },
    { LL_STATUS_NOTEMPTY, "LL_STATUS_NOTEMPTY - List needs to be empty before deregistration" },
    { LL_STATUS_NOTREGISTERED, "LL_STATUS_NOTREGISTERED - Can't use the methods until the list is registered" },
    { LL_STATUS_INVALIDSIZE, "LL_STATUS_INVALIDSIZE - Range of user data area 1-8192" },
    { -1,                    "MNEMONIC_UNKNOWN"      }
};


    DirectToken_t ReturnToken;                          // Return From GetDirectToken() - Used in SetDirectPointer() 
    unsigned int   ListTotalElementLength = 0;          // Internal length of user area with pointer header
    StatusBlock_t Status;                               // Reports what happned in a method call
//--------------------------------------------------------------------
// Constructor method will init the local protected and user data to
//    reasonable values
//--------------------------------------------------------------------

LLMgr::LLMgr ()
{
    ElementCount        = 0;
    pUserCurrentElement = NULL;     
    pUserAddBuffer      = NULL;
    pListCurrent        = NULL;
    pListTop            = NULL;
    pListBottom         = NULL;
    pClassBuffer        = NULL;
    ListElementCount    = 0;
    ListUserElementLength   = 0;              // Internal length of user data area passed at registration
    ListRegistered      = false;
    Status.ReturnCode   = true;
    Status.Slistname    = "NOT SET";
    srand(static_cast<unsigned int>(time(0)));              // Seed the random number generator

}

/*
* Destructor cleans up memory if you did not
*/
 LLMgr::~LLMgr()
{

     ListDeleteAll();

    ElementCount        = 0;
    pUserCurrentElement = NULL;
    pUserAddBuffer      = NULL;
    pListCurrent        = NULL;
    pListTop            = NULL;
    pListBottom         = NULL;
    pClassBuffer        = NULL;
    ListElementCount    = 0;                 // Number of elements in the list
    ListUserElementLength   = 0;             // Internal length of user data area passed at registration
    ListRegistered      = false;

    // Added Deregister Check in here  als

    if (ListRegistered == true)
    {
       this-> ListDeregister();
    }

    ListRegistered      = false;

}

/*
 *--------------------------------------------------------------------
 * Function: Register the list which includes obtaining the
 *    necessary buffer for new elements to the list.
 *    This buffer is used to build an entry prior to it being added to the list.  
 *    
 *    Each user element of the list must be the same size.
 *    
 *    The buffer which is allocated is the sizeof the user area + the ListPointers_t which
 *    are prepended to the user data area.  We will then do buffer relocation
 *    to relocate the buffer into the list.
 *------------------------------------------------------------------
*/
bool LLMgr::ListRegister(long int ListSize, std::string Name)
{
    ListPointers_t  *pElementPointers;

    InitStatus(  LL_FILELINE, LL_REGISTER );

    if (ListRegistered != false)
    {
        SetStatusFail(LL_FILELINE, LL_STATUS_ALREADYREGISTERED, LL_REGISTER   );
        return false;
    }

    if (ListSize < 1 || ListSize > 8192)
    {
        SetStatusFail( LL_FILELINE, LL_STATUS_INVALIDSIZE, LL_REGISTER   );
        return false;
    }

/*----------------------------------------------------------------------------------------------------------
 *  Storage area for pointers and the element size passed at registration
 *      This is the user passed lenght + the pointers for the list.
 *       Calculated during registration and used in all add operations.
 *       Then allocate the entire storage area and initial pointer
 *       structure.
 *------------------------------------------------------------------------------------------------------------ 
*/
   ListTotalElementLength = ListSize + sizeof(ListPointers_t);          // Memory needed = pointer structure + user data area

   if ((pClassBuffer = malloc(ListTotalElementLength)) == NULL)         // Failure is not an opton - Panic!
    {
        SetStatusFail( LL_FILELINE, LL_STATUS_ALLOCFAIL, LL_REGISTER   );
        return false;
    }
/*----------------------------------------------------------------------------------------------------------
*   Now:
*       1. Seed the randome number generator
*       2. Save the class name
*       3. Set up the pointers and the user data area
*       4. Register the list.
*-------------------------------------------------------------------------------------------------------------
*/

    Status.Slistname = Name;                              // Add list name to status block

    pElementPointers       = (ListPointers_t *) pClassBuffer;   // Cast over pointer structure at the front
    pElementPointers->pFwd  = NULL;                       //  Set fwd pointer to null
    pElementPointers->pBwd  = NULL;                       //  set bwd pointer to null   
    pElementPointers->Address = pClassBuffer;             //  Store the direct address of this list element in the structure
    pElementPointers->Random = rand();                    //  Store random number in pointer structure
    pUserAddBuffer = (char *) pClassBuffer 
            + sizeof(ListPointers_t);                     // Add the pointer structure length to point to user data area

    ListRegistered    = true;                             // Set list to registered
    ListUserElementLength = ListSize;                     // Save user data area size for other methods
 
   return true;                                           // Nothing broke, done
}
/*
 *--------------------------------------------------------------------
 * Function: DeRegister the list which includes freeing the
 *    buffer for new elements. First we must assure they have
 *    not left any elements in the list.
 *------------------------------------------------------------------
*/

bool LLMgr::ListDeregister()
{
     InitStatus(  LL_FILELINE, LL_DEREGISTER );

    if (ListRegistered != true )                /* if not registered als */
    {
         SetStatusFail(  LL_FILELINE, LL_STATUS_NOTREGISTERED, LL_DEREGISTER  );
        return false;
    }

    if (ListElementCount > 0 )
    {
        SetStatusFail(  LL_FILELINE, LL_STATUS_NOTEMPTY , LL_DEREGISTER );
        return  false;
    }

    free(pClassBuffer);                                 // Free the temporaty buffer

    pUserAddBuffer = NULL;

    ListRegistered = false;

    return  true;
}

/*
 *--------------------------------------------------------------------------
 * Function: AddEnd will add an element to the end of the list and increment
 *    the element counter by one when successfully allocated and inserted
 *    into the list.
 *--------------------------------------------------------------------------
*/
bool LLMgr::ListAddEnd(void)
{
     void       *pHoldBuffer;                                   // Pointer to hold on to the current element
     void       *pNewElement;                                   // Pointer to the new element
     ListPointers_t *pNextEntry;                                // Cast of pointer structure over just the pointers
     ListPointers_t *pCurrentPointers;                          // Cast for pointer structure over current buffer
/*
 *-------------------------------------------------------------------
 * Check to be sure they registered their list
 *------------------------------------------------------------------
*/
     InitStatus(  LL_FILELINE, LL_ADDEND );

    if (ListRegistered != true)
    {
        SetStatusFail(  LL_FILELINE, LL_STATUS_NOTREGISTERED, LL_ADDEND  );
        return  false;
    }

/*
 *-----------------------------------------------------------------
 * 1)  Get the size of the element + pointer  fields.
 * 2) Now get the buffer for the size of the data + pointers.
 * 3) Once we have the buffer then we will want to
 *    copy user's new element buffer to a holder
 *    make the new buffer the user's new element buffer
 *    make Old user buffer the element to add to the list.
 *-----------------------------------------------------------------
*/

    if ((pNewElement = malloc(ListTotalElementLength)) == NULL)
    {
         SetStatusFail(  LL_FILELINE, LL_STATUS_ALLOCFAIL, LL_ADDEND  );
        return  false;
    }

    ((ListPointers_t*)pNewElement)->Random = rand();        // Get a random number for safety check
    ((ListPointers_t*)pNewElement)->Address = pNewElement;  // Save elements address in pointer structure

    pHoldBuffer    = pNewElement;	                         //  Save new storage area addr 
    pNewElement    = pClassBuffer;                           //  Make the current memory area the current to update the pointers
    pClassBuffer   = pHoldBuffer;                            // Make the make the new memory area the temporary for new items
    pUserAddBuffer = (char *) pClassBuffer + sizeof( ListPointers_t);
/*
 *--------------------------------------------------------------------------------------------------
 * 0) Set the Pointers to the current entry
 * 1) Null the Forward and Backward Pointers in the new element.
 * 2) Now check to see if an entry exist (1st update).
 *      if it is null, then begin the list.
 *      If it has a valid entry chase the chain until the end.
 *      Then add it to the end of the list.
 *----------------------------------------------------------------
*/
    pCurrentPointers       = ( ListPointers_t *) pNewElement;       // The current tempory is now availabe; fpr next ADD
    pCurrentPointers->pFwd = nullptr;                               // Set pointers to null  
    pCurrentPointers->pBwd = nullptr;
//
//   Nothing in the list - Set top/bottom  pointers to this element else set fwdptr 
//
    if (pListTop == NULL)
    {
        pListTop = pNewElement;
        pListBottom = pNewElement;
    }
    else
    {
        pNextEntry = ( ListPointers_t *) pListTop;                 //  First Entry - List has elements, so start at the top 
   //------------------------------------------------------------------
   // Loop thru looking for the end of the list.  Each time make the
   //      Last equal to the next then increment to the next.
   //------------------------------------------------------------------
        while (pNextEntry->pFwd != NULL)
        {
            pNextEntry =  ( ListPointers_t *)pNextEntry->pFwd;      // get the forward pointer
        }
   //--------------------------------------------------------------------------------------------------
   // We are now at the last entry so:
   //      1) Point to the current element from the last in list.
   //      2) Point to the last element from the current.
   //      3) Update bottom of list Pointer.  
   //   !!Note: Null for the last forward pointer initialized above = Set end of list
   //----------------------------------------------------------------------------------------------------
        pNextEntry->pFwd       = pNewElement;                       // Update last entry forward pointer
        pCurrentPointers->pBwd = pNextEntry;                        // The new element now points to last entry
        pListBottom            = pNewElement;                       // Reset bottom of the list
    }
/*
 *--------------------------------------------------------------------
 *  Now we need to:
 *    1. Set user area pointer for next request
*     2. Point to current element 
 *-------------------------------------------------------------------
*/
    pUserCurrentElement = (char *)pNewElement + sizeof( ListPointers_t);            // Set user area pointer past pointers
    pListCurrent        = pNewElement;                                              // Pointer to current element

/*
 *--------------------------------------------------------------------
 * Set the element count up by 1
 *-------------------------------------------------------------------
 */
    ElementCount = ++ListElementCount;

    return  true;
}

/*
 *--------------------------------------------------------------------
 *   ListAddBefore will add an element to the list before the
 *    current element of the list.  Storage will be aquired and the
 *    NewElement buffer will be copied to the new list element
 *    allocated from the memory heap.
 *-------------------------------------------------------------------
*/
bool LLMgr::ListAddBefore(void)
{
     ListPointers_t *pCurrentEntryPointers;
     ListPointers_t *pNewEntryPointers;
     ListPointers_t *pPriorEntryPointers;
    void *pNewBuffer;
    void *pHoldBuffer;
/*
 *-------------------------------------------------------------------
 * Check to be sure they registered their list
 *------------------------------------------------------------------
*/
     InitStatus(  LL_FILELINE, LL_ADDBEFORE );

    if (ListRegistered != true)
    {
         SetStatusFail(  LL_FILELINE, LL_STATUS_NOTREGISTERED ,LL_ADDBEFORE );
        return  false;
    }
/*
 *------------------------------------------------------------------
 * Check to see if this is the first entry in the list.
 *-----------------------------------------------------------------
*/
    if (pListTop == NULL)
    {
        return  ListAddEnd();
    }
/*
 *-----------------------------------------------------------------
 *  Get the local buffer for the size of the data + pointers. The constructor
 *       saved this in ListTotalElementLength.
 *-----------------------------------------------------------------
*/
    if ((pNewBuffer = malloc(ListTotalElementLength)) == NULL)
    {
        SetStatusFail(  LL_FILELINE, LL_STATUS_ALLOCFAIL, LL_ADDBEFORE  );
        return  false;
    }

    ((ListPointers_t*)pNewBuffer)->Random = rand();        // Get a random number for safety check
    ((ListPointers_t*)pNewBuffer)->Address = pNewBuffer;  // Save elements address in pointer structure

/*
 *------------------------------------------------------------------
 * Relocate the user's add buffer to the new buffer and the
 *   new buffer to the user's add buffer.
 *-----------------------------------------------------------------
*/
    pHoldBuffer    = pNewBuffer;	 // save new buffer addr
    pNewBuffer     = pClassBuffer;   //  Get the current class buffer
    pClassBuffer   = pHoldBuffer;    // make the new buff the add buff
    pUserAddBuffer = (char *) pClassBuffer + sizeof( ListPointers_t);
/*
 *------------------------------------------------------------------
 *  Cast the forward and backwards pointers to the currnt entry
 *     and to the new entry to be added to the list.
 *------------------------------------------------------------------
*/
    pCurrentEntryPointers = ( ListPointers_t *) pListCurrent;
    pNewEntryPointers     = ( ListPointers_t *) pNewBuffer;
/*
 *------------------------------------------------------------------
 * Set the pNewEntry to the local new entry, and NULL Fwd and Bwd.
 *-----------------------------------------------------------------
*/
    pNewEntryPointers->pFwd = NULL;
    pNewEntryPointers->pBwd = NULL;
/*
 *------------------------------------------------------------------
 * Since AddBefore, check for top of list.
 *------------------------------------------------------------------
 * At top, must
 * 1) Set the Fwd pointer of the NewEntry to what was the ListTop
 * 2) Set Bwd pointer of the Current to the locally defined BeforeBuffer
 * 3) Adjust the ListTop to point to the locally defined BeforeBuffer
 *-----------------------------------------------------------------
*/
    if (pCurrentEntryPointers->pBwd == NULL)
    {
        pNewEntryPointers->pFwd     = pListTop;
        pCurrentEntryPointers->pBwd = pNewBuffer;
        pListTop                    = pNewBuffer;
    }
    else
    {
/*
 *------------------------------------------------------------------
 * Not at top, must
 * 1) Set the Fwd pointer of the NewEntry to what is ListCurrent
 * 2) Set the Bwd pointer of the NewEntry to what was the Bwd pointer
 *    of the current entry.
 * 3) Cast the PriorEntry's pointers from the current Bwd pointer.
 * 4) Set the PriorEntry's Fwd pointer to the locally defined
 *    BeforeBuffer.
 * 5) Set the Bwd pointer of what was the current entry to point to
 *    the locally defined BeforeBuffer.
 *-----------------------------------------------------------------
*/
        pNewEntryPointers->pFwd = pListCurrent;
        pNewEntryPointers->pBwd = pCurrentEntryPointers->pBwd;

        pPriorEntryPointers = ( ListPointers_t *)pCurrentEntryPointers->pBwd;

        pPriorEntryPointers->pFwd   = pNewBuffer;
        pCurrentEntryPointers->pBwd = pNewBuffer;
    }
/*
 *--------------------------------------------------------------------
 * Now we need to:
 * 1) Make the new buffer the current entry
 * 2) Point the user to there data area portion of the entry
 *-------------------------------------------------------------------
*/

    pUserCurrentElement = (char *)pNewBuffer + sizeof( ListPointers_t);
    pListCurrent        = pNewBuffer;

/*
 *--------------------------------------------------------------------
 * Set the element count up by 1
 *-------------------------------------------------------------------
*/
    ElementCount = ++ListElementCount;

    return  true;
}

/*
 *--------------------------------------------------------------------
 *  Add the list element after the current element in the list.
 *     The current pointer will be positioned to the new element.
 *     Once all the repairs have been done to the list chains.
 *--------------------------------------------------------------------
*/
bool LLMgr::ListAddAfter(void)
{
    void    *pHoldBuffer;
    void    *pAfterBuffer;
     ListPointers_t *pCurrentPointers;
     ListPointers_t *pNewEntry;

/*
 *-------------------------------------------------------------------
 * Check to be sure they registered their list
 *------------------------------------------------------------------
*/
     InitStatus(  LL_FILELINE, LL_ADDAFTER );

    if (ListRegistered != true)
    {
         SetStatusFail(  LL_FILELINE, LL_STATUS_NOTREGISTERED,LL_ADDAFTER  );
        return  false;
    }
/*
 *------------------------------------------------------------------
 * Check to see if this is the first entry in the list.
 *-----------------------------------------------------------------
*/
    if (pListTop == NULL)
    {
        return  ListAddEnd();
    }

/*
 *-----------------------------------------------------------------
 *  Get the local buffer for the size of the data + pointers.
 *-----------------------------------------------------------------
*/
    if ((pAfterBuffer = malloc(ListTotalElementLength)) == NULL)
    {
        SetStatusFail(  LL_FILELINE, LL_STATUS_ALLOCFAIL, LL_ADDAFTER  );
        return  false;
    }

    ((ListPointers_t*)pAfterBuffer)->Random = rand();        // Get a random number for safety check
    ((ListPointers_t*)pAfterBuffer)->Address = pAfterBuffer;  // Save elements address in pointer structure

/*
 *------------------------------------------------------------------
 * Set the pPointers to the current entry.
 *-----------------------------------------------------------------
*/
    pHoldBuffer    = pAfterBuffer;	 // save new buffer addr
    pAfterBuffer   = pClassBuffer;   //  Get the current class buffer
    pClassBuffer   = pHoldBuffer;    // make the new buff the add buff
    pUserAddBuffer = (char *) pClassBuffer + sizeof( ListPointers_t);
/*
 *------------------------------------------------------------------
 * Set the current entry forward and backward pointers up
 *------------------------------------------------------------------
*/
    pCurrentPointers = ( ListPointers_t *) pListCurrent;
/*
 *------------------------------------------------------------------
 * Set the pNewEntry to the local new entry, and NULL Fwd and Bwd.
 *-----------------------------------------------------------------
*/
    pNewEntry       = ( ListPointers_t *) pAfterBuffer;
    pNewEntry->pFwd = NULL;
    pNewEntry->pBwd = NULL;
/*
 *------------------------------------------------------------------
 * Since AddAfter, check for bottom of list.
 *-----------------------------------------------------------------
*/
    if (pCurrentPointers->pFwd == NULL)
    {
/*
 *------------------------------------------------------------------
 * At bottom, must
 * 1) Set the Bwd pointer of the NewEntry to what was the ListBottom
 * 2) Set Fwd pointer of the Current to the locally defed AfterBuffer
 * 3) Adjust the ListBottom to point to the locally defed Afterbuffer
 *-----------------------------------------------------------------
*/
        pNewEntry->pBwd        = ( ListPointers_t *)pListBottom;
        pCurrentPointers->pFwd = pAfterBuffer;
        pListBottom            = ( ListPointers_t *)pAfterBuffer;
    }
    else
    {
/*
 *------------------------------------------------------------------
 * Not at bottom, must
 * 1) Set the Bwd pointer of the NewEntry to what is ListCurrent
 * 2) Set the Fwd pointer of the NewEntry to what was the Fwd pointer
 *    of the current entry.
 * 3) Set the Fwd pointer of what was the current entry to point to
 *    the locally defined AfterBuffer.
 *-----------------------------------------------------------------
*/
        pNewEntry->pBwd        = ( ListPointers_t *)pListCurrent;
        pNewEntry->pFwd        = pCurrentPointers->pFwd;
        pCurrentPointers->pFwd = pAfterBuffer;
    }
/*
 *--------------------------------------------------------------------
 * Now we need to:
 *  Set the user current entry to their added entry and our
 *  current pointer to this entry.
 *-------------------------------------------------------------------
*/

    pUserCurrentElement = (char *)pAfterBuffer + sizeof( ListPointers_t);
    pListCurrent        = pAfterBuffer;

/*
 *--------------------------------------------------------------------
 * Set the element count up by 1
 *-------------------------------------------------------------------
*/
    ElementCount = ++ListElementCount;


    return  true;
}

/*
*--------------------------------------------------------------
*  Deletes all items of the list
*--------------------------------------------------------------
*/

 bool  LLMgr::ListDeleteAll(void){

   InitStatus(  LL_FILELINE, LL_DELETE_ALL );
   ListPointTop();                          // Fixed Al's coding  mistake - GMG 2025-08-21
  
if(ElementCount){                           // if we have anything to delete
    while( ListDelete() ==  true);
  }


  return true;
}

/*
 *--------------------------------------------------------------------
 *  Delete the linked list entry which is at the current pointer
 *    then adjust the users current pointer to point to the
 *    correct entry.  This entry can be null if the list is empty
 *    so the next read or access can create a storage violation if
 *    the return code is not checked.
 *--------------------------------------------------------------------
*/

bool LLMgr::ListDelete(void)
{
/*
 *----------------------------------------------------------------
 * TopBottomSwitch Values
 * N = None      - T = Top      - B = Bottom
 *---------------------------------------------------------------
*/
    char    		TopBottomSwitch;
     ListPointers_t   *pNextEntry ;
     ListPointers_t  *pCurrentPointers;
/*
 *----------------------------------------------------------------
 * 0) Assure we have registered the list
 * 1) Initialize the status return information.
 * 2) Check to assure we are not pointing to a NULL list.
 *----------------------------------------------------------------
*/
     InitStatus(  LL_FILELINE, LL_DELETE );

    if (ListRegistered != true)
    {
         SetStatusFail(  LL_FILELINE, LL_STATUS_NOTREGISTERED, LL_DELETE  );
        return  false;
    }

    if (pListTop == NULL)
    {
         SetStatusFail(  LL_FILELINE, LL_STATUS_LISTEMPTY, LL_DELETE  );
        return  false;
    }
/*
 *-------------------------------------------------------------------
 * The checks must be complete if we get here so
 *   cast the pointers on the current entry fro the repair job.
 *------------------------------------------------------------------
*/
    pCurrentPointers = ( ListPointers_t *)pListCurrent;
    TopBottomSwitch  = 'N';
/*
 *-------------------------------------------------------------------
 * 1) Check to see if we are at the head of the list , if we are
 * 2) Make the NextEntry = to the forward pointer of current.
 * 3) Check to see if NextEntry is null.  When Null, no elements are
 *      left in the list so null all pointers.
 *      If it is not, Null the backward pointer and update the
 *      top and current list pointers.
 *------------------------------------------------------------------
*/
    if (pListTop == pListCurrent)
    {
        TopBottomSwitch = 'T';
        pNextEntry = ( ListPointers_t *)pCurrentPointers->pFwd;

        if (pNextEntry == NULL)
        {
            pListTop     = NULL;
            pListBottom  = NULL;
            pListCurrent = NULL;
        }
        else
        {
            pNextEntry->pBwd = NULL;
            pListTop         = pNextEntry;
            pListCurrent     = pNextEntry;
        }
    }
/*
 *----------------------------------------------------------------
 * 4) Check to see if we are at the Tail of the list,
 *      and we did not go through the top list routine. If we are
 *      then do the same stuff as above but with the last entry.
 *----------------------------------------------------------------
*/
    if (pListBottom == pCurrentPointers && TopBottomSwitch == 'N')
    {
        TopBottomSwitch = 'B';
        pNextEntry = ( ListPointers_t*)pCurrentPointers->pBwd;

        if (pNextEntry == NULL)
        {
            pListTop = NULL;
            pListBottom = NULL;
            pListCurrent = NULL;
        }
        else
        {
            pNextEntry->pFwd = NULL;
            pListBottom      = pNextEntry;
            pListCurrent     = pNextEntry;
        }
    }
/*
 *----------------------------------------------------------------
 *  5) Not top, not bottom so do the standard delete and repair.
 *      First point to the last entry
 *      Next take the current forward pointer and move it to the
 *              Last elements fwd pointer.
 *      Now point to the next entry and move the current element
 *              back pointer to the element.
 *----------------------------------------------------------------
*/
    if (TopBottomSwitch == 'N')
    {
        pNextEntry       = ( ListPointers_t *)pCurrentPointers->pBwd;
        pNextEntry->pFwd = pCurrentPointers->pFwd;
        pNextEntry       = ( ListPointers_t *)pCurrentPointers->pFwd;
        pNextEntry->pBwd = pCurrentPointers->pBwd;
        pListCurrent     = pNextEntry;
    }
/*
 *----------------------------------------------------------------
 * 0) Check to see if the list is empty.  When it is set the users
 *    current pointer to null.
 * 1) Decrement the element counter
 * 2) Update the user's element counter
 * 3) Return
 *----------------------------------------------------------------
*/
    if (pListCurrent != NULL)
    {
        pUserCurrentElement = (char *)pListCurrent + sizeof ( ListPointers_t);
    }
    else
    {
        pUserCurrentElement = NULL;
    }

    ElementCount = --ListElementCount;

    (( ListPointers_t *) pCurrentPointers)->Random = 0;

    free(pCurrentPointers);

    return  true;
}

/*
 *--------------------------------------------------------------------
 *  ListPointTop sets the current pointer equal to the
 *   pListTop pointer
 *------------------------------------------------------------------
*/

bool LLMgr::ListPointTop(void)
{
/*
 *----------------------------------------------------------------
 * Indicate the last command to be executed for dubugging
 *----------------------------------------------------------------
*/
     InitStatus(  LL_FILELINE, LL_pTOP );

/*
 *-----------------------------------------------------------------
 * 1) Current pointer to top
 * 2) Set user pointer to the correct off set
 *----------------------------------------------------------------
*/
    pListCurrent = pListTop;

    pUserCurrentElement = (char *)pListCurrent + sizeof( ListPointers_t);

    return  true;
}
/*
 *--------------------------------------------------------------------
 *  ListPointBottom sets the current pointer equal to the
 *   pListBottom pointer
 *------------------------------------------------------------------
*/

bool LLMgr::ListPointBottom(void)
{
/*
 *----------------------------------------------------------------
 * Indicate the last command to be executed for dubugging
 *----------------------------------------------------------------
*/
     InitStatus(  LL_FILELINE, LL_pBOTTOM );

/*
 *-----------------------------------------------------------------
 * 1) Current pointer to bottom
 * 2) Set user pointer to the correct off set
 *----------------------------------------------------------------
*/
    pListCurrent = pListBottom;

    pUserCurrentElement = (char *)pListCurrent + sizeof( ListPointers_t);

    return  true;
}

/*
 *--------------------------------------------------------------------
 *  ListPointNext will increment the current pointer to the
 *    next linked lis entry unless the nex entry is null
 *    in which cans it will not move and the return code
 *    of fail will be returned.
 *-------------------------------------------------------------------
*/
bool LLMgr::ListPointNext(void)
{
     ListPointers_t  *pCurrentPointers;
/*
 *----------------------------------------------------------------
 * Indicate the last command to be executed for dubugging
 *----------------------------------------------------------------
*/
     InitStatus(  LL_FILELINE, LL_pNEXT );

/*
 *------------------------------------------------------------------
 * Mask the forward and backward pointers on the current entry
 *   Then check to be sure it is not the end of list.  When at
 *    the end, indicate the status and return fail.  If not then
 *    increment the pointer to the last entry.
 *-----------------------------------------------------------------
*/
    pCurrentPointers = ( ListPointers_t *)pListCurrent;

    if (pCurrentPointers->pFwd == NULL)
    {
        SetStatusFail(  LL_FILELINE, LL_STATUS_LISTEND, LL_pNEXT  );
        return  false;
    }

    pListCurrent = pCurrentPointers->pFwd;
/*-----------------------------------------------------------------
 * we are at the entry in the list so Copy it to the buffer
 *      after we Updt the last referenced command
 *----------------------------------------------------------------*/

    pUserCurrentElement = (char *)pListCurrent + sizeof( ListPointers_t);

    return  true;
}

/*
 *---------------------------------------------------------------
 * Point backwards thru the list.  When you reach the
 *   begining then return fail if they read past the
 *   end of the list
 *---------------------------------------------------------------
*/
bool LLMgr::ListPointLast(void)
{
     ListPointers_t  *pCurrentPointers;
/*
 *----------------------------------------------------------------
 * Indicate the last command to be executed for dubugging
 *----------------------------------------------------------------
*/
     InitStatus(  LL_FILELINE, LL_pLAST );

/*
 *------------------------------------------------------------------
 * Mask the forward and backward pointers on the current entry
 *   Then check to be sure it is not the begining of the list.  When at
 *    the begining, indicate the status and return fail.  If not then
 *    increment the pointer to the last entry.
 *-----------------------------------------------------------------
*/
    pCurrentPointers = ( ListPointers_t *)pListCurrent;

    if (pCurrentPointers->pBwd == NULL)
    {
         SetStatusFail(  LL_FILELINE, LL_STATUS_LISTEND, LL_pLAST );
        return  false;
    }

    pListCurrent = pCurrentPointers->pBwd;

/*-----------------------------------------------------------------
 * we are at the entry in the list so Copy it to the buffer
 *      after we Update the last referenced command
 *-----------------------------------------------------------------
*/
    pUserCurrentElement = (char *)pListCurrent + sizeof( ListPointers_t);

    return  true;
}

/*
    SetStatusFail
     - File name - from __FILE__, __LINE__
     - Line number of the calling routine
     - Enumerated status code from a function that calls the method
     - The requested command being executed

     All fields are added to the control status control block and can be retrived
            with GetStatus()
     The routine will fill in the message body for the status array.
*/

bool  LLMgr::SetStatusFail(const char file[], long line, long status, long command)
 {
     Status.ReturnCode  =   false;
     Status.Command     =   0;
     Status.LineNo      =   line;
     Status.FileName    =   file;
     Status.Smessage    =   "NOT SET";
     Status.Scommand    =   "NotSet";

 //
 //  Strip the directory from the file - It will need to change for linux system
 //
    size_t lastSlashPos = Status.FileName.find_last_of("/\\");

    std::string fileName = (lastSlashPos == std::string::npos) ? Status.FileName : Status.FileName.substr(lastSlashPos + 1);
//
// Set the status block file name and command
//
    Status.FileName = fileName;
    Status.Command = command;
//
// Locate the command in the table and load to status block
//      Need to determine the number of rows int the array
//
    Status.Scommand.clear();
    int ArrayRows = sizeof(LL_CommandArray) / sizeof(LL_CommandArray[0]);

    for (int i = 0; i< ArrayRows; ++i)
     {
       if (LL_CommandArray[i].CommandValue == Status.Command)
       {
           Status.Scommand = LL_CommandArray[i].CommandName;
            break;
       }
     }
//
// Find the status message in the table and load to status block
//
     ArrayRows = sizeof(LL_StatusArray) / sizeof(LL_StatusArray[0]);

     for (int i = 0; i < ArrayRows; ++i)
     {

         if (LL_StatusArray[i].StatusValue == status)
           {
               Status.Smessage = LL_StatusArray[i].StatusName;
               break;
           }
     }

      return true;

 }
/*
*
* InitStatus() clears the status control block and sets it to True values and the last command passed.
*
*/
bool  LLMgr::InitStatus(const char file[], long line, long command)
 {
     // add code here
     Status.Command     =  command;           // Command that requested
     Status.LineNo      =  line;              // Source code line where the call was made
     Status.FileName    =  file;              // Source code file name
     Status.Scommand    =  " NOT SET ";       // String representation of command - changed below   
     Status.ReturnCode   =   true;            // Assume nothing goes wrong
     Status.Smessage     = "** TRUE - NO MESSSAGE PROVIDED **";  // When true - default message

 //
 //  This will strip the directory from the file - It will need to change for linux system
 //
    size_t lastSlashPos = Status.FileName.find_last_of("/\\");

    std::string fileName = (lastSlashPos == std::string::npos) ? Status.FileName : Status.FileName.substr(lastSlashPos + 1);

    Status.FileName = fileName;         // Set status block file name


// Search the table for the character representation of the command and load status block set uo file and line
//
    Status.Scommand.clear();
    int ArrayRows = sizeof(LL_CommandArray) / sizeof(LL_CommandArray[0]);

    for (int i = 0; i< ArrayRows; ++i)
     {
       if (LL_CommandArray[i].CommandValue == Status.Command)
       {
           Status.Scommand = LL_CommandArray[i].CommandName;
        break;
       }
     }

     return true;
 }

/*
*   Returns the status block
*/

StatusBlock_t LLMgr::GetStatus()
{
    return Status;

}
//
//      GetDirectToken() will load the current element address, random number (generated at element malloc()) and insert the
//       Magic number - My birth year into the return token block.
//
DirectToken_t LLMgr::GetDirectToken()
{
    InitStatus(LL_FILELINE, LL_GETDIRECTTOKEN);
    ReturnToken.Magic = 0;

//
//   Check the list is not  empty - Can't load address that do not exist
//
    if (ListElementCount == 0)
         {
            SetStatusFail(LL_FILELINE, LL_STATUS_LISTEMPTY, LL_GETDIRECTTOKEN);
            return ReturnToken;
         }
 
    ReturnToken.RNumber = ((ListPointers_t *) pListCurrent)->Random;    // Random Number from the  current element in the list
    ReturnToken.Address = ((ListPointers_t *) pListCurrent)->Address;   // Stored direct address
    ReturnToken.Magic = 1955;                                           // Validation so we prevent a memory crash on the SetDirectPointer
    
    return ReturnToken;                // Return the address of the token
}
/*----------------------------------------------------------------------------------------------------
*    When SetDirectPointer(DirectToken_t) is called the current pointer is
*       set to the address passed in the TOKEN. 
*       A GetDirectToken() shouuld have been done to get a valid token.
*       Magic is check to be sure there was GetDirectToken() then the address and random number are
*         checked before setting the pointer.
*   NOTE! If a valid token was obtained and then the element was deleted, this condition 
*           cannot be detected. You will have a memory reference crash.
*------------------------------------------------------------------------------------------------------
*/
bool  LLMgr::SetDirectPointer(DirectToken_t token) 
{
 /*
  *----------------------------------------------------------------
  * Indicate the last command to be executed for dubugging
  *----------------------------------------------------------------
 */
    InitStatus(LL_FILELINE, LL_SETDIRECTPOINTER);

// Now validate the token came from a get operation

    if   (token.Magic != 1955)
      { 
        SetStatusFail(LL_FILELINE, LL_STATUS_INVALIDMAGICTOKEN, LL_SETDIRECTPOINTER);
        return false;
      }

      void *pPassedElement = nullptr;                // Need a pointer to the element
      pPassedElement = token.Address;                // Get the address in the passsed in TOKEN
       
     if (token.Address == ((ListPointers_t *) pPassedElement)->Address && token.RNumber == ((ListPointers_t*)pPassedElement)->Random)
        {                                                       
         pUserCurrentElement = (char*)pPassedElement + sizeof(ListPointers_t);
         pListCurrent = pPassedElement;         // Pointer set to element in the TOKEN
        }
     else
       {
         SetStatusFail(LL_FILELINE, LL_STATUS_INVALIDADDRESS, LL_SETDIRECTPOINTER);
         return false;
       }

return true;
}

 
