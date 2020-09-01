// Class implementing a string according to the ideas of Stanislav Jurny.
// Copyright (c) 2020 Stanislav Jurny (github.com/STjurny) licence MIT

#pragma once
#ifndef _PracticString_
#define _PracticString_

#include <memory>

#pragma warning(disable : 26812)  // Turn off MSVC warning C26812 about class enum

namespace Practic {


// The value returned by indexOf methods if searched string or character is not found.
enum { notFound = -1 };


// Mode of comparing strings.
enum EqualityMode {
    caseSensitive,
    caseInsensitive
};


// Mode of converting strings.
enum LetterCase {
    upperCase,
    lowerCase
};


// Condition used by character testing methods.
// It is used by methods for finding and checking characters in the string.
enum CharTestCondition
{
    // Only characters contained in passed string.
    containedIn,

    // Only characters that is not contained in passed string.
    notContainedIn
};
            

// Pointer to function for testing characters. It is used by methods for finding and checking characters in the string.
// Definition of function type corresponds to standard functions like isaplha, isdigit, isspace, and so on.
// The tested character is passed as int parameter. Non zero result is considered as fullfilment the condition.
typedef int (*CharTestFunction)(int);


// Class that keeps parameters of parsing and the current index in parsed string.
struct ParsingContext;


// Special descendant of String used as accumulator for joining string using operator "+". *Never* use this class directly. 
// Using this class for joining is kind of hack, but reduce count of memory allocations and thus possible heap fragmentation.
// Result of the joinning of strings must to be assigned to a variable with explicitly expressed type String 
// don't use "auto" or type _StringJoiningResult for declaration of target variable otherwise bad things will happen.
class _StringJoiningResult;


struct _Allocation;


// Class implementing the string of characters.
class String
{
    // Constructors & Destructor
    public: 
        // Creates empty string.
        String();

        // Creates a string containing one character aChar.
        // Empty string is created if aChar is null terminatig character `\0'.
        String(char aChar);

        // Creates a string object from standard null-terminated C string aCString.
        // If parameter aCStringIsLiteral is false (default) then object use own buffer and copies aCString to him.
        // If aCStringIsEmpty is true then considers aCString as permanently allocated C string (e.g. C string literal) which exists all the time of object existence.
        // For literal string is not allocated buffer, object only holds reference to literal (which saves memory and time especially for longer strings).
        // Use macro "ls" for easy create string object from C string literal. 
        // Parameter aCString can be NULL then it creates null string object (it can be tested using isNull() method).
        String(const char * aCString, bool aCStringIsLiteral = false);

        // Macro for easy conversion C string literal to string object.
        // See description of String constructor with parameter aCStringIsLiteral for details.
        // Use `String s = ls("Hello, world!!")'
        #define ls(aStringLiteral) String(aStringLiteral, true)

        // Creates a string object from aCString with limitation of length.
        // This constructor is suitable for creating string known length from array of characters which is not terminated by null character.
        // Resulting length of string is length to the first terminating null character or aMaxLength whichever comes first.
        // Parameter aCString can be NULL then it creates null string object (it can be tested using isNull() method).
        String(const char * aCString, int aMaxLength);

        // Copy constructor. Creates copy of aString.
        // Uses copy-on-write approach so only reference to the string buffer is copied. 
        // Actual copy of buffer is created only when the string is changed.
        String(const String & aString);

        // Destructor
        ~String();


    // Typed NULL
    public:
        // NULL string for assigning or testing equality.            
        static const String null;

        // Empty string.
        static const String empty;


    // Formating String
    public:
        // Returns string builded from aFormat and variadic arguments using same rules as standard printf function.
        // If aFormat is NULL returns NULL string.
        static String formatted(const char * aFormat, ...);

        // Returns string builded from aFormat and anArguments using same rules as standard vprintf function.
        // If aFormat is NULL returns NULL string.
        static String formattedList(const char * aFormat, va_list anArguments);


    // String of Characters
    public:
        // Returns string containing aCount characters aChar.
        // It allows fill buffer of the newly created string with terminating character '\0'.
        static String ofChar(char aChar, int aCount = 1);


    // Basic Properties
    public: 
        // Returns true when the string is NULL (for empty or non-empty string returns false).
        bool isNull() const;

        // Returns true when the string is empty (for NULL or non-empty string returns false).
        bool isEmpty() const;

        // Returns length of string in number of characters (without terminating null character).
        // For empty or NULL string returns 0.
        int length() const;


    // Managing Capacity
    public: 
        // Returns string with allocated buffer for string with maximal length aRequiredCapacity.
        // It adds one byte for null terminating character so for aRequiredCapacity = 20 is allocated memory for 21 characters.
        // Actual allocated capacity is always greater or equal to innerCapacity which is capacity of string self-contained buffer.
        static String withCapacity(int aRequiredCapacity);

        // Maximal length of the contained string (1073741823 = 1GiB - 1).
        //static inline int maxCapacity { return 0x03FFFFFFF; /* uint32 without two most significant bytes used for Mode of string */ };
        static const int maxCapacity = 0x03FFFFFFF; 

        // Capacity of internal buffer used for short length strings (11 bytes)
        // Strings with length less or equals inner capacity are placed in to internal buffer and not to heap.
        //static inline int innerCapacity { return 11; /* sizeof(Data::asCharsBuffer) - 1 */ }
        static const int innerCapacity = 11;

        // Returns current capacity of string buffer in number of characterss (without terminator character).
        // Buffer capacity can be changed by methods wb, reserveCapacity and minimizeCapacity.
        int capacity() const;
            
        // Allocates memory for string with maximal length aRequiredCapacity.
        // It adds one byte for null terminating character so for aRequiredCapacity = 20 is allocated memory for 21 characters.
        // If aCopyOriginal is true (default) it keeps current string (copy it when it must to do reallocation) otherwise it sets string to empty.
        // Parameter anAllowShrink allows reduce size of allocated memory when aRequiredCapacity is less than current capacity. 
        // Reducing of already allocated memory require reallocation and thus possible greater fragmentation of heap (hence anAllowShrink is false in default).
        // If aCopyOriginal is true then resulting capacity is always set to at least length of current string even if aRequiredCapacity is less and anAllowShrink is true.
        // Resulted capacity is never smaller than size of internal buffer for short strings (usually 11 bytes, see method innerCapacity).
        void reserveCapacity(int aRequiredCapacity, bool aCopyOriginal = true, bool anAllowShrink = false);

        // Reduce capacity to length of current string.
        // If capacity is 100 and length of string is 50 characters then reduce capacity to 50 characters and release remaining memory.
        // Consider that reducing of already allocated memory require reallocation and thus possible greater fragmentation of heap.
        // Capacity is reduced only when buffer is not referenced from other string instance (method references() returns 1).
        void minimizeCapacity();

        // Returns number of instances that referencing the buffer allocated on heap.
        // If the buffer is not allocated on heap returns -1. The feature is intended mainly for debugging purposes.
        int references() const;


    // Accesing buffer
    public: 
        // Returns pointer to the read only buffer containing the string in native C format. Pointer can be passed to functions that require null terminated string (e.g. printf).
        // Content of this buffer must *not* be changed because it can be shared with another instances of string. For access to writable buffer use method "wb".
        // Pointer to buffer is valid until the call of some modifying (non const) method on string or until end of scope of string variable. 
        const char * rb() const;

        // The value passed to the wb method if the allocated capacity does not change.
        enum { unchanged = -1 };

        // Returns pointer to writable buffer containing the string in native C format. Pointer can be passed to functions that require null terminated string (e.g. scanf).
        // Content of this buffer can be freely changed but it must be always terminated by null character. 
        // Pointer to buffer is valid only until it is called any other string method except basic properties (length(), capacity(), isEmptyOrNull(), isNull()).
        // This method also allows change capacity of the buffer same way as reserveCapacity method. 
        // Parameter aRequiredCapacity determines size of buffer in count of characters. To keep current capacity set aRequiredCapacity to "unchanged" (default).
        // Method adds one byte for null terminating character (so for aRequiredCapacity = 20 is allocated memory for 21 characters).
        // If aCopyOriginal is true (default) it keeps current string (copy it when it must to do reallocation) otherwise it sets string to empty.
        // Parameter anAllowShrink allows reduce size of allocated memory when aRequiredCapacity is less than current capacity. 
        // Reducing of already allocated memory require reallocation and thus possible greater fragmentation of heap (hence anAllowShrink is false in default).
        // If aCopyOriginal is true then resulting capacity is always set to at least length of current string even if aRequiredCapacity is less and anAllowShrink is true.
        // Resulted capacity is never smaller than size of internal buffer for short strings (usually 11 bytes, see method innerCapacity).
        char * wb(int aRequiredCapacity = unchanged, bool aCopyOriginal = true, bool anAllowShrink = false); 


    // Finding Substring / Character
    public: 
        // Returns index of beginning of aSubstring or notFound if the string does not contain substring in the searched part. 
        // Parameter aMode defines case sensitive (default) or case insensitive searching.
        // Searching starts from aStartIndex which is zero in default. If passed aStartIndex is less than zero searching starts from zero.
        // If aSubstring is empty it returns aStartIndex if is in range of string (including index of terminating '\0' character) or notFound if aStartIndex is out of range.
        // Returns notFound if string itself or substring is NULL.
        inline int indexOf(const String & aSubstring, EqualityMode aMode = caseSensitive, int aStartIndex = 0) const { return indexOf(aSubstring.rb(), aMode, aStartIndex); }

        // Returns index of beginning of aCSubstring or notFound if the string does not contain substring in the searched part. 
        // Parameter aMode defines case sensitive (default) or case insensitive searching.
        // Searching starts from aStartIndex which is zero in default. If passed aStartIndex is less than zero searching starts from zero.
        // If aCSubstring is empty it returns aStartIndex if is in range of string (including index of terminating '\0' character) or notFound if aStartIndex is out of range.
        // Returns notFound if string itself or substring is NULL.
        int indexOf(const char * aCSubstring, EqualityMode aMode = caseSensitive, int aStartIndex = 0) const;

        // Returns index of character aChar in string or notFound if the string does not contain aChar in the searched part. 
        // Parameter aMode defines case sensitive (default) or case insensitive searching.
        // Searching starts from aStartIndex which is zero in default. If passed aStartIndex is less than zero searching starts from zero.
        // Terminating '\0' character is not considered to be a part of the string (can not be found). When the string is empty or NULL returns notFound. 
        int indexOf(char aChar, EqualityMode aMode = caseSensitive, int aStartIndex = 0) const;


        // Returns index of a first found character which is contained (or is not contained) in aChars. If such char is not found returns notFound (-1).
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        // Searching starts from aStartIndex which is zero in default. If aStartIndex is less than zero searching starts from zero.
        // Terminating '\0' character is not considered to be a part of the string (can not be found). When the string is empty or NULL returns notFound. 
        inline int indexOfAnyChar(CharTestCondition aCondition, const String & aChars, int aStartIndex = 0) const { return indexOfAnyChar(aCondition, aChars.rb(), aStartIndex); }

        // Returns index of a first found character which is contained (or not contained) in aChars. If such char is not found returns notFound (-1).
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        // Searching starts from aStartIndex which is zero in default. If aStartIndex is less than zero searching starts from zero.
        // Terminating '\0' character is not considered to be a part of the string (can not be found). When the string is empty or NULL returns notFound. 
        int indexOfAnyChar(CharTestCondition aCondition, const char * aChars, int aStartIndex = 0) const;

        // Returns index of first found character for which aTestFunction returns aTestResult or notFound if string does not contain such character in searched part.
        // Method is designed for using with standard functions isalpha, isdigit, isspace, ... (e.g. indexOfAnyCharWhere(isdigit, true) returns index of first found digit).
        // Searching starts from aStartIndex which is zero in default. If passed aStartIndex is less than zero searching starts from zero.
        // Terminating '\0' character is not considered to be a part of the string (can not be found). When the string is empty or NULL returns notFound. 
        int indexOfAnyCharWhere(CharTestFunction aTestFunction, bool aTestResult, int aStartIndex = 0) const;
            

    // Testing contained substring / character
    public:
        // Returns true if string contains aSubstring. 
        // Parameter aMode defines case sensitive (default) or case insensitive searching.
        // If aSubstring is empty always returns true except the case when string itself is NULL. 
        // If aSubstring is NULL returns true if string itself is also NULL.
        inline bool contains(const String & aSubstring, EqualityMode aMode = caseSensitive) const { return contains(aSubstring.rb(), aMode); }

        // Returns true if string contains aCSubstring. 
        // Parameter aMode defines case sensitive (default) or case insensitive searching.
        // If aCSubstring is empty always returns true except the case when string itself is NULL. 
        // If aCSubstring is NULL returns true if string itself is also NULL.
        inline bool contains(const char * aCSubstring, EqualityMode aMode = caseSensitive) const { return isNull() ? aCSubstring == NULL : indexOf(aCSubstring, aMode) != notFound; }

        // Returns true if string contains character aChar. 
        // Parameter aMode defines case sensitive (default) or case insensitive searching.
        // Terminating '\0' character is not considered to be a part of the string (always returns false for character '\0').
        inline bool contains(char aChar, EqualityMode aMode = caseSensitive) const { return indexOf(aChar, aMode) != notFound; } 


        // Returns true is the string contains at least one characters which is contained (or is not contained) in aChars.
        // Parameter aCondition specifies search for characters which are contained or are not contained in parameter aChars.
        // Terminating '\0' character is not considered to be a part of the string (its position is not tested).
        inline bool containsAnyChar(CharTestCondition aCondition, const String & aChars) const { return containsAnyChar(aCondition, aChars.rb()); }

        // Returns true is the string contains at least one characters which is contained (or is not contained) in aChars.
        // Parameter aCondition specifies search for characters which are contained or are not contained in parameter aChars.
        // Terminating '\0' character is not considered to be a part of the string (its position is not tested).
        inline bool containsAnyChar(CharTestCondition aCondition, const char * aChars) const { return indexOfAnyChar(aCondition, aChars) != notFound; }

        // Returns true if string contains at least one character for which aTestFunction returns aTestResult.
        // Method is designed for using with standard functions isalpha, isdigit, isspace, ... 
        // e.g. containsAnyCharWhere(isdigit, true) returns true if string contains digit.
        // Terminating '\0' character is not considered to be a part of the string (its position is not tested).
        inline bool containsAnyCharWhere(CharTestFunction aTestFunction, bool aTestResult) const { return indexOfAnyCharWhere(aTestFunction, aTestResult) != notFound; }


        // Returns true if the string contains only characters which are contained (or are not contained) in aChars. 
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        // Terminating '\0' character is not considered to be a part of the string (its position is not tested). Always returns false for empty string.
        bool containsOnlyChars(CharTestCondition aCondition, const String & aChars) const { return containsOnlyChars(aCondition, aChars.rb()); }

        // Returns true if the string contains only characters which are contained (or are not contained) in aChars. 
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        // Terminating '\0' character is not considered to be a part of the string (its position is not tested). Always returns false for empty string.
        bool containsOnlyChars(CharTestCondition aCondition, const char * aChars) const;

        // Returns true if string contains only characters for which aTestFunction returns aTestResult but not any other.
        // Method is designed for using with standard functions isalpha, isdigit, isspace, ... 
        // e.g. containsOnlyCharsWhere(isdigit, true) returns true if string contains only digits.
        // Terminating '\0' character is not considered to be a part of the string (its position is not tested). Always returns false for empty string.
        bool containsOnlyCharsWhere(CharTestFunction aTestFunction, bool aTestResult) const;


    // Testing substring / character at index
    public:
        // Returns true if string constains aSubstring starting at anIndex. 
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // If aSubstring is empty it returns true if anIndex is in range of string (including index of terminating '\0' character) or false if anIndex is out of range.
        // Returns false if string itself or substring is NULL.
        inline bool containsAt(int anIndex, const String & aSubstring, EqualityMode aMode = caseSensitive) const { return containsAt(anIndex, aSubstring.rb(), aMode); }

        // Returns true if string constains aCSubstring starting at anIndex. 
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // If aSubstring is empty it returns true if anIndex is in range of string (including index of terminating '\0' character) or false if anIndex is out of range.
        // Returns false if string itself or substring is NULL.
        bool containsAt(int anIndex, const char * aCSubstring, EqualityMode aMode = caseSensitive) const;
            
        // Returns true if string contains character aChar at position anIndex. 
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // When anIndex is out of range of string or string is NULL returns false. 
        // Terminating '\0' character is not considered to be a part of the string (testing its index for character '\0' returns false).
        bool containsAt(int anIndex, char aChar, EqualityMode aMode = caseSensitive) const;


        // Returns true if the string contains at a position anIndex one or more characters which are contained (or are not contained) in aChars.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        // Parameter oLength is setted to count of found characters.
        // Terminating '\0' character is not considered to be a part of the string (its position is not tested).
        inline bool containsCharsAt(int anIndex, CharTestCondition aCondition, const String & aChars, int * oLength) const 
            { return containsCharsAt(anIndex, aCondition, aChars.rb(), oLength); }

        // Returns true if the string contains at a position anIndex one or more characters which are contained (or are not contained) in aChars.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        // Parameter oLength is setted to count of found characters.
        // Terminating '\0' character is not considered to be a part of the string (its position is not tested).
        bool containsCharsAt(int anIndex, CharTestCondition aCondition, const char * aChars, int * oLength) const;

        // Returns true if the string contains from a position anIndex one or more characters for which aTestFunction returns aTestResult. 
        // Parameter oLength is setted to count of found characters.
        // Method is designed for using with standard functions isalpha, isdigit, isspace, ... 
        // Terminating '\0' character is not considered to be a part of the string (its position is not tested).
        bool containsCharsAtWhere(int anIndex, CharTestFunction aTestFunction, bool aTestResult, int * oLength) const;


        // Returns true if the string contains at anIndex any character contained (or not contained) in aChars.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        // Terminating '\0' character is not considered to be a part of the string (its position can not be tested).
        inline bool containsAnyCharAt(int anIndex, CharTestCondition aCondition, const String & aChars) const { return containsAnyCharAt(anIndex, aCondition, aChars.rb()); }

        // Returns true if the string contains at anIndex any character contained (or not contained) in aChars.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        // Terminating '\0' character is not considered to be a part of the string (its position can not be tested).
        bool containsAnyCharAt(int anIndex, CharTestCondition aCondition, const char * aChars) const;

        // Returns true if the string at anIndex contains character for which aTestFunction returns aTestResult.
        // Method is designed for using with standard functions isalpha, isdigit, isspace, ... 
        // Terminating '\0' character is not considered to be a part of the string (its position can not be tested).
        bool containsAnyCharAtWhere(int anIndex, CharTestFunction aTestFunction, bool aTestResult) const;


    // Testing Prefix / Suffix
    public: 
        // Returns true if string starts with aSubstring. 
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // If aSubstring is empty always returns true except the case when string itself is NULL. 
        // Returns false if string itself or substring is NULL.
        inline bool hasPrefix(const String & aSubstring, EqualityMode aMode = caseSensitive) const { return containsAt(0, aSubstring, aMode); }

        // Returns true if string starts with aSubstring. 
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // If aCSubstring is empty always returns true except the case when string itself is NULL. 
        // Returns false if string itself or substring is NULL.
        inline bool hasPrefix(const char * aCSubstring, EqualityMode aMode = caseSensitive) const { return containsAt(0, aCSubstring, aMode); }

        // Returns true if the first character of the string is aChar. 
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // If the string is empty or NULL returns false. Terminating '\0' character is not considered to be a part of the string.
        inline bool hasPrefix(char aChar, EqualityMode aMode = caseSensitive) const { return containsAt(0, aChar, aMode); }


        // Returns true if the string ending with aSubstring. 
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // If aSubstring is empty always returns true except the case when string itself is NULL. 
        // Returns false if string itself or substring is NULL.
        inline bool hasSuffix(const String & aSubstring, EqualityMode aMode = caseSensitive) const { return hasSuffix(aSubstring.rb(), aMode); }

        // Returns true if the string ending with aCSubstring. 
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // If aCSubstring is empty always returns true except the case when string itself is NULL. 
        // Returns false if string itself or substring is NULL.
        bool hasSuffix(const char * aCSubstring, EqualityMode aMode = caseSensitive) const;

        // Returns true if the last character of the string is aChar.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // If the string is empty or NULL returns false. Terminating '\0' character is not considered to be a part of the string.
        bool hasSuffix(char aChar, EqualityMode aMode = caseSensitive) const;


    // Substring
    public: 
        // Returns a substring beginning at aStartIndex of length aLength. 
        // If the string is NULL returns NULL. 
        String substringFrom(int aStartIndex, int aLength) const;

        // Returns a substring from aStartIndex to the end of the string.
        // If the string is NULL returns NULL. 
        inline String substringFrom(int aStartIndex) const { return substringFrom(aStartIndex, length() - aStartIndex); }

        // Returns a substring from the beginning to anEndIndex. Character at anEndIndex is not included (half open interval).
        // If the string is NULL returns NULL. 
        inline String substringBefore(int anEndIndex) const { return substringFrom(0, anEndIndex); }

        // Returns a substring from aStartIndex to anEndIndex. Character at anEndIndex is not included (half open interval).
        // If the string is NULL returns NULL. 
        inline String substringBetween(int aStartIndex, int anEndIndex) const { return substringFrom(aStartIndex, anEndIndex - aStartIndex); }


        // Returns a substring beginning at aStartIndex if it contains only characters which are contained (or are not contained) in aChars.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        // e.g. call substringOfCharsAt(1, containedIn, "21") for string "x123" returns "12".
        // If the string is NULL returns NULL. 
        inline String substringOfCharsAt(int aStartIndex, CharTestCondition aCondition, const String & aChars) const { return substringOfCharsAt(aStartIndex, aCondition, aChars.rb()); }

        // Returns a substring beginning at aStartIndex if it contains only characters which are contained (or are not contained) in aChars.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        // e.g. call substringOfCharsAt(1, containedIn, "21") for string "x123" returns "12".
        // If the string is NULL returns NULL. 
        String substringOfCharsAt(int aStartIndex, CharTestCondition aCondition, const char * aChars) const;

        // Returns a substring beginning at aStartIndex if it contains only characters for which aTestFunction returns aTestResult.
        // Method is designed for using with standard functions isalpha, isdigit, isspace, ... 
        // e.g. call substringOCharsFromWhere(0, isdigit, true) for string "123ABC" returns "123".
        // If the string is NULL returns NULL. 
        String substringOfCharsAtWhere(int aStartIndex, CharTestFunction aTestFunction, bool aTestResult) const;
            

    // Converting Uppercase / Lowercase
    public: 
        // Converts all letters in string to upper case or lower case.
        void convertTo(LetterCase aLetterCase);

        // Returns string with letters converted to upper case or lower case.
        inline String convertedTo(LetterCase aLetterCase) const { String result = *this; result.convertTo(aLetterCase); return result; }


        // Pointer to function which string methods use for conversion characters to lower case.
        // This function is used by methods for conversion string case and all methods which takes argument EqualityMode for case insensitive comparison.
        // By default is set to standard function tolower. It can be set to another function to customize conversion.
        static int (*onToLower)(int);

        // Pointer to function which string methods use for conversion characters to upper case.
        // This function is used by methods for conversion string case.
        // By default is set to standard function toupper. It can be set to another function to customize conversion.
        static int (*onToUpper)(int);


    // Appending
    public: 
        // Appends aString at the end of string. 
        // If aString is NULL then string is not changed. If string itself is NULL then assigns aString to string. 
        void append(const String aString);

        // Appends aString at the end of string. 
        // If aCString is NULL then string is not changed. If string itself is NULL then assigns aString to string.
        void append(const char * aCString);

        // Appends aChar at the end of string.
        // If string is NULL assigns aChar to string.
        void append(const char aChar); 


        // Appends string builded from aFormat and variadic arguments using same rules as standard printf function.
        // Buffer returned by methods rb() or wb() of this instance must not be passed as argument of the calling this method on same instance.
        // If aFormat is NULL then string is not changed.
        void appendFormatted(const char * aFormat, ...);

        // Appends string builded from aFormat and anArguments using same rules as standard vprintf function.
        // Buffer returned by methods rb() or wb() of this instance must not be passed as argument of the calling this method on same instance.
        // If aFormat is NULL then string is not changed.
        void appendFormattedList(const char * aFormat, va_list anArguments);


        // Appends string at the end of string.
        inline String & operator += (const String & aString) { append(aString); return *this; }

        // Appends cstring at the end of  string.
        inline String & operator += (const char * aCString) { append(aCString); return *this; }

        // Appends char at the end of string.
        inline String & operator += (const char aChar) { append(aChar); return *this; }


    // Inserting 
    public:     
        // Inserts aString at anIndex of string.
        // If aString is NULL then string is not changed. If string itself is NULL then simply assigns aString to string. 
        // If anIndex is less than zero then is considered to be zero. If anIndex is greater than last index of string then aString is appended at the end of string.
        void insertAt(int anIndex, const String aString);

        // Inserts aString at anIndex of string.
        // If aString is NULL then string is not changed. If string itself is NULL then simple assigns aString to string.
        // If anIndex is less than zero then is considered to be zero. If anIndex is greater than last index of string then aString is appended at the end of string.
        void insertAt(int anIndex, const char * aCString);

        // Inserts aChar at anIndex of string.
        // If string itself is NULL then assigns aChar to string.
        // If anIndex is less than zero then is considered to be zero. If anIndex is greater than last index of string then aChar is appended at the end of string.
        void insertAt(int anIndex, char aChar);


        // Inserts string builded from aFormat and variadic arguments using same rules as standard printf function.
        // Buffer returned by methods rb() or wb() of this instance must not be passed as argument of the calling this method on same instance.
        // If aFormat is NULL then string is not changed. If string itself is NULL then simply assigns the formatted string.
        // If anIndex is less than zero then is considered to be zero. If anIndex is greater than last index of string then the formatted string is appended at its end.
        void insertFormattedAt(int anIndex, const char * aFormat, ...);

        // Inserts string builded from aFormat and anArguments using same rules as standard vprintf function.
        // Buffer returned by methods rb() or wb() of this instance must not be passed as argument of the calling this method on same instance.
        // If aFormat is NULL then string is not changed. If string itself is NULL then simply assigns the formatted string.
        // If anIndex is less than zero then is considered to be zero. If anIndex is greater than last index of string then the formatted string is appended at its end.
        void insertFormattedListAt(int anIndex, const char * aFormat, va_list anArguments);


    // Ensuring Prefix / Suffix
    public:
        // If the string does not have aSubstring at the beginning it inserts aSubstring at the beginning.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        inline void ensurePrefix(const String aSubstring, EqualityMode aMode = caseSensitive) { ensurePrefix(aSubstring.rb(), aMode); }

        // If the string does not have aCSubstring at the beginning it inserts aCSubstring at the beginning.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        void ensurePrefix(const char * aSubstring, EqualityMode aMode = caseSensitive);

        // If the string does not have aChar at the beginning it inserts aChar at the beginning.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        void ensurePrefix(const char aChar, EqualityMode aMode = caseSensitive);


        // If the string does not have aSubstring at the end it inserts aSubstring at the end.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        inline void ensureSuffix(const String aSubstring, EqualityMode aMode = caseSensitive) { ensureSuffix(aSubstring.rb(), aMode); }

        // If the string does not have aCSubstring at the end it inserts aCSubstring at the end.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        void ensureSuffix(const char * aSubstring, EqualityMode aMode = caseSensitive);

        // If the string does not have aChar at the end it inserts aChar at the end.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        void ensureSuffix(const char aChar, EqualityMode aMode = caseSensitive);


    // Removing Prefix / Suffix
    public:
        // If the string starts with aSubstring it removes him. 
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // Returns true if prefix was removed from string.
        inline void removePrefix(const String aSubstring, EqualityMode aMode = caseSensitive) { return removePrefix(aSubstring.rb(), aMode); }

        // If the string starts with aSubstring it removes him. 
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // Returns true if prefix was removed from string.
        void removePrefix(const char * aCSubstring, EqualityMode aMode = caseSensitive);

        // If the string starts with aChar it removes him. 
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // Returns true if prefix was removed from string.
        void removePrefix(const char aChar, EqualityMode aMode = caseSensitive);


        // If the string ends with aSubstring it removes him.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // Returns true if suffix was removed from string.
        inline void removeSuffix(const String aSubstring, EqualityMode aMode = caseSensitive) { return removeSuffix(aSubstring.rb(), aMode); }

        // If the string ends with aSubstring it removes him.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // Returns true if suffix was removed from string.
        void removeSuffix(const char * aCSubstring, EqualityMode aMode = caseSensitive);

        // If the string ends with aChar it removes him.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        // Returns true if suffix was removed from string.
        void removeSuffix(const char aChar, EqualityMode aMode = caseSensitive);


    // Removing by Index
    public:         
        // Removes from string a substring beginning at aStartIndex of length aLength.
        void removeFrom(int aStartIndex, int aLength); 

        // Removes from string a substring beginning at aStartIndex to the end of the string.
        inline void removeFrom(int aStartIndex) { removeFrom(aStartIndex, length() - aStartIndex); } 

        // Removes from string a substring from the beginning to anEndIndex. Character at anEndIndex is not included (half open interval).
        inline void removeBefore(int anEndIndex) { removeFrom(0, anEndIndex); }

        // Removes from string a substring from aStartIndex to anEndIndex. Character at anEndIndex is not included (half open interval).
        inline void removeBetween(int aStartIndex, int anEndIndex) { removeFrom(aStartIndex, anEndIndex - aStartIndex); }


    // Removing by Content
    public:
        // Removes all occurrences of aSubstring beginning with aStartIndex.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        inline void remove(const String aSubstring, EqualityMode aMode = caseSensitive, int aStartIndex = 0) { replace(aSubstring.rb(), "", aMode, aStartIndex); }

        // Removes all occurrences of aSubstring beginning with aStartIndex.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        inline void remove(const char * aCSubstring, EqualityMode aMode = caseSensitive, int aStartIndex = 0) { replace(aCSubstring, "", aMode, aStartIndex); }

        // Removes all occurrences of aSubstring beginning with aStartIndex.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        void remove(char aChar, EqualityMode aMode = caseSensitive, int aStartIndex = 0);


        // Removes all characters which are contained (or are not contained) in aChars from the string beginning with aStartIndex.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        inline void removeChars(CharTestCondition aCondition, const String aChars, int aStartIndex = 0) { removeChars(aCondition, aChars.rb(), aStartIndex); }

        // Removes all characters which are contained (or are not contained) in aChars from the string beginning with aStartIndex.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        void removeChars(CharTestCondition aCondition, const char * aChars, int aStartIndex = 0);

        // Removes all characters from the string for which aTestFunction returns aTestResult.
        // Method is designed for using with standard functions isalpha, isdigit, isspace, ... 
        // e.g. removeCharsWhere(isspace, true) removes all whitespace characters from the string.
        void removeCharsWhere(CharTestFunction aTestFunction, bool aTestResult, int aStartIndex = 0);


    // Replacing
    public:
        // Replaces all occurencies (from aStartIndex) of the substring anOriginal with the string aSubstitute.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        void replace(const char * anOriginal, const char * aSubstitute, EqualityMode aMode = caseSensitive, int aStartIndex = 0);

        // Replaces all occurencies (from aStartIndex) of the substring anOriginal with the string aSubstitute.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        inline void replace(const String anOriginal, const char * aSubstitute, EqualityMode aMode = caseSensitive, int aStartIndex = 0) 
            { replace(anOriginal.rb(), aSubstitute, aMode, aStartIndex); }

        // Replaces all occurencies (from aStartIndex) of the substring anOriginal with the string aSubstitute.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        inline void replace(const char * anOriginal, const String aSubstitute, EqualityMode aMode = caseSensitive, int aStartIndex = 0) 
            { replace(anOriginal, aSubstitute.rb(), aMode, aStartIndex); }

        // Replaces all occurencies (from aStartIndex) of the substring anOriginal with the string aSubstitute.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        inline void replace(const String anOriginal, const String aSubstitute, EqualityMode aMode = caseSensitive, int aStartIndex = 0) 
            { replace(anOriginal.rb(), aSubstitute.rb(), aMode, aStartIndex); }


        // Replaces all occurencies (from aStartIndex) of character anOriginal with character aSubstitute.
        // Parameter aMode defines case sensitive (default) or case insensitive comparing.
        void replace(char anOriginal, char aSubstitute, EqualityMode aMode = caseSensitive, int aStartIndex = 0);


        // Replaces all characters which are contained (or are not contained) in aChars with character aSubstitute.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        // Parameter aStartIndex specifies the beginning of replacing.
        inline void replaceChars(CharTestCondition aCondition, const String aChars, char aSubstitute, int aStartIndex = 0) 
            { replaceChars(aCondition, aChars.rb(), aSubstitute, aStartIndex); }

        // Replaces all characters which are contained (or are not contained) in aChars with character aSubstitute.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        // Parameter aStartIndex specifies the beginning of replacing.
        void replaceChars(CharTestCondition aCondition, const char * aChars, char aSubstitute, int aStartIndex = 0);

        // Replaces all characters (from aStartIndex) for which aTestFunction returns aTestResult with character aSubstitute.
        // Method is designed for using with standard functions isalpha, isdigit, isspace, ... 
        // e.g. replaceCharsWhere(isspace, true, '_') replaces all white chars to character underscore
        void replaceCharsWhere(CharTestFunction aTestFunction, bool aTestResult, char aSubstitute, int aStartIndex = 0);


    // Trimming
    public: 
        // Removes all whitespace characters at the beginning and the end of the string.
        // Whitespace characters are characters for which standard function "isspace" returns true.
        void trimWhitespace();

        // Returns string without all whitespace characters at the beginning and the end.
        // Whitespace characters are characters for which function "isspace" returns true.
        inline String trimmedWhitespace() const { String result = *this; result.trimWhitespace(); return result; }


        // Removes all characters which are contained (or are not contained) in aChars from the beginning of the string.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        inline void trimLeftChars(CharTestCondition aCondition, const String aChars) { trimLeftChars(aCondition, aChars.rb()); }

        // Removes all characters which are contained (or are not contained) in aChars from the beginning of the string.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        void trimLeftChars(CharTestCondition aCondition, const char * aChars);

        // Removes all characters from the beginning of the string for which aTestFunction returns aTestResult.
        // Method is designed for using with standard functions isalpha, isdigit, isspace, ... 
        // e.g. trimLeftCharsWhere(isspace, true) removes all whitespace characters from the beginning of the string.
        void trimLeftCharsWhere(CharTestFunction aTestFunction, bool aTestResult);


        // Removes all characters which are contained (or are not contained) in aChars from the end of the string.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        inline void trimRightChars(CharTestCondition aCondition, const String aChars) { trimRightChars(aCondition, aChars.rb()); }

        // Removes all characters which are contained (or are not contained) in aChars from the end of the string.
        // Parameter aCondition specifies mode of character testing (only characters contained or not contained in aChars).
        void trimRightChars(CharTestCondition aCondition, const char * aChars);

        // Removes all characters from the end of the string for which aTestFunction returns aTestResult.
        // Method is designed for using with standard functions isalpha, isdigit, isspace, ...
        // e.g. trimRightCharsWhere(isspace, true) removes all whitespace characters from the end of the string.
        void trimRightCharsWhere(CharTestFunction aTestFunction, bool aTestResult);


    // Padding
    public: 
        // Inserts characters aChar from the left side of the string so that the total length will be aLength.
        // If aLength is less or equals current length then the string is not changed.
        void padLeft(int aLength, char aChar);

        // Returns string completed by characters aChar from the left side of the string so that the total length is aLength.
        // If aLength is less or equals current length then it returns unchanged string.
        inline String paddedLeft(int aLength, char aChar) const { String result = *this; result.padLeft(aLength, aChar); return result; }


        // Inserts characters aChar from the right side of the string so that the total length will be aLength.
        // If aLength is less or equals current length then the string is not changed.
        void padRight(int aLength, char aChar);

        // Returns string completed by characters aChar from the right side of the string so that the total length is aLength.
        // If aLength is less or equals current length then it returns unchanged string.
        inline String paddedRight(int aLength, char aChar) const { String result = *this; result.padRight(aLength, aChar); return result; }


        // Inserts characters aChar from both sides of the string so that the total length will be aLength and original string will be in the middle.
        // If aLength is less or equals current length then the string is not changed.
        void padCenter(int aLength, char aChar);

        // Returns string completed by from both sides of the string so that the total length is aLength and string is in the middle.
        // If aLength is less or equals current length then it returns unchanged string.
        inline String paddedCenter(int aLength, char aChar) const { String result = *this; result.padCenter(aLength, aChar); return result; }


    // Reversing
    public:
        // Reverses order of characters in string.
        // Reversing is done in place without allocating helper buffer.
        void reverse();

        // Returns reversed version of string.
        inline String reversed() const { String result = *this; result.reverse(); return result; }


    // Parsing
    public:
        // Method for incremental split string to parts separated by one of the delimiter characters.
        // It starts at position ioCharIndex (usually zero) and builds substring up to first found delimiter character (one on of the characters in aDelimiterChars). 
        // It returns builded substring in output argument oPart. Parameter ioCharIndex is setted to first character after delimiter. 
        // Method returns true if it was readed next part or false if ioCharIndex was at end of the string. 
        // This approach allows incremental iteration through all parts of the string e.g. in while cycle.
        // For direct access to parts of the string by index (without iteration) use methods "part" and "partCount".
        bool nextPart(String * oPart, int * ioCharIndex, const char * aDelimiterChars, const char * aQuotationChars = "", bool anIgnoreEmpty = false) const;
        bool nextPart(String * oPart, ParsingContext * aContext) const;

        int partCount(const char * aDelimiterChars, const char * aQuotationChars = "", bool anIgnoreEmpty = false) const;
        int partCount(const ParsingContext & aContext) const;

        String part(int aPartIndex, const char * aDelimiterChars, const char * aQuotationChars = "", bool anIgnoreEmpty = false) const;
        String part(int aPartIndex, const ParsingContext & aContext) const;


    // Equality
    public: 
        // Returns true if string contains same text as aString.
        // anEqualityMode determines case sensitive or case insensitive comparison.
        inline bool equals(const String & aString, EqualityMode aMode) const { return equals(aString.rb(), aMode); }

        // Returns true if string contains same text as aCString.
        // anEqualityMode determines case sensitive or case insensitive comparison.
        bool equals(const char * aCString, EqualityMode aMode) const;

        // Returns true if string contains only one character and the one is same as aChar.
        // anEqualityMode determines case sensitive or case insensitive comparison.
        bool equals(const char aChar, EqualityMode aMode) const; 


        // Returns true if both strings contains same text or both are NULL. The comparison is case sensitive.
        inline friend bool operator == (const String & aFirst, const String & aSecond) { return aFirst.equals(aSecond, caseSensitive); }

        // Returns true if strings do not contain same text or one is NULL and the second is not. The comparison is case sensitive.
        inline friend bool operator != (const String & aFirst, const String & aSecond) { return !aFirst.equals(aSecond, caseSensitive); }


        // Returns true if both strings contains same text or both are NULL. The comparison is case sensitive.
        inline friend bool operator == (const String & aFirst, const char * aSecond) { return aFirst.equals(aSecond, caseSensitive); }

        // Returns true if strings do not contain same text or one is NULL and the second is not. The comparison is case sensitive.
        inline friend bool operator != (const String & aFirst, const char * aSecond) { return !aFirst.equals(aSecond, caseSensitive); }


        // Returns true if both strings contains same text or both are NULL. The comparison is case sensitive.
        inline friend bool operator == (const char * aFirst, const String & aSecond) { return aSecond.equals(aFirst, caseSensitive); }

        // Returns true if strings do not contain same text or one is NULL and the second is not. The comparison is case sensitive.
        inline friend bool operator != (const char * aFirst, const String & aSecond) { return !aSecond.equals(aFirst, caseSensitive); }


        // Returns true if both strings contains same text or both are NULL. The comparison is case sensitive.
        inline friend bool operator == (const String & aFirst, const char aSecond) { return aFirst.equals(aSecond, caseSensitive); }

        // Returns true if strings do not contain same text or one is NULL and the second is not. The comparison is case sensitive.
        inline friend bool operator != (const String & aFirst, const char aSecond) { return !aFirst.equals(aSecond, caseSensitive); }


        // Returns true if both strings contains same text or both are NULL. The comparison is case sensitive.
        inline friend bool operator == (const char aFirst, const String & aSecond) { return aSecond.equals(aFirst, caseSensitive); }

        // Returns true if strings do not contain same text or one is NULL and the second is not. The comparison is case sensitive.
        inline friend bool operator != (const char aFirst, const String & aSecond) { return !aSecond.equals(aFirst, caseSensitive); }


    // Joining
    public: 
        // Joins two strings into one new string.
        // For reduce memory allocations it uses class _StringJoiningResult which is descendant of String and is used as a joining accumulator.
        // Result of the joinning must to be assigned to a variable of explicitly expressed type String don't use "auto" for declaration of target variable.
        // If both strings are NULL returns NULL. If one of strings is empty and second is NULL returs empty string.
        friend _StringJoiningResult & operator + (const _StringJoiningResult & aFirst, const String & aSecond);

        // Joins two strings into one new string.
        // For reduce memory allocations it uses class _StringJoiningResult which is descendant of String and is used as a joining accumulator.
        // Result of the joinning must to be assigned to a variable of explicitly expressed type String don't use "auto" for declaration of target variable.
        // If both strings are NULL returns NULL. If one of strings is empty and second is NULL returs empty string.
        friend _StringJoiningResult & operator + (const _StringJoiningResult & aFirst, const char * aSecond);

        // Joins string and char into new string.
        // For reduce memory allocations it uses class _StringJoiningResult which is descendant of String and is used as a joining accumulator.
        // Result of the joinning must to be assigned to a variable of explicitly expressed type String don't use "auto" for declaration of target variable.
        // If string is NULL and character is '\0' then returns empty string.
        friend _StringJoiningResult & operator + (const _StringJoiningResult & aFirst, char aSecond);


    // Assigning 
    public: 
        // Assigns string to string.
        String & operator = (const String & aString);

        // Assigns c string to string. 
        String & operator = (const char * aCString);

        // Assigns character to string.
        String & operator = (const char aChar);


    // Subscripting
    public: 
        // Returns character at index.
        // Terminating '\0' character at index length() is also accessible.
        inline char operator [] (int anIndex) const { return rb()[anIndex]; }

        // Sets character at index.
        // All characters in buffer from index 0 to capacity() of string are accessible.
        // Remember that string must be always terminated by null character.
        inline char & operator [] (int anIndex) { return *(wb() + anIndex); }


    // Error Handling
    public:
        // Pointer to function which is invoked if it isn't enough memory when string buffer allocation.
        // Custom function can be set which can e.g. print debug details about memory to console.
        // Parameter aRequstedSize contains size of memory block which was to be allocated from heap.
        // This method must not return, it must invoke standard method abort(). If method returns it will happen "undefined behaviour".
        // By default pointer is set to static method String::defaultOutOfMemoryHandler (see its description).
        static void (*onOutOfMemory)(size_t aRequestedSize);

        // Handler for out of memory situation (set to onOutOfMemory by default).
        // In debug mode writes message to console using printf and invokes standard function abort().
        static void defaultOutOfMemoryHandler(size_t aRequestedSize);


    // Internals
    private: 
        void setInner(const char * aCString);
        void setLiteral(const char * aCString);
        void setAllocation(int aCapacity, const char * aCString);
        inline void setBuffer(int aRequiredCapacity);

        void copyFrom(const char * aCString, int aLength);
        void retain(const String & anOther);
        void release();

        void uniquate(int aRequiredCapacity, bool aCopyOriginal, bool anAllowShrink);
        inline void uniquateInner(int aRequiredCapacity, bool aCopyOriginal);
        inline void uniquateLiteral(int aRequiredCapacity, bool aCopyOriginal);
        inline void uniquateSingleReferenceAllocation(int aRequiredCapacity, bool aCopyOriginal, bool anAllowShrink);
        inline void uniquateMultiReferenceAllocation(int aRequiredCapacity, bool aCopyOriginal);
            
        void enableLengthCache(int aLength);

        typedef bool (*ParameterizedCharTestFunction)(char aChar, void * aParameter);
        static ParameterizedCharTestFunction checkingFunctionFor(CharTestFunction aTestFunction, bool aResult);
        static ParameterizedCharTestFunction checkingFunctionFor(CharTestCondition aCondition);

        int indexOfAnyCharBy(ParameterizedCharTestFunction aTestFunction, void * aTestParameter, int aStartIndex) const;
        bool containsCharsAtBy(int anIndex, ParameterizedCharTestFunction aTestFunction, void * aTestParameter, int * oLength) const;
        bool containsAnyCharAtBy(int anIndex, ParameterizedCharTestFunction aTestFunction, void * aTestParameter) const;
        String substringOfCharsBy(int aStartIndex, ParameterizedCharTestFunction aTestFunction, void * aTestParameter) const;
        void removeCharsBy(ParameterizedCharTestFunction aTestFunction, void * aTestParameter, int aStartIndex);
        void replaceCharsBy(ParameterizedCharTestFunction aTestFunction, void * aTestParameter, char aSubstitute, int aStartIndex);
        void trimLeftCharsBy(ParameterizedCharTestFunction aTestFunction, void * aTestParameter);
        void trimRightCharsBy(ParameterizedCharTestFunction aTestFunction, void * aTestParameter);

        void appendBlockAndIncrementIndex(int * ioCharIndex, int aLength, String * ioToken, bool * ioTokenIsEmpty) const;

        _Allocation * _AllocationPublisher() {};  // only to make visible _Allocation in visual studio debugger

        enum Mode
        {
            smInner = 0,  
            smLiteral = 1,
            smAllocation = 2
        };

        #ifdef __GNUC__
            #define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
        #endif

        #ifdef _MSC_VER
            #define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
        #endif

        PACK(struct Fields
        {
            void * pointer;  
            int32_t lengthCache;
            unsigned int size : 30;  
            unsigned int mode : 2;
        });

        PACK(union Data
        {
            Fields asFields;
            char asCharsBuffer[12];
        }); 

        mutable Data data;
};


struct ParsingContext
{
    public:
        ParsingContext(const String & aDelimiterChars, const String & aQuotationChars = String::empty, bool anIgnoreEmpty = false):
            delimiterChars(aDelimiterChars), quotingChars(aQuotationChars), ignoreEmpty(anIgnoreEmpty) {}

        void reset() { charIndex = 0; }            

    public:
        String delimiterChars;
        String quotingChars;
        bool ignoreEmpty;
        int charIndex = 0;
};


class _StringJoiningResult: public String
{
    public:
        inline _StringJoiningResult(const String & aString): String(aString) {}
        inline _StringJoiningResult(const char * aCString): String(aCString) {}
        inline _StringJoiningResult(char aChar): String(aChar) {}
};


struct _Allocation
{
    uint16_t references;
    
    #pragma warning(suppress : 4200)  // suppress MSVC warning about zero sized variable
    char buffer[];

    inline static size_t sizeForBufferSize(size_t aBufferSize)
    {
        return sizeof(_Allocation) + aBufferSize * sizeof(char);
    }
};


}


#endif // _PracticString_

