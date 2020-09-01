// Copyright (c) 2020 Stanislav Jurny (github.com/STjurny) licence MIT

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cctype>
#include <climits>
#include "PracticString.h"


#pragma warning(disable : 4996)  // Turn of MSVC deprecation warning C4996


namespace Practic {


//#define PRACTIC_STRING_DEBUG  


// Assertions Configuration ///////////////////////////////////////////////////////////////////////////////////////////

#ifdef PRACTIC_STRING_DEBUG
    #define debug_assert(param) assert(param)
#else
    #define debug_assert(param) 
#endif





// Error Handling /////////////////////////////////////////////////////////////////////////////////////////////////////
void (*String::onOutOfMemory)(size_t aRequiredSize) = String::defaultOutOfMemoryHandler;


void String::defaultOutOfMemoryHandler(size_t aRequestedSize)
{
    #ifdef PRACTIC_STRING_DEBUG
        printf("Practic::String can't allocated buffer of size %u bytes.\n", aRequestedSize);
    #endif

    abort();
}





// Char Test Functions ////////////////////////////////////////////////////////////////////////////////////////////////

bool charIsContainedIn(char aChar, void * aChars)
{
    if (aChars)
        return strchr((const char *)aChars, aChar);
    else
        return false;
}


bool charIsNotContainedIn(char aChar, void * aChars)
{
    if (aChars)
        return !strchr((const char *)aChars, aChar);
    else
        return true;
}


String::ParameterizedCharTestFunction String::checkingFunctionFor(CharTestCondition aCondition)
{
    if (aCondition == containedIn)
        return charIsContainedIn;
    else
        return charIsNotContainedIn;
}


bool testFunctionReturnsTrue(char aChar, void * aTestFunction)
{
    return ((CharTestFunction)aTestFunction)(aChar);
}


bool testFunctionReturnsFalse(char aChar, void * aTestFunction)
{
    return !((CharTestFunction)aTestFunction)(aChar);
}


String::ParameterizedCharTestFunction String::checkingFunctionFor(CharTestFunction aTestFunction, bool aResult)
{
    if (!aTestFunction)
        return NULL;
    else
        if (aResult)
            return testFunctionReturnsTrue;
        else
            return testFunctionReturnsFalse;
}





// Case Insensitive Search ////////////////////////////////////////////////////////////////////////////////////////////

const char * strichr(const char * aString, int aChar)
{
    const char * currentChar = aString;
    char searchedChar = String::onToLower(aChar);

    do {
        if (String::onToLower(*currentChar) == searchedChar)
            return currentChar;
    } while (*currentChar++);

    return NULL;
}


const char * stristr(const char * aString, const char * aSubstring) 
{
    do  {
        const char * stringChar = aString;
        const char * substringChar = aSubstring;

        while (String::onToLower(*stringChar) == String::onToLower(*substringChar) && *substringChar) 
        {
            stringChar++;
            substringChar++;
        }

        if (*substringChar == '\0') 
            return aString;

    } while (*aString++);

    return NULL;
}


char * findSubstring(const char * aString, const char * aSubstring, EqualityMode aMode)
{
    if (aMode == caseSensitive)
        return (char*) strstr(aString, aSubstring);
    else
        return (char*) stristr(aString, aSubstring);
}





// Internal Types /////////////////////////////////////////////////////////////////////////////////////////////////////

#define self (*this)
#define isEmptyCString(cstring) (!cstring[0])

#define asLiteral(pointer) ((char *) pointer)
#define asAllocation(pointer) ((_Allocation *) pointer)
 

enum LengthCacheState
{
    csUnknown = -1,
    csDisabled = -2
};





// Lifecycle //////////////////////////////////////////////////////////////////////////////////////////////////////////

const String String::null = String(NULL, true);

const String String::empty = String();


String::String()
{
    data.asFields.mode = smInner;
    data.asCharsBuffer[0] = '\0';
}


String::String(char aChar)
{
    data.asFields.mode = smInner;
    data.asCharsBuffer[0] = aChar;
    data.asCharsBuffer[1] = '\0';
}


String::String(const char * aCString, bool aCStringIsLiteral) 
{
    if (aCString == NULL)
        setLiteral(NULL);
    else 
        if (isEmptyCString(aCString))
            setInner("");
        else
            if (aCStringIsLiteral)
                setLiteral(aCString);
            else
            {
                int length = strlen(aCString);
                setBuffer(length);
                copyFrom(aCString, length);
            }
}


String::String(const char * aCString, int aMaxLength) 
{
    if (aCString == NULL)
        setLiteral(NULL);
    else 
        if (aMaxLength <= 0 || isEmptyCString(aCString))
            setInner("");
        else
            if (aMaxLength == 1)
            {
                char chars[] = { aCString[0], '\0' };
                setInner(chars);
            }
            else
            {
                int actualLength = strnlen(aCString, aMaxLength);
                setBuffer(actualLength);
                copyFrom(aCString, actualLength);
            }
}


String::String(const String & aString) 
{
    retain(aString);
}


String::~String() 
{
    release();
}





// Managing Capacity //////////////////////////////////////////////////////////////////////////////////////////////////

String String::withCapacity(int aRequiredCapacity)
{
    String s;

    if (aRequiredCapacity > innerCapacity)
        s.setAllocation(aRequiredCapacity, "");

    return s;
}


void String::reserveCapacity(int aRequiredCapacity, bool aCopyOriginal, bool anAllowShrink) 
{
    if (aRequiredCapacity <= capacity() && aCopyOriginal && !anAllowShrink)
        return;  // prevents premature allocating buffer but same behaviour as method "wb"

    uniquate(aRequiredCapacity, aCopyOriginal, anAllowShrink);  
}


void String::minimizeCapacity() 
{
    if (references() == 1)
        uniquate(0, true, true);
}





// Properties /////////////////////////////////////////////////////////////////////////////////////////////////////////

int String::capacity() const  // don't use this method in basic manipulations because it has side efects
{
    if (data.asFields.mode == smInner)
        return innerCapacity;
    else
        if (data.asFields.mode == smLiteral)
            return length();
        else
            return data.asFields.size - 1;
}


int String::length() const  // don't use this method in basic manipulations because it has side efects
{
    int result;

    if (data.asFields.mode == smInner)
        result = strlen(data.asCharsBuffer);
    else
    {    
        if (data.asFields.lengthCache != csUnknown && data.asFields.lengthCache != csDisabled)
            result = data.asFields.lengthCache;
        else
        {
            if (data.asFields.pointer == NULL)
                result = 0;
            else
            {
                char * charsBuffer;     

                if (data.asFields.mode == smLiteral)
                    charsBuffer = asLiteral(data.asFields.pointer);
                else
                    charsBuffer = asAllocation(data.asFields.pointer)->buffer;

                result = strlen(charsBuffer);
            }

            if (data.asFields.lengthCache != csDisabled)
                data.asFields.lengthCache = result;
        }
    }    

    debug_assert(isNull() || isEmpty() ? result == 0 : result == strlen(rb()));

    return result;
}


bool String::isNull() const 
{
    return data.asFields.mode == smLiteral && asLiteral(data.asFields.pointer) == NULL;
}


bool String::isEmpty() const
{
    if (data.asFields.mode == smInner)
        return data.asCharsBuffer[0] == '\0';
    else
        if (data.asFields.mode == smAllocation)
            return asAllocation(data.asFields.pointer)->buffer[0] == '\0';
        else
            if (asLiteral(data.asFields.pointer) == NULL)
                return false;
            else
                return asLiteral(data.asFields.pointer)[0] == '\0';
}


int String::references() const 
{
    if (data.asFields.mode == smAllocation)
        return asAllocation(data.asFields.pointer)->references;
    else
        return -1;
}





// Basic Manipulations ////////////////////////////////////////////////////////////////////////////////////////////////

void String::setInner(const char * aCString) 
{
    debug_assert(aCString);
    debug_assert(strlen(aCString) <= innerCapacity);

    data.asFields.mode = smInner;

    if (aCString)
        strcpy(data.asCharsBuffer, aCString);
}


void String::setLiteral(const char * aCString) 
{
    data.asFields.mode = smLiteral;
    data.asFields.lengthCache = csUnknown;
    data.asFields.pointer = (void *) aCString;
}


void String::setAllocation(int aCapacity, const char * aCString) 
{
    debug_assert(aCString);
    debug_assert(aCapacity >= 0);
    debug_assert(aCapacity <= maxCapacity);
    debug_assert(strlen(aCString) <= (unsigned) aCapacity);

    int size = aCapacity + 1;  // add one position for terminating null character

    void * pointer = malloc(_Allocation::sizeForBufferSize(size));

    if (!pointer)
        onOutOfMemory(size);

    #pragma warning (suppress : 6011)  // suppress MSVC warning about referencing NULL pointer
    strcpy(asAllocation(pointer)->buffer, aCString);  
    asAllocation(pointer)->references = 1;

    data.asFields.mode = smAllocation;
    data.asFields.size = size;
    data.asFields.pointer = pointer;
    #pragma warning (suppress : 6011)  // suppress MSVC warning about referencing NULL pointer
    data.asFields.lengthCache = isEmptyCString(aCString) ? 0 : csUnknown;
}


inline void String::setBuffer(int aRequiredCapacity) 
{
    if (aRequiredCapacity <= innerCapacity)
        setInner("");
    else
        setAllocation(aRequiredCapacity, "");
}


void String::retain(const String & anOther) 
{
    data = anOther.data;

    if (data.asFields.mode == smAllocation)
        asAllocation(data.asFields.pointer)->references += 1;
}


void String::release() 
{
    if (data.asFields.mode == smAllocation) 
    {
        asAllocation(data.asFields.pointer)->references -= 1;

        if (asAllocation(data.asFields.pointer)->references == 0)
            free(data.asFields.pointer);
    }

    data.asFields.mode = smLiteral;  // remove reference to released allocation 
    data.asFields.lengthCache = 0;   // and set fData to consistent state  
    data.asFields.pointer = NULL;
}


void String::copyFrom(const char * aCString, int aLength) 
{
    debug_assert(aCString);
    debug_assert(data.asFields.mode != smLiteral);

    char * buffer;
    if (data.asFields.mode == smInner)
    {
        debug_assert(aLength <= innerCapacity);
        debug_assert(strnlen(aCString, aLength) <= innerCapacity);

        buffer = data.asCharsBuffer;
    }
    else
    {
        debug_assert((unsigned) aLength < data.asFields.size);
        debug_assert(strnlen(aCString, aLength) < data.asFields.size);

        data.asFields.lengthCache = aLength;
        buffer = asAllocation(data.asFields.pointer)->buffer;
    }

    strncpy(buffer, aCString, aLength);
    buffer[aLength] = '\0';
}





// Uniquating /////////////////////////////////////////////////////////////////////////////////////////////////////////

void String::uniquate(int aRequiredCapacity, bool aCopyOriginal, bool anAllowShrink) 
{
    debug_assert(aRequiredCapacity <= maxCapacity);

    if (data.asFields.mode == smInner)
        uniquateInner(aRequiredCapacity, aCopyOriginal);
    else
        if (data.asFields.mode == smLiteral)
            uniquateLiteral(aRequiredCapacity, aCopyOriginal);
        else
        {
            debug_assert(asAllocation(data.asFields.pointer)->references > 0);

            if (asAllocation(data.asFields.pointer)->references == 1)
                uniquateSingleReferenceAllocation(aRequiredCapacity, aCopyOriginal, anAllowShrink);
            else
                uniquateMultiReferenceAllocation(aRequiredCapacity, aCopyOriginal);
        }

    debug_assert(data.asFields.mode != smLiteral);
}


inline void String::uniquateInner(int aRequiredCapacity, bool aCopyOriginal) 
{
    if (aRequiredCapacity <= unchanged || aRequiredCapacity <= innerCapacity)
    {
        if (!aCopyOriginal)
            data.asCharsBuffer[0] = '\0';  // set empty string for consistent behaviour
    }
    else
        if (aCopyOriginal) 
            setAllocation(aRequiredCapacity, data.asCharsBuffer);
        else
            setAllocation(aRequiredCapacity, "");
}


inline void String::uniquateLiteral(int aRequiredCapacity, bool aCopyOriginal) 
{
    int originalLength = data.asFields.lengthCache;
    
    if (originalLength == csUnknown || originalLength == csDisabled)
    {
        if (data.asFields.pointer == NULL)
            originalLength = 0;
        else
            originalLength = strlen(asLiteral(data.asFields.pointer));
    }

    if (aRequiredCapacity <= unchanged || (aRequiredCapacity < originalLength && aCopyOriginal))
        aRequiredCapacity = originalLength;

    char * originalBuffer = asLiteral(data.asFields.pointer);

    setBuffer(aRequiredCapacity);

    if (aCopyOriginal && originalLength)
        copyFrom(originalBuffer, originalLength);
}


inline void String::uniquateMultiReferenceAllocation(int aRequiredCapacity, bool aCopyOriginal) 
{
    if (aRequiredCapacity <= unchanged)
        aRequiredCapacity = data.asFields.size - 1;

    if (aCopyOriginal)
    {
        char * originalBuffer = asAllocation(data.asFields.pointer)->buffer;

        int originalLength = data.asFields.lengthCache;
        if (originalLength == csUnknown || originalLength == csDisabled)
            originalLength = strlen(originalBuffer);
        
        if (aRequiredCapacity < originalLength)
            aRequiredCapacity = originalLength;

        release();
        setBuffer(aRequiredCapacity);
        copyFrom(originalBuffer, originalLength);
    }
    else
    {
        release();
        setBuffer(aRequiredCapacity);
    }
}


inline void String::uniquateSingleReferenceAllocation(int aRequiredCapacity, bool aCopyOriginal, bool anAllowShrink) 
{
    unsigned int requiredSize;

    if (aRequiredCapacity <= unchanged)
        requiredSize = data.asFields.size;
    else
        requiredSize = aRequiredCapacity + 1;

    if (requiredSize < data.asFields.size)
    {
        if (!anAllowShrink)
            requiredSize = data.asFields.size;
        else
            if (aCopyOriginal)
            {
                if (data.asFields.lengthCache == csUnknown || data.asFields.lengthCache == csDisabled)
                    data.asFields.lengthCache = strlen(asAllocation(data.asFields.pointer)->buffer);  // actualize length cache
                
                unsigned int occupiedSize = data.asFields.lengthCache + 1;

                if (requiredSize < occupiedSize)
                    requiredSize = occupiedSize;           
            }
    }

    if (!aCopyOriginal)
    {
        asAllocation(data.asFields.pointer)->buffer[0] = '\0';
        data.asFields.lengthCache = 0;
    }

    if (requiredSize != data.asFields.size)
    {
        unsigned int innerSize = innerCapacity + 1;

        if (requiredSize <= innerSize)
        {
            void * pointer = data.asFields.pointer;
            setInner(asAllocation(pointer)->buffer);
            free(pointer);
        }
        else
        {
            void * newPointer = realloc(data.asFields.pointer, _Allocation::sizeForBufferSize(requiredSize));

            if (!newPointer)
                onOutOfMemory(requiredSize);

            data.asFields.pointer = newPointer;
            data.asFields.size = requiredSize;
        }
    }
}





// Accessing Buffer ///////////////////////////////////////////////////////////////////////////////////////////////////

const char * String::rb() const
{   
    if (data.asFields.mode == smInner)
        return data.asCharsBuffer;
    else
        if (data.asFields.mode == smLiteral)
            return asLiteral(data.asFields.pointer);
        else
            return asAllocation(data.asFields.pointer)->buffer;
}


char * String::wb(int aRequiredCapacity, bool aCopyOriginal, bool anAllowShrink) 
{
    uniquate(aRequiredCapacity, aCopyOriginal, anAllowShrink);  // same behaviour as method reserveCapacity

    if (data.asFields.mode == smInner)
        return data.asCharsBuffer;
    else
    {
        data.asFields.lengthCache = csDisabled;  // stop caching until next call non-const method 
        return asAllocation(data.asFields.pointer)->buffer;
    }
}


void String::enableLengthCache(int aLength) 
{
    debug_assert(aLength == csUnknown || aLength == strlen(rb()));

    if (data.asFields.mode != smInner)
        data.asFields.lengthCache = aLength;

    // ensure string terminating with '\0' in all cases
    if (data.asFields.mode == smInner)  
        data.asCharsBuffer[innerCapacity] = '\0';
    else
        if (data.asFields.mode == smAllocation)
            asAllocation(data.asFields.pointer)->buffer[data.asFields.size-1] = '\0';
}





// Assigning //////////////////////////////////////////////////////////////////////////////////////////////////////////

String & String::operator = (const String & anOther) 
{
    if (this == &anOther)
        return *this;

    if (references() == 1 && !anOther.isNull())       // prefer copying to already allocated buffer
    {
        int otherLength = anOther.length();
        if (otherLength <= self.capacity())           // if the buffer has enough capacity
        {
            copyFrom(anOther.rb(), otherLength);  
            return *this;
        }
    }
    
    release();
    retain(anOther);

    return *this;
}


String & String::operator = (const char * aCString) 
{
    if (aCString == NULL)
    {
        release();
        setLiteral(NULL);
    }
    else
    {
        int length = strlen(aCString);
        uniquate(length, false, false);
        copyFrom(aCString, length);
    }

    return *this;
}


String & String::operator = (const char aChar) 
{
    char * buffer = wb(1, false, false);

    buffer[0] = aChar;

    if (aChar == '\0')
        enableLengthCache(0);
    else
    {
        buffer[1] = '\0';
        enableLengthCache(1);
    }    

    return *this;
}





// Appending //////////////////////////////////////////////////////////////////////////////////////////////////////////

void String::append(const String aString) 
{
    if (aString.isNull())
        return;

    if (aString.isEmpty())  // prevent reallocation (calling wb) unless there is change
    {
        if (self.isNull())
            setInner("");
        return;
    }

    if (self.isNull() || (self.isEmpty() && data.asFields.mode != smAllocation))
        retain(aString);
    else
    {
        int selfLength = self.length();
        int otherLength = aString.length();
        int newLength = selfLength + otherLength;

        char * buffer = wb(newLength);
        memcpy(buffer + selfLength, aString.rb(), otherLength);  // strcat or strcpy can't be used because possible ovelapping on terminator char
        buffer[newLength] = '\0';
        enableLengthCache(newLength);
    }
}


void String::append(const char * aCString)
{
    if (!aCString)
        return;

    if (isEmptyCString(aCString))  // prevent reallocation (calling wb) unless there is change
    {
        if (self.isNull())
            setInner("");
        return;
    }

    int selfLength = self.length();
    int otherLength = strlen(aCString);
    int newLength = selfLength + otherLength;

    char * buffer = wb(newLength);
    memcpy(buffer + selfLength, aCString, otherLength);  // strcat or strcpy can't be used because possible ovelapping on terminator char
    buffer[newLength] = '\0';
    enableLengthCache(newLength);
}


void String::append(const char aChar)
{
    if (aChar == '\0' && !isNull())  // prevent reallocation (calling wb) unless there is change
        return;

    int newIndex = self.length();
    int newLength = newIndex + 1;

    char * buffer = self.wb(newLength);
    buffer[newIndex] = aChar;
    buffer[newIndex+1] = '\0';

    if (aChar == '\0')
        newLength -= 1;

    enableLengthCache(newLength);
}


void String::appendFormatted(const char * aFormat, ...)
{
    va_list arguments;
    va_start(arguments, aFormat);
        appendFormattedList(aFormat, arguments);
    va_end(arguments);
}


void String::appendFormattedList(const char * aFormat, va_list anArguments)
{
    if (!aFormat)
        return;

    va_list arguments;

    va_copy(arguments, anArguments);
        int formattedLength = vsnprintf(NULL, 0, aFormat, arguments);
        debug_assert(formattedLength >= 0);
    va_end(arguments);

    if (formattedLength == 0)  // prevent reallocation (calling wb) unless there is change
    {
        if (self.isNull()) 
            setInner("");
        return;
    }

    int selfLength = self.length();
    int newLength = selfLength + formattedLength;

    char * buffer = wb(newLength);

    va_copy(arguments, anArguments);
        vsnprintf(buffer + selfLength, formattedLength + 1, aFormat, arguments);
    va_end(arguments);

    enableLengthCache(newLength);
}





// Joining ////////////////////////////////////////////////////////////////////////////////////////////////////////////

_StringJoiningResult & operator +(const _StringJoiningResult & aFirst, const String & aSecond)
{
    _StringJoiningResult & first = const_cast<_StringJoiningResult&>(aFirst);
    first.append(aSecond);
    return first;
}


_StringJoiningResult & operator +(const _StringJoiningResult & aFirst, const char * aSecond)
{
    _StringJoiningResult & first = const_cast<_StringJoiningResult&>(aFirst);
    first.append(aSecond);
    return first;
}


_StringJoiningResult & operator +(const _StringJoiningResult & aFirst, char aSecond)
{
    _StringJoiningResult & first = const_cast<_StringJoiningResult&>(aFirst);
    first.append(aSecond);
    return first;
}





// Equality ///////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool stringsEqualIgnoreCase(const char * aFirst, const char * aSecond)
{
    while (*aFirst)
        if (String::onToLower(*aFirst++) != String::onToLower(*aSecond++))
            return false;

    return String::onToLower(*aFirst) == String::onToLower(*aSecond);
}


bool String::equals(const char * anOther, EqualityMode aMode) const
{
    if (isNull())
        return anOther == NULL;
    else
        if (anOther == NULL)
            return isNull();
        else
        {
            const char * selfBuffer = self.rb();

            if (selfBuffer == anOther)
                return true;
            else
                if (aMode == caseSensitive)
                    return !strcmp(selfBuffer, anOther);
                else
                    return stringsEqualIgnoreCase(selfBuffer, anOther);
        }
}


bool String::equals(const char aChar, EqualityMode anEqualityMode) const
{
    if (isNull() || isEmpty())
        return false;
    else 
    {
        const char * buffer = rb();

        if (anEqualityMode == caseSensitive)
            return buffer[0] == aChar && buffer[1] == '\0';
        else
            return onToLower(buffer[0]) == onToLower(aChar) && buffer[1] == '\0';  
    }
}





// Searching //////////////////////////////////////////////////////////////////////////////////////////////////////////

int String::indexOf(const char * aCSubstring, EqualityMode aMode, int aStartIndex) const
{
    if (isNull() || !aCSubstring || aStartIndex > length())
        return notFound;

    if (aStartIndex < 0)
        aStartIndex = 0;

    const char * buffer = rb();
    const char * position = findSubstring(buffer + aStartIndex, aCSubstring, aMode);

    if (position)
        return position - buffer;
    else
        return notFound;
}


int String::indexOf(char aChar, EqualityMode aMode, int aStartIndex) const
{
    if (aStartIndex < 0)
        aStartIndex = 0;

    if (aStartIndex >= length())
        return notFound;

    const char * firstChar = rb();

    const char * position;
    if (aMode == caseSensitive)
        position = strchr(firstChar + aStartIndex, aChar);
    else
        position = strichr(firstChar + aStartIndex, aChar);

    if (position && *position)     // terminating char is not part of the string
        return position - firstChar;
    else
        return notFound;
}


int String::indexOfAnyCharBy(ParameterizedCharTestFunction aTestFunction, void * aTestParameter, int aStartIndex) const
{
    if (!aTestFunction)
        return notFound;

    if (aStartIndex < 0)
        aStartIndex = 0;

    if (aStartIndex >= length()) // null, empty, beyond end
        return notFound;

    const char * firstChar = rb();
    const char * testedChar = firstChar + aStartIndex;

    while (*testedChar)
    {
        if (aTestFunction(*testedChar, aTestParameter)) 
            return testedChar - firstChar;
        testedChar++;
    }

    return notFound;
}


int String::indexOfAnyChar(CharTestCondition aCondition, const char * aChars, int aStartIndex) const
{
    return indexOfAnyCharBy(checkingFunctionFor(aCondition), (void *)aChars, aStartIndex);
}


int String::indexOfAnyCharWhere(CharTestFunction aTestFunction, bool aTestResult, int aStartIndex) const
{
    return indexOfAnyCharBy(checkingFunctionFor(aTestFunction, aTestResult), (void *)aTestFunction, aStartIndex);
} 





// Containing substring / character ///////////////////////////////////////////////////////////////////////////////////

bool String::containsOnlyChars(CharTestCondition aCondition, const char * aChars) const
{
    if (isEmpty() || isNull())
        return false;

    if (!aChars)
        aChars = "";

    if (aCondition == containedIn)
        aCondition = notContainedIn;
    else
        aCondition = containedIn;

    return indexOfAnyChar(aCondition, aChars, 0) == notFound;
}


bool String::containsOnlyCharsWhere(CharTestFunction aTestFunction, bool aTestResult) const 
{ 
    if (!aTestFunction || isEmpty() || isNull())
        return false;
    else
        return indexOfAnyCharWhere(aTestFunction, !aTestResult) == notFound; 
}


bool String::containsAt(int anIndex, const char * aCSubstring, EqualityMode aMode) const
{
    if (isNull() || !aCSubstring || anIndex < 0 || anIndex > length())
        return false;

    const char * testedChar = aCSubstring;
    const char * selfChar = rb() + anIndex;

    if (aMode == caseSensitive)
    {
        while (*testedChar)
            if (*testedChar++ != *selfChar++)
                return false;
    }
    else
    {
        while (*testedChar)
            if (onToLower(*testedChar++) != onToLower(*selfChar++))
                return false;
    }

    return true;
}


bool String::containsAt(int anIndex, char aChar, EqualityMode aMode) const
{
    if (anIndex < 0 || anIndex >= length())
        return false;

    char testedChar = rb()[anIndex];

    if (aMode == caseSensitive)
        return testedChar == aChar;
    else
        return onToLower(testedChar) == onToLower(aChar);
}



bool String::containsCharsAtBy(int anIndex, ParameterizedCharTestFunction aTestFunction, void * aTestParameter, int * oLength) const
{
    *oLength = 0;

    if (!aTestFunction || anIndex < 0 || anIndex >= length())
        return false;

    const char * firstChar = rb() + anIndex;
    const char * testedChar = firstChar;

    while (*testedChar && aTestFunction(*testedChar, aTestParameter))
        testedChar++;

    *oLength = testedChar - firstChar;

    return *oLength;
}


bool String::containsCharsAt(int anIndex, CharTestCondition aCondition, const char * aChars, int * oLength) const
{
    return containsCharsAtBy(anIndex, checkingFunctionFor(aCondition), (void *)aChars, oLength);
}


bool String::containsCharsAtWhere(int anIndex, CharTestFunction aTestFunction, bool aTestResult, int * oLength) const
{
    return containsCharsAtBy(anIndex, checkingFunctionFor(aTestFunction, aTestResult), (void *)aTestFunction, oLength);
}


bool String::containsAnyCharAtBy(int anIndex, ParameterizedCharTestFunction aTestFunction, void * aTestParameter) const
{
    return aTestFunction && anIndex >= 0 && anIndex < length() && aTestFunction(rb()[anIndex], aTestParameter);
}


bool String::containsAnyCharAt(int anIndex, CharTestCondition aCondition, const char * aChars) const 
{
    return containsAnyCharAtBy(anIndex, checkingFunctionFor(aCondition), (void *)aChars);
}


bool String::containsAnyCharAtWhere(int anIndex, CharTestFunction aTestFunction, bool aTestResult) const
{
    return containsAnyCharAtBy(anIndex, checkingFunctionFor(aTestFunction, aTestResult), (void *) aTestFunction);
}





// Substrings /////////////////////////////////////////////////////////////////////////////////////////////////////////

String String::substringFrom(int aStartIndex, int aLength) const
{
    if (isNull())
        return null;

    if (isEmpty())
        return empty;

    if (aStartIndex < 0)
    {
        aLength += aStartIndex;
        aStartIndex = 0;
    }

    if (aLength <= 0)
        return empty;

    int selfLength = length();

    if (aStartIndex >= selfLength)
        return empty;

    if (aStartIndex + aLength > selfLength)
        aLength = selfLength - aStartIndex;

    if (aStartIndex == 0 && aLength == selfLength)
        return self;

    return String(rb() + aStartIndex, aLength);
}


String String::substringOfCharsBy(int aStartIndex, ParameterizedCharTestFunction aTestFunction, void * aTestParameter) const
{
    if (isNull())
        return null;

    if (!aTestFunction || isEmpty() || aStartIndex >= length())
        return empty;

    int substringLength;    
    if (containsCharsAtBy(aStartIndex, aTestFunction, aTestParameter, &substringLength))
        return substringFrom(aStartIndex, substringLength);
    else
        return empty;
}


String String::substringOfCharsAt(int aStartIndex, CharTestCondition aCondition, const char * aChars) const
{   
    return substringOfCharsBy(aStartIndex, checkingFunctionFor(aCondition), (void *)aChars);
}


String String::substringOfCharsAtWhere(int aStartIndex, CharTestFunction aTestFunction, bool aTestResult) const
{
    return substringOfCharsBy(aStartIndex, checkingFunctionFor(aTestFunction, aTestResult), (void *)aTestFunction);
}

            



// Testing Suffex /////////////////////////////////////////////////////////////////////////////////////////////////////

bool String::hasSuffix(const char * aCSubstring, EqualityMode aMode) const
{
    if (!aCSubstring)
        return false;

    int selfLength = length();
    int otherLength = strlen(aCSubstring);

    return otherLength <= selfLength && containsAt(selfLength - otherLength, aCSubstring, aMode);
}


bool String::hasSuffix(char aChar, EqualityMode aMode) const
{
    int selfLength = length();
    return selfLength && containsAt(selfLength-1, aChar, aMode);
}





// Inserting //////////////////////////////////////////////////////////////////////////////////////////////////////////

void String::insertAt(int anIndex, const String aString)
{
    if (aString.isNull())
        return;

    if ((self.isEmpty() || self.isNull()) && data.asFields.mode != smAllocation)
    {
        retain(aString);
        return;
    }

    int otherLength = aString.length();  

    if (otherLength == 0)  // prevent reallocation (call wb) when there is nothing to insert 
        return;

    int selfLength = length();

    if (anIndex > selfLength)
        anIndex = selfLength;

    if (anIndex < 0)
        anIndex = 0;

    int newLength = selfLength + otherLength;

    char * buffer = wb(newLength);
    memmove(buffer + anIndex + otherLength, buffer + anIndex, selfLength - anIndex + 1);
    memcpy(buffer + anIndex, aString.rb(), otherLength);
    enableLengthCache(newLength);
}


void String::insertAt(int anIndex, const char * aCString)
{
    if (!aCString)
        return;

    int otherLength = strlen(aCString);  

    if ((self.isEmpty() || self.isNull()) && data.asFields.mode != smAllocation)
    {
        setBuffer(otherLength);
        copyFrom(aCString, otherLength);
        return;
    }

    if (otherLength == 0)  // prevent reallocation (call wb) when there is nothing to insert 
        return;

    int selfLength = length();

    if (anIndex > selfLength)
        anIndex = selfLength;

    if (anIndex < 0)
        anIndex = 0;

    int newLength = selfLength + otherLength;

    char * buffer = wb(newLength);
    memmove(buffer + anIndex + otherLength, buffer + anIndex, selfLength - anIndex + 1);
    memcpy(buffer + anIndex, aCString, otherLength);
    enableLengthCache(newLength);
}

       
void String::insertAt(int anIndex, char aChar)
{
    int selfLength = length();

    if (anIndex > selfLength)
        anIndex = selfLength;

    if (anIndex < 0)
        anIndex = 0;

    int newLength;
    char * buffer;

    if (aChar == '\0')
    {
        if (!isNull() && anIndex == selfLength) // prevent reallocation (calling wb) unless there is change
            return;

        newLength = anIndex;
        buffer = wb(newLength);
    }
    else
    {
        newLength = selfLength + 1;
        buffer = wb(newLength);
        memmove(buffer + anIndex + 1, buffer + anIndex, selfLength - anIndex + 1);
    }

    buffer[anIndex] = aChar;
    enableLengthCache(newLength);
}


void String::insertFormattedAt(int anIndex, const char * aFormat, ...)
{
    va_list arguments;
    va_start(arguments, aFormat);
        insertFormattedListAt(anIndex, aFormat, arguments);
    va_end(arguments);
}


void String::insertFormattedListAt(int anIndex, const char * aFormat, va_list anArguments)
{
    if (!aFormat)
        return;

    va_list arguments;

    va_copy(arguments, anArguments);
        int formattedLength = vsnprintf(NULL, 0, aFormat, arguments);
        debug_assert(formattedLength >= 0);
    va_end(arguments);
      
    if (formattedLength == 0)  // prevent reallocation (calling wb) unless there is change
    {
        if (isNull())
            setInner("");
        return;
    }

    int selfLength = length();

    if (anIndex > selfLength)
        anIndex = selfLength;

    if (anIndex < 0)
        anIndex = 0;

    int newLength = selfLength + formattedLength;

    char * buffer = wb(newLength);
    memmove(buffer + anIndex + formattedLength, buffer + anIndex, selfLength - anIndex + 1);

    char * formattedTerminatorPosition = buffer + anIndex + formattedLength;
    char temp = *formattedTerminatorPosition;  // save character at position of terminator of formatted string

    va_copy(arguments, anArguments);
        vsnprintf(buffer + anIndex, formattedLength + 1, aFormat, arguments);  // insert formatted string with terminator
    va_end(arguments);

    *formattedTerminatorPosition = temp;  // restore character at position of terminator of formatted string

    enableLengthCache(newLength);
}





// Ensuring Prefix / Suffix ///////////////////////////////////////////////////////////////////////////////////////////

void String::ensurePrefix(const char * aSubstring, EqualityMode aMode)
{
    if (!hasPrefix(aSubstring, aMode))
        insertAt(0, aSubstring);
}


void String::ensurePrefix(const char aChar, EqualityMode aMode)
{
    if (!hasPrefix(aChar, aMode))
        insertAt(0, aChar);
}


void String::ensureSuffix(const char * aSubstring, EqualityMode aMode)
{
    if (!hasSuffix(aSubstring, aMode))
        append(aSubstring);
}


void String::ensureSuffix(const char aChar, EqualityMode aMode)
{
    if (!hasSuffix(aChar, aMode))
        append(aChar);
}





// Converting Case ////////////////////////////////////////////////////////////////////////////////////////////////////

int (*String::onToLower)(int) = tolower;

int (*String::onToUpper)(int) = toupper;


void String::convertTo(LetterCase aLetterCase)
{
    if (isEmpty() || isNull())
        return;

    const char * firstChar = rb();
    const char * testedChar = firstChar;

    if (aLetterCase == upperCase)
        while (*testedChar && *testedChar == onToUpper(*testedChar)) testedChar++;
    else
        while (*testedChar && *testedChar == onToLower(*testedChar)) testedChar++;

    if (!*testedChar)  // prevent reallocation (calling wb) when there is nothing to convert
        return;

    int startIndex = testedChar - firstChar;

    char * writeBufferStart = wb();
    char * convertedChar = writeBufferStart + startIndex;
    
    if (aLetterCase == upperCase)
        do { *convertedChar = onToUpper(*convertedChar); } while (*(++convertedChar));
    else
        do { *convertedChar = onToLower(*convertedChar); } while (*(++convertedChar));

    enableLengthCache(convertedChar - writeBufferStart);
}





// Trimming ///////////////////////////////////////////////////////////////////////////////////////////////////////////

void String::trimLeftCharsBy(ParameterizedCharTestFunction aTestFunction, void * aTestParameter)
{
    if (!aTestFunction || isEmpty() || isNull())  
        return;

    const char * firstChar = rb();
    const char * restChar = firstChar;
    
    while (*restChar && aTestFunction(*restChar, aTestParameter)) 
        restChar++;

    if (restChar != firstChar)  // prevent reallocation (calling wb) when there is nothing to trim
        removeFrom(0, restChar - firstChar);
}


void String::trimLeftChars(CharTestCondition aCondition, const char * aChars)
{
    trimLeftCharsBy(checkingFunctionFor(aCondition), (void *) aChars);
}


void String::trimLeftCharsWhere(CharTestFunction aTestFunction, bool aTestResult)
{
    trimLeftCharsBy(checkingFunctionFor(aTestFunction, aTestResult), (void *)aTestFunction);
}


void String::trimRightCharsBy(ParameterizedCharTestFunction aTestFunction, void * aTestParameter)
{
    if (!aTestFunction || isEmpty() || isNull())  
        return;

    int currentLength = length();

    if (!aTestFunction(rb()[currentLength-1], aTestParameter))  // prevent reallocation (calling wb) when there is nothing to trim
        return;

    char * firstChar = wb();
    char * lastChar = firstChar + currentLength - 1;

    while (lastChar >= firstChar && aTestFunction(*lastChar, aTestParameter))
        lastChar--;

    *(lastChar + 1) = '\0';

    enableLengthCache(lastChar - firstChar + 1);
}


void String::trimRightChars(CharTestCondition aCondition, const char * aChars)
{
    return trimRightCharsBy(checkingFunctionFor(aCondition), (void *)aChars);
}


void String::trimRightCharsWhere(CharTestFunction aTestFunction, bool aTestResult)
{
    trimRightCharsBy(checkingFunctionFor(aTestFunction, aTestResult), (void *)aTestFunction);
}


void String::trimWhitespace()
{
    trimRightCharsWhere(isspace, true);
    trimLeftCharsWhere(isspace, true);
}





// Removing by Index //////////////////////////////////////////////////////////////////////////////////////////////////

void String::removeFrom(int aStartIndex, int aLength)
{
    if (isEmpty() || isNull())
        return;

    if (aStartIndex < 0)
    {
        aLength += aStartIndex;
        aStartIndex = 0;
    }

    if (aLength <= 0)
        return;

    int selfLength = length();

    if (aStartIndex >= selfLength)
        return;

    if (aStartIndex + aLength > selfLength)
        aLength = selfLength - aStartIndex;

    char * buffer = wb();

    if (aLength == selfLength)
        buffer[aStartIndex] = '\0';
    else
        memmove(buffer + aStartIndex, buffer + aStartIndex + aLength, selfLength - aStartIndex - aLength + 1);

    enableLengthCache(selfLength - aLength);
}





// Removing Prefix / Suffix ///////////////////////////////////////////////////////////////////////////////////////////

void String::removePrefix(const char * aCSubstring, EqualityMode aMode)
{
    if (hasPrefix(aCSubstring, aMode))
        removeFrom(0, strlen(aCSubstring));
}


void String::removePrefix(const char aChar, EqualityMode aMode)
{
    if (hasPrefix(aChar, aMode))
        removeFrom(0, 1);
}


void String::removeSuffix(const char * aCSubstring, EqualityMode aMode)
{
    if (hasSuffix(aCSubstring, aMode))
    {
        int substringLength = strlen(aCSubstring);
        removeFrom(self.length() - substringLength, substringLength);
    }
}


void String::removeSuffix(const char aChar, EqualityMode aMode)
{
    if (hasSuffix(aChar, aMode))
        removeFrom(length() - 1, 1);
}





// Removing Characters ////////////////////////////////////////////////////////////////////////////////////////////////

void String::removeCharsBy(ParameterizedCharTestFunction aTestFunction, void * aTestParameter, int aStartIndex)
{
    if (!aTestFunction)
        return;

    if (aStartIndex < 0)
        aStartIndex = 0;

    if (aStartIndex >= length())  // isNull, isEmpty
        return;

    const char * rbFirstChar = rb();
    const char * searchedChar = rbFirstChar + aStartIndex;
    
    while (*searchedChar && !aTestFunction(*searchedChar, aTestParameter)) 
        searchedChar++;
    
    if (*searchedChar == '\0')  // prevent reallocation (calling wb) when there is nothing to remove 
        return;

    aStartIndex = searchedChar - rbFirstChar;  // begin at the first removable character

    char * wbFirstChar = wb();
    char * testedChar = wbFirstChar + aStartIndex;
    char * lastChar = testedChar;

    while (*testedChar)
        if (aTestFunction(*testedChar, aTestParameter))
            testedChar++;
        else
        {
            *lastChar = *testedChar;
            testedChar++;
            lastChar++;
        }

    *lastChar = '\0';

    enableLengthCache(lastChar - wbFirstChar);
}


void String::remove(char aChar, EqualityMode aMode, int aStartIndex)
{
    if (aMode == caseSensitive || !isalpha(aChar))
    {
        char chars[] = { aChar, '\0' };
        removeCharsBy(charIsContainedIn, (void *) &chars, aStartIndex);
    }
    else
    {
        char chars[] = { (char)onToLower(aChar), (char)onToUpper(aChar), '\0' };
        removeCharsBy(charIsContainedIn, (void *) &chars, aStartIndex);
    }
} 


void String::removeChars(CharTestCondition aCondition, const char * aChars, int aStartIndex)
{
    removeCharsBy(checkingFunctionFor(aCondition), (void *)aChars, aStartIndex);
}


void String::removeCharsWhere(CharTestFunction aTestFunction, bool aTestResult, int aStartIndex)
{
    removeCharsBy(checkingFunctionFor(aTestFunction, aTestResult), (void *)aTestFunction, aStartIndex);
}





// Replacing //////////////////////////////////////////////////////////////////////////////////////////////////////////

void String::replaceCharsBy(ParameterizedCharTestFunction aTestFunction, void * aTestParameter, char aSubstitute, int aStartIndex)
{   
    if (!aTestFunction || isEmpty() || isNull())
        return;

    if (aStartIndex < 0)
        aStartIndex = 0;

    if (aStartIndex >= length())
        return;

    const char * rbFirstChar = rb();
    const char * foundAt = rbFirstChar + aStartIndex;
    
    while (*foundAt && !aTestFunction(*foundAt, aTestParameter)) 
        foundAt++;
    
    if (*foundAt == '\0')  // prevent reallocation (calling wb) when there is nothing to replace 
        return;

    int foundAtIndex = foundAt - rbFirstChar;  // begin at the first character to replace

    char * wbFirstChar = wb();
    char * currentChar = wbFirstChar + foundAtIndex;

    while (*currentChar)
    {
        if (aTestFunction(*currentChar, aTestParameter))
            *currentChar = aSubstitute;
        currentChar++;
    }

    enableLengthCache(aSubstitute ? currentChar - wbFirstChar : csUnknown);
}


void String::replace(char anOriginal, char aSubstitute, EqualityMode aMode, int aStartIndex)
{
    if (aMode == caseSensitive || !isalpha(anOriginal))
    {
        char original[] = { anOriginal, '\0' };
        replaceCharsBy(charIsContainedIn, (void *) &original, aSubstitute, aStartIndex);
    }
    else
    {
        char original[] = { (char)onToLower(anOriginal), (char)onToUpper(anOriginal), '\0' };
        replaceCharsBy(charIsContainedIn, (void *) &original, aSubstitute, aStartIndex);
    }    
}


void String::replaceChars(CharTestCondition aCondition, const char * aChars, char aSubstitute, int aStartIndex)
{
    replaceCharsBy(checkingFunctionFor(aCondition), (void *)aChars, aSubstitute, aStartIndex);
}


void String::replaceCharsWhere(CharTestFunction aTestFunction, bool aTestResult, char aSubstitute, int aStartIndex)
{
    replaceCharsBy(checkingFunctionFor(aTestFunction, aTestResult), (void *)aTestFunction, aSubstitute, aStartIndex);
}


void String::replace(const char * anOriginal, const char * aSubstitute, EqualityMode aMode, int aStartIndex)
{
    if (isEmpty() || isNull() || !anOriginal || isEmptyCString(anOriginal))
        return;

    int selfLength = self.length();
    int originalLength = strlen(anOriginal);

    if (aStartIndex < 0)
        aStartIndex = 0;

    int searchedLength = selfLength - aStartIndex;
    if (searchedLength < originalLength)
        return;

    const char * rbFirstChar = rb(); 
    const char * rbFoundAt = findSubstring(rbFirstChar + aStartIndex, anOriginal, aMode); 

    if (!rbFoundAt)  // prevent reallocation (calling wb) when there is nothing to replace
        return;

    int foundAtIndex = rbFoundAt - rbFirstChar;
    int substituteLength = aSubstitute ? strlen(aSubstitute) : 0;  // NULL is considered to be empty string

    if (substituteLength == originalLength)
    {
        char * firstChar = wb();
        char * foundAt = firstChar + foundAtIndex;

        do {
            memmove(foundAt, aSubstitute, substituteLength);
            foundAt += substituteLength;
        } while (( foundAt = findSubstring(foundAt, anOriginal, aMode) )); 

        enableLengthCache(selfLength);
    }
    else
        if (substituteLength < originalLength)
        {
            char * firstChar = wb();
            char * foundAt = firstChar + foundAtIndex;

            char * writeTo = foundAt;
            memmove(writeTo, aSubstitute, substituteLength);
            writeTo += substituteLength;

            foundAt += originalLength;
            char * readFrom = foundAt;

            while (( foundAt = findSubstring(foundAt, anOriginal, aMode) ))
            {
                int movedLength = foundAt - readFrom;
                memmove(writeTo, readFrom, movedLength);
                writeTo += movedLength;

                memmove(writeTo, aSubstitute, substituteLength);
                writeTo += substituteLength;

                foundAt += originalLength;
                readFrom = foundAt;
            }

            int movedLength = (firstChar + selfLength) - readFrom;
            memmove(writeTo, readFrom, movedLength);
            writeTo += movedLength;

            *writeTo = '\0';

            enableLengthCache(writeTo - firstChar);
        }
        else
        {
            int increment = substituteLength - originalLength;   
            int newLength = selfLength;         

            do {
                newLength += increment;
                rbFoundAt += originalLength;
            } while (( rbFoundAt = findSubstring(rbFoundAt, anOriginal, aMode) ));

            char * firstChar = wb(newLength);
            char * termChar = firstChar + selfLength;
            char * foundAt = firstChar + foundAtIndex;

            do {
                char * readFrom = foundAt + originalLength;
                char * writeTo = foundAt + substituteLength;
                int movedLength = termChar - readFrom + 1;
                memmove(writeTo, readFrom, movedLength);

                memmove(foundAt, aSubstitute, substituteLength);

                termChar += increment; 
                foundAt += substituteLength;

            } while (( foundAt = findSubstring(foundAt, anOriginal, aMode) ));

            enableLengthCache(newLength);
        }
}





// Padding ////////////////////////////////////////////////////////////////////////////////////////////////////////////

void String::padLeft(int aLength, char aChar)
{
    if (aLength <= 0)
        return;

    int selfLength = self.length();

    if (aLength <= selfLength)
        return;

    char * buffer = wb(aLength);

    int paddingLength = aLength - selfLength;
    memmove(buffer + paddingLength, buffer, selfLength + 1);  // move including terminating null char
    memset(buffer, aChar, paddingLength);

    int actualLength;

    if (aChar)
        actualLength = aLength;
    else
        actualLength = 0;

    enableLengthCache(actualLength);
}


void String::padRight(int aLength, char aChar)
{
    if (aLength <= 0)
        return;

    int selfLength = self.length();

    if (aLength <= selfLength)
        return;

    char * buffer = wb(aLength);
    memset(buffer + selfLength, aChar, aLength - selfLength);
    buffer[aLength] = '\0';

    int actualLength;

    if (aChar)
        actualLength = aLength;
    else
        actualLength = selfLength;

    enableLengthCache(actualLength);
}


void String::padCenter(int aLength, char aChar)
{
    if (aLength <= 0)
        return;

    int selfLength = self.length();

    if (aLength <= selfLength)
        return;

    int paddingLength = aLength - selfLength;
    int leftPaddingLength = paddingLength / 2;
    int rightPaddingLength = leftPaddingLength + paddingLength % 2;

    char * buffer = wb(aLength);
    memmove(buffer + leftPaddingLength, buffer, selfLength);
    memset(buffer, aChar, leftPaddingLength);
    memset(buffer + leftPaddingLength + selfLength, aChar, rightPaddingLength);
    buffer[aLength] = '\0';

    int actualLength;

    if (aChar)
        actualLength = aLength;
    else
        actualLength = csUnknown;

    enableLengthCache(actualLength);
}





// Reversing //////////////////////////////////////////////////////////////////////////////////////////////////////////

void String::reverse()
{
    int selfLength = length();

    if (selfLength < 2)  
        return;

    char * beginning = wb();
    char * end = beginning + selfLength - 1;

    while (beginning < end)
    {
        char temp = *beginning;
        *beginning = *end;
        *end = temp;

        beginning++;
        end--;
    }

    enableLengthCache(selfLength);
}





// Formatting /////////////////////////////////////////////////////////////////////////////////////////////////////////

String String::formattedList(const char * aFormat, va_list anArguments)
{
    if (!aFormat)
        return null;

    String result;
    result.appendFormattedList(aFormat, anArguments);
    return result;
}


String String::formatted(const char * aFormat, ...)
{
    va_list arguments;
    
    va_start(arguments, aFormat);
        String result = String::formattedList(aFormat, arguments);
    va_end(arguments);
    
    return result;
}





// String Of Characters ///////////////////////////////////////////////////////////////////////////////////////////////
String String::ofChar(char aChar, int aCount)
{
    if (aCount < 1)
        return empty;
    
    if (aCount == 1)
        return String(aChar);

    String result = String::withCapacity(aCount);
    
    char * buffer = result.wb();
    memset(buffer, aChar, aCount);
    buffer[aCount] = '\0';

    result.enableLengthCache(aChar ? aCount : 0);

    return result;
}





// Parsing ////////////////////////////////////////////////////////////////////////////////////////////////////////////

int String::partCount(const ParsingContext & aContext) const
{
    return partCount(aContext.delimiterChars.rb(), aContext.quotingChars.rb(), aContext.ignoreEmpty);
}


int String::partCount(const char * aDelimiterChars, const char * aQuotationChars, bool anIgnoreEmpty) const
{
    int partCount = 0;
    int charIndex = 0;

    while (nextPart(NULL, &charIndex, aDelimiterChars, aQuotationChars, anIgnoreEmpty))
        partCount++;

    return partCount;
}


String String::part(int aPartIndex, const ParsingContext & aContext) const
{
    return part(aPartIndex, aContext.delimiterChars.rb(), aContext.quotingChars.rb(), aContext.ignoreEmpty);
}


String String::part(int aPartIndex, const char * aDelimiterChars, const char * aQuotationChars, bool anIgnoreEmpty) const
{
    if (aPartIndex < 0)
        return empty;

    int charIndex = 0;

    int currentPartIndex = 0;
    while (currentPartIndex < aPartIndex && nextPart(NULL, &charIndex, aDelimiterChars, aQuotationChars, anIgnoreEmpty))
        currentPartIndex++;

    String result;
    nextPart(&result, &charIndex, aDelimiterChars, aQuotationChars, anIgnoreEmpty);

    return result;
}


void String::appendBlockAndIncrementIndex(int * ioCharIndex, int aLength, String * ioToken, bool * ioTokenIsEmpty) const
{
    if (ioToken) 
        (*ioToken).append(substringFrom(*ioCharIndex, aLength));
    *ioCharIndex += aLength;
    *ioTokenIsEmpty = false;
}


bool String::nextPart(String * oPart, ParsingContext * aContext) const
{
    return nextPart(oPart, &(aContext->charIndex), aContext->delimiterChars.rb(), aContext->quotingChars.rb(), aContext->ignoreEmpty);
}


bool String::nextPart(String * oPart, int * ioCharIndex, const char * aDelimiterChars, const char * aQuotationChars, bool anIgnoreEmpty) const
{
    if (oPart)
        *oPart = empty;

    int selfLength = self.length();  

    if (*ioCharIndex >= selfLength)  // also isEmpty, isNull
        return false;

    if (*ioCharIndex < 0)
        *ioCharIndex = 0;

    if (!aQuotationChars)
        aQuotationChars = "";

    if (!aDelimiterChars)
        aDelimiterChars = "";

    String controlChars = String(aDelimiterChars) + aQuotationChars;
    
    int  blockLength;
    bool tokenIsQuoted;
    bool tokenIsEmpty = true;
    bool tokenIsDelimited = false;

    do {
        if (containsCharsAt(*ioCharIndex, notContainedIn, controlChars, &blockLength))
            appendBlockAndIncrementIndex(ioCharIndex, blockLength, oPart, &tokenIsEmpty);

        tokenIsQuoted = false;

        while (containsAnyCharAt(*ioCharIndex, containedIn, aQuotationChars))
        {
            char quotationChar = self[*ioCharIndex];

            *ioCharIndex += 1;

            if (!tokenIsQuoted)  // part content is only text between quotation characters
            {
                if (oPart) *oPart = empty;  // remove characters before quotation character
                tokenIsEmpty = true;
                tokenIsQuoted = true;
            }

            char doubleQuoted = false;

            do {
                if (containsCharsAt(*ioCharIndex, notContainedIn, quotationChar, &blockLength))
                    appendBlockAndIncrementIndex(ioCharIndex, blockLength, oPart, &tokenIsEmpty);

                if (containsAt(*ioCharIndex, quotationChar))
                    *ioCharIndex += 1;
                
                doubleQuoted = containsAt(*ioCharIndex, quotationChar);  // two consecutive quotation characters  
                if (doubleQuoted)
                    appendBlockAndIncrementIndex(ioCharIndex, 1, oPart, &tokenIsEmpty);  // resulting in one quoting character in part

            } while (doubleQuoted);

            if (containsCharsAt(*ioCharIndex, notContainedIn, controlChars, &blockLength))  // skip characters after quotation character
                *ioCharIndex += blockLength;
        }
    
        if (containsAnyCharAt(*ioCharIndex, containedIn, aDelimiterChars))
        {
            *ioCharIndex += 1;
            tokenIsDelimited = true;
        }

    } while (anIgnoreEmpty && tokenIsEmpty && !tokenIsQuoted && *ioCharIndex < selfLength);

    return !tokenIsEmpty || tokenIsQuoted || (tokenIsDelimited && !anIgnoreEmpty) ;
}





}