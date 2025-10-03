// LinkedList.cpp : This file contains the 'main' function. Program execution FOR TESTS
//      THis version uses pointers to the class function and the "new" operator
//

#include <iostream>
#include "LLMgr.h"
#include <string>

using namespace std;

void PrintStatusBlock(LLMgr*,std::string, int, std::string);                // Prototype for print routine at the bottom

int main()
{
    std::cout << "\nHello world!\n\n";

    LLMgr* pTestLLM;

    pTestLLM = new LLMgr();

   //LLMgr  Test;                         //  non pointer version

    //
    // Used for adding list elements
    //
    CommandTable_t test_element;
    test_element.CommandValue = -5;
    test_element.CommandName = "INITIAL Declarations Test for LLMGR";


    std::cout << "\n *** MAIN TEST PGM ***\n";

    std::cout << "\n\n***************************  BEGIN Registration TEST *****************************\n";

    //
    //  Test registration - passing 0 length
    //

    if (pTestLLM->ListRegister(0, std::string("Test Link List")) == false)
    {
        PrintStatusBlock(pTestLLM,__FILE__, __LINE__, "TEST SUCCESS  - Registration fails TEST - length 0");
    }
    //
    //  Test registration - passing greater then 8192
    //

    if (pTestLLM->ListRegister(9000, std::string("Test Link List")) == false)
    {
        PrintStatusBlock(pTestLLM,__FILE__, __LINE__,  "TEST SUCCESS - Registration fails TEST - length greater than 8192");
    }
    //
    //  Test registration - passing a structure of size
    //

    if (pTestLLM->ListRegister(sizeof(CommandTable_t), std::string("Test Link List")) == true)
    {
        PrintStatusBlock(pTestLLM,__FILE__, __LINE__, "TEST SUCCESS - Registration success");
    }

    //
    //  Test registration - Try to re-register - it should fail
    //

    if (pTestLLM->ListRegister(sizeof(CommandTable_t), std::string("Test Link List")) == false)
    {
        PrintStatusBlock(pTestLLM, __FILE__, __LINE__, " TEST SUCCESS - Duplicate registration - failure test");
    }
    //
    //  Test deregister an empty list
    //

    if (pTestLLM->ListDeregister() == true)
    {
        PrintStatusBlock(pTestLLM,__FILE__, __LINE__,  "TEST SUCCESS - DEREGISTER Contains no element");
    }
    else {
        PrintStatusBlock(pTestLLM, __FILE__, __LINE__,"TEST FAILED - DEREGISTER Contains element");
    }
    //******************************************************************************
    //  Test registration,  add element, deregister with element which is not allowed
    //********************************************************************************

    if (pTestLLM->ListRegister(sizeof(CommandTable_t), std::string("Test Link List")) == true)
    {
        PrintStatusBlock(pTestLLM,__FILE__, __LINE__, " TEST SUCCESS - registration successful");
    }
//
//  Copy the element into the user area
//
    memcpy(pTestLLM->pUserAddBuffer, &test_element, sizeof(CommandTable_t));
//
//  Now  add element to the list
//
    if (pTestLLM->ListAddEnd() == true)
    {
        PrintStatusBlock(pTestLLM,__FILE__, __LINE__, "TEST SUCCESS - element added");
    }
    else {
        PrintStatusBlock(pTestLLM,__FILE__, __LINE__,  "TEST FAILED - Element not added");
    }

 //
 //  Try to de-register and fail
 //

    if (pTestLLM->ListDeregister() == false)
    {
        PrintStatusBlock(pTestLLM,__FILE__, __LINE__, "TEST SUCCESS - DEREGISTER Can't contain an element ");
    }
    else {
        PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "TEST FAILED - DEREGISTER Contains no element");
    }
 //
 //  Remove the initial list element added.  It is the only element and should be the current element
 //
    if (pTestLLM->ListDelete() == true)
        {
            PrintStatusBlock(pTestLLM, __FILE__, __LINE__,"TEST SUCCESS - element deleted");
        }
    else {
        PrintStatusBlock(pTestLLM,__FILE__, __LINE__, "TEST FAILED - Element not deleted");
    }

    //
    //
    //**************************************** END REGISTRATION TESTS *****************************
    //
    //******************************************************************************
    //  We will add 10 elements then print get them back for printing
    //       - List is still registered from above
    //********************************************************************************
    //
    std::cout << "\n\n***************************  BEGIN LOAD LIST TEST *****************************\n";

    std::string msg("TEST Element # - ");

    for (int i = 0; i < 10; ++i)
    {
        msg += std::to_string(i);

        test_element.CommandValue = i;
        test_element.CommandName = msg;

        std::cout << "\n *** SHOW - LOAD LIST - LIST element COMMAND name: " << test_element.CommandName 
            << "  LIST element COMMAND Value  = " << test_element.CommandValue;

        memcpy(pTestLLM->pUserAddBuffer, &test_element, sizeof(CommandTable_t));

        if (pTestLLM->ListAddEnd() == true)
        {
            //PrintStatusBlock(pTestLLM,__FILE__, __LINE__, "TEST SUCCESS - element added");
        }
        else {
            PrintStatusBlock(pTestLLM,__FILE__, __LINE__, "TEST FAILED - Element not added FOR LOOP TEST");
        }
    }
    //
    // Point to top of list, then loop through until reaching the end - Assure the count is correct (ELEMENT COUNT)
    //

    std::cout << "\n\n***************************  BEGIN Print LIST AND ELEMENT COUNT TEST *****************************\n";

    if (pTestLLM->ListPointTop() == false)
    {
        PrintStatusBlock(pTestLLM,__FILE__, __LINE__, "TEST FAILED - Point TOP not working\n");
        return false;
    }
    else {
        std::cout << "\nTEST SUCCESS -TOP OF LIST POINTED \n";
    }

    for (int i = 0; i <= pTestLLM->ElementCount; ++i)
    {
        memcpy(&test_element, pTestLLM->pUserCurrentElement, sizeof(CommandTable_t));

        //
        std::cout << "\nSHOW - PRINT LIST - LIST Name element: " << test_element.CommandName <<
            "   LIST Value element = " << test_element.CommandValue << "  Value of i = " << i;

        if (pTestLLM->ListPointNext() == false)
        {
            PrintStatusBlock(pTestLLM,__FILE__, __LINE__, "TEST SUCCESS  - END OF ELEMENT LIST FOUND\n");
            break;
        }
    }


    std::cout << "\n\n***************************  BEGIN DIRECT POINTING TEST *****************************\n";

    DirectToken_t  testtoken = {nullptr, 0, 0};

    testtoken.RNumber = 0;
    testtoken.Address = nullptr;


        if (pTestLLM->SetDirectPointer(testtoken) == false)
            {
            std::cout <<" \nTEST SUCCESS - Invalid TOKEN passed - Could not verify token";
            PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "TEST SUCCESS - Expeceted invalid token passed");
               }

        testtoken = pTestLLM->GetDirectToken();

        if (testtoken.Magic == 0)
           {
            std::cout << " TEST Failed - TOKEN MAGIC RETURNED -  0 -  Could not verify token\n";
            PrintStatusBlock(pTestLLM,__FILE__, __LINE__, "TEST FAILED - NO elements in the list");
             }
        else
        {
            PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "TEST SUCCESS  - GOT THE TOKEN!");

        }
/*----------------------------------------------------------------------------------------------
*   Point to top of the list.
*   Get the token for the 5 element
*   Then print the 5 element at the end o the list again.
* 
* ---------------------------------------------------------------------------------------------
*/
        if (pTestLLM->ListPointTop() == false)
        {
            PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "TEST FAILED - Point TOP not working\n");
            return false;
        }
        else {
            std::cout << "\nTEST SUCCESS -TOP OF LIST POINTED \n";
        }
        
        int ct = 0;

        for (int i = 0; i < 10; ++i)
        {
            memcpy(&test_element, pTestLLM->pUserCurrentElement, sizeof(CommandTable_t));

            std::cout << "\nSHOW - PRINT LIST - LIST Name element: " << test_element.CommandName <<
                "   LIST Value element = " << test_element.CommandValue << "  Value of i = " << i;
            if (ct == 5)
            {
                testtoken = pTestLLM->GetDirectToken();
                if (testtoken.Magic == 0)
                 {
                    std::cout << " TEST Failed - TOKEN MAGIC RETURNED False -  Could not verify token\n";
                    PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "TEST FAILED - NO elements in the list");
                    break;
                }
                else
                {
                    PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "TEST SUCCESS  - GOT THE TOKEN");
                }
            }

            if (pTestLLM->ListPointNext() == false)
            {
                PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "TEST SUCCESS  - END OF ELEMENT LIST FOUND");
            }
        ct++;
        }
       

        if (pTestLLM->SetDirectPointer(testtoken) == true)

           {
            memcpy(&test_element, pTestLLM->pUserCurrentElement, sizeof(CommandTable_t));
            std::cout << "\n\nSHOW - SUCCESS REPRINT CAPTURED ELEMENT - LIST Name element: " << test_element.CommandName <<
               "   LIST Value element = " << test_element.CommandValue;
           }
        else
          {
            PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "TEST FAILED - Did not expect invalid token");
          }

        std::cout << "\n\n*************************** END DIRECT POINTING TEST *****************************\n";
        std::cout << "\n\n*************************** BEGIN BACKWARD POINTING TEST *****************************\n";

/*---------------------------------------------------------------------------------------------------------------
*       Print the list in reverse.  
*           Need to point to list bottom and run the pointers in reverse
* ------------------------------------------------------------------------------------------------------------------
*/

     if (pTestLLM->ListPointBottom() == false)
        {
            PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "TEST FAILED - Point BOTTOM  not working\n");
            return false;
        }
        else {
            std::cout << "\nTEST SUCCESS - BOTTOM  OF LIST POINTED \n";
        }


      for (int i = 0; i < 10; ++i)
        {
            memcpy(&test_element, pTestLLM->pUserCurrentElement, sizeof(CommandTable_t));

            std::cout << "\nSHOW - PRINT LIST BACKWARDS - LIST Name element: " << test_element.CommandName <<
                "   LIST Value element = " << test_element.CommandValue << "  Value of i = " << i;

            if (pTestLLM->ListPointLast() == false)
            {
                PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "TEST SUCCESS  - END OF ELEMENT LIST FOUND");
            }
            ct++;
        }
    
  std::cout << "\n\n*************************** END BACKWARD POINTING TEST *****************************\n";
  std::cout << "\n\n*************************** START BEFORE AND AFTER Insertion Tests *****************************\n";
//
//  Do an add before and add after no status block checking - results are printed.
//      Error checking was tested above.  For simplicity, that is skipped here.
//

  CommandTable_t test;    
  test.CommandName.clear();                                                       // New structure
  test.CommandValue = -5;
  test.CommandName = "TEST AFTER";

  pTestLLM->ListPointTop();                                                     // Top of the list
  memcpy(pTestLLM->pUserAddBuffer, &test, sizeof(CommandTable_t));              // Copy to the user buffer
  pTestLLM->ListAddAfter();                                                     // Now is the second element
  
  test.CommandName.clear();                                                     // Need a clean string object
  test.CommandName = "TEST BEFORE";                                             // New message - Prove a different element
  test.CommandValue = -20;                                                      // Change the number
          
  pTestLLM->ListPointBottom();                                                  // GO to bottom of the list
  memcpy(pTestLLM->pUserAddBuffer, &test, sizeof(CommandTable_t));              // Copy structure into the add buffer
  pTestLLM->ListAddBefore();                                                    // Add The element before the end
  

  //
  // Print the list again - Use a do while loop for an example
  //

  pTestLLM->ListPointTop();                                                     // Point to the top to print

  do 
     {
         test.CommandName.clear();                                                     // Need a clean string object

         memcpy(&test_element, pTestLLM->pUserCurrentElement, sizeof(CommandTable_t));
        
         std::cout << "\nSHOW - PRINT LIST Forward - LIST Name element: " << test_element.CommandName <<
              "   LIST Value element = " << test_element.CommandValue << "  No counter";
   
     } while (pTestLLM->ListPointNext() == true);

  std::cout << "\n\n*************************** END OF INSERTION TESTS BEFORE AND AFTER *****************************\n";

  std::cout << "\n\n*************************** EMPTY LIST and DEREGISTER TEST *****************************\n";

    if (pTestLLM->ListDeleteAll() == false)
        {
             PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "TEST Failed  - Not able to delete all the elements");
        }
    else 
        {
            PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "TEST SUCCESS  - All elements free\n");
       }

    if (pTestLLM->ListDeregister() == true)                             //  No more tests, so look at the status block

     { std::cout << "\nTEST SUCCESS - Deregister Successful \n";
     }
     else
        { 
            PrintStatusBlock(pTestLLM, __FILE__, __LINE__, "\nTEST FAILED  - LAST STATUS BLOCK\n");
           std::cout  << " \nELEMENT COUNT = "  << pTestLLM->ElementCount;
       
        }

//((CommandTable_p)pTestLLM->pUserAddBuffer)->CommandName = "Test";

    std::cout << "\n\n*************************** END EMPTY LIST and DEREGISTER TEST *****************************\n";

    std::cout << "\n  END OF TEST - Goodby world!\n\n" << endl;

    return true;
}

/*
* Print routine for status block information in the test program
*/

void PrintStatusBlock(LLMgr* pTestLLM, std::string file,int lineno, std::string test)
{
    StatusBlock_t Status = pTestLLM->GetStatus();
    //
     //  Strip the directory from the file - It will need to change for linux system
     //
    size_t lastSlashPos = file.find_last_of("/\\");

    std::string fileName = (lastSlashPos == std::string::npos) ? file : file.substr(lastSlashPos + 1);
    //

    std::cout << "\n\n *** Test ***  " << test << "\n   TEST File Name: " << fileName  << "  LINE #: " << lineno
         << "  ***\n" << "   List Name: " <<  Status.Slistname <<
        "\n   LLLMgr Source File Name: " << Status.FileName   << "  LLMgr Source Line Number: "
        << Status.LineNo   << "\n   ReturnCode: " << Status.ReturnCode
        << "   Command #: " << Status.Command << "\n   Command Name: " << Status.Scommand
        << "\n   Status Message: " << Status.Smessage << "\n"; return;
}
