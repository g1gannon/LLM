/**--------------------------------------------------------------
 * File: LLMgr.h
 *
 *  Author: Gregory Gannon
 * Created: 09/26/1995
 * Status : COMPLETE 
 *---------------------------------------------------------------------
 * HISTORY
 *
 *	 Date       Author      Change Description
 *   ----		------		------------------
 * 9/26/1995     GMG        Developed the class
 * 12/02/1995    GMG        Changed the class to use commands
 * 03/24/1996    EGC        Added direct memory addressing.
 * 08/09/2025    GMG        Split command and status table  clean up definitions for 64 bit addresses, 
 *                           elminated commands and updated direct addressing
 *
 * PURPOSE
 *    This file contains the header files required for the Linked list manager class
 *    class.  It is used to create a linked list of size 1-8192.
 *
 *.
 *----------------------------------------------------------------------
*/

#define  LL_FILELINE  __FILE__, __LINE__        // C++ preprocessor lines and file name
#include <cstdint>
#include <string.h>
/* 
 *----------------------------------------------------------------------
 * Defines the typedef for the status message array used for the
 *      self diagnosing.
 *--------------------------------------------------------------------
*/
typedef struct {
    long   StatusValue;
    std::string  StatusName;
}  StatusTable_t;
/*
 *----------------------------------------------------------------------
 * Defines the typedef for the command message array used for the
 *      self diagnosing.
 *--------------------------------------------------------------------
*/

typedef struct {
    long   CommandValue;
    std::string CommandName;
}  CommandTable_t;

/*
 *----------------------------------------------------------------------
 * Defines the typedef for the status control block
 *--------------------------------------------------------------------
*/
typedef struct {
    bool        ReturnCode;           // Return either True of false
    long        Command ;             // Enumerated command number
    long        LineNo  ;             // Line in source file the item was detected
    std::string  FileName;            // Name of source file where error was detected
    std::string  Smessage;            // String version of error message
    std::string  Scommand;            // String version of command
    std::string  Slistname;           // Name of the linked list   
}  StatusBlock_t;   
/**-----------------------------------------------------------------------------------------------
 * Defines the typedef for the linked list forward, backward and direct access pointers
 * Magic is a number to assure we get the entry should the memory address not be the same.
 *      When a malloc() is executed, the random number and current pointer to the memory location
 *      are stored in the ListPointers_t part of the memory location. 
 *      This can be used by a subsystem to get the token.  With the token, the LLMgr can
 *      move the current pointer to the requested entry without searching the list.     
 *      This was used by the sockets subsystem by adding it to the Request/Response header
 *      of a message.  The returned message could directly address the socket control block; 
 *--------------------------------------------------------------------------------------------------
*/
typedef struct {
    void *pFwd;                           /// Forward memory pointer
    void *pBwd;                           /// Backward memory pointer
    void *Address;                        /// Memory address of this Linked list element on creation 
    time_t  Random;                       ///  Random number generated when area was malloc()ed
}  ListPointers_t;

//
// This structure is returned on the GetDirectToken() and is built from the current element pointed to in
//      linked list.  When SetDirectPointer(DirectToken_t) is passed the current pointer is
//      set to the address passed back. Then the magic,address and random number are checked before setting the pointer
//      The Magic number is used to verify a valid GetDirectToken() was called.  
// 
//   !! NOTE!! - A valid GetDirectToken() stored off and then a delete of that element will cause a memory crash. 
//
typedef struct {
    void    *Address;         /// Address of element returned on GetDirectToken()
    time_t    RNumber;        /// Used to validate we got to the right element
    int       Magic;          ///  GetDirectToken() will set this to - my birth year
} DirectToken_t;
/*
 *------------------------------------------------------------------------------------
 * Defines the linked list methods messages for creating and updating list elements
 *------------------------------------------------------------------------------------
*/
enum  LL
{
      LL_ADDEND = 100,
      LL_ADDAFTER,
      LL_ADDBEFORE,
      LL_DELETE,
      LL_DELETE_ALL,
      LL_SETDIRECTPOINTER,
      LL_GETDIRECTTOKEN,
      LL_pNEXT,
      LL_pLAST,
      LL_pBOTTOM,
      LL_pTOP,
	  LL_REGISTER,
      LL_DEREGISTER,
};
  /*
The enum start at 0 so they can be used as an index into the message array
*/

enum  LL_STATUS {
      LL_STATUS_INVALIDSIZE = 0,
      LL_STATUS_LISTEMPTY,
      LL_STATUS_LISTEND,
      LL_STATUS_ALLOCFAIL,
      LL_STATUS_NOTEMPTY,
      LL_STATUS_ALREADYREGISTERED,
	  LL_STATUS_NOTREGISTERED,
	  LL_STATUS_INVALIDADDRESS,
	  LL_STATUS_INVALIDMAGICTOKEN,
};


class  LLMgr
{
  protected:

	void        *pListCurrent;                                      /// Current element in the list with pointers
	void        *pListTop;                                          /// First list element
    void        *pListBottom;                                       /// Last list element
    void        *pClassBuffer;                                      /// Used to allocate storage on an add request
    long        ListElementCount;                                   /// Number of items in the list
   /long        ListUserElementLength;                              /// User requested length at registration
    bool        ListRegistered;                                     /// Indicate list is registered



//
//  Internally used class functions
//
     bool  InitStatus(const char arr[], long, long );               // Initialize the status block for success
//                      Sourcw File Name, Line Number,  enumerated method 
     bool  SetStatusFail(const char arr[], long, long, long);       /// Set the status block to failure with reasons
//                      Sourcw File Name, Line Number,  enumerated status, enumerated method  

   public:
      long          ElementCount;                                    /// Number of elements in the list
      void          * pUserCurrentElement;                           /// Point to user area of current element in the list
      void          * pUserAddBuffer;                                /// Point to the RAW add buffer for the next element to be added
 //   Methods to manipulate the list                                                
      bool          ListPointNext(void);                             /// Point to the next in the list
      bool          ListPointTop(void);                              /// Point to the top of the list
      bool          ListPointBottom(void);                           /// Point to the bottom of the list
      bool          ListPointLast(void);                             /// Point back one
      bool          ListAddEnd(void);                                /// Adding to end of the list
      bool          ListAddBefore(void);                             /// Add Before the current element
      bool          ListAddAfter(void);                              /// Add After the current element
      bool          ListRegister(long int, std::string );            /// Registration - User buffer size and list name
      bool          ListDeregister(void);                            /// Deregistration - Must be empty
      bool          ListDelete(void);                                /// Delete current entry in the list
      bool          ListDeleteAll(void);                             /// Delete all elements in the list - used to deregister
      StatusBlock_t  GetStatus(void);                                /// Returns the status block with all information on last operation
      DirectToken_t GetDirectToken();                                /// Returns a token for direct pointing can be used in messages
      bool          SetDirectPointer(DirectToken_t);                 /// Uses the token to point directly without searching the list
                    LLMgr();                                         /// Constructor no parameters
                    ~LLMgr();                                        /// Destructor no parameters

};
