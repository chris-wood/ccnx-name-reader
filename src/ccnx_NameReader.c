#include <parc/algol/parc_SafeMemory.h>
#include <parc/algol/parc_BufferComposer.h>
#include <parc/algol/parc_LinkedList.h>
#include <parc/algol/parc_Iterator.h>
#include <parc/algol/parc_File.h>
#include <parc/algol/parc_FileInputStream.h>
#include <parc/developer/parc_StopWatch.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>

#include <ccnx/common/ccnx_Name.h>

typedef struct {
    PARCFile *sourceFile;
    int fd;
} CCNxNameReader;

static bool
_ccnxNameReader_Destructor(CCNxNameReader **statsPtr)
{
    CCNxNameReader *reader = *statsPtr;
    parcFile_Release(&reader->sourceFile);
    close(reader->fd);
    return true;
}

parcObject_Override(CCNxNameReader, PARCObject,
                    .destructor = (PARCObjectDestructor *) _ccnxNameReader_Destructor);

parcObject_ImplementAcquire(ccnxNameReader, CCNxNameReader);
parcObject_ImplementRelease(ccnxNameReader, CCNxNameReader);

CCNxNameReader *
ccnxNameReader_Create(char *fileName)
{
    CCNxNameReader *reader = parcObject_CreateInstance(CCNxNameReader);
    if (reader != NULL) {
        reader->sourceFile = parcFile_Create(fileName);
        if (!(reader->sourceFile != NULL && parcFile_Exists(reader->sourceFile))) {
            ccnxNameReader_Release(&reader);
        }
        reader->fd = open(fileName, O_RDONLY, 0600);
    }
    return reader;
}

static bool
_readToBuffer(int fd, PARCBuffer *buffer)
{
    while (parcBuffer_HasRemaining(buffer)) {
        void *buf = parcBuffer_Overlay(buffer, 0);
        int nread = read(fd, buf, parcBuffer_Remaining(buffer));
        if (nread <= 0) {
            break;
        }
        parcBuffer_SetPosition(buffer, parcBuffer_Position(buffer) + nread);
    }
    return !parcBuffer_HasRemaining(buffer);
}

typedef struct {
    bool atEnd;
    int numLines;
    int fd;

    PARCBufferComposer *composer;
    PARCBuffer *leftover;
} _ccnxNameReaderIteratorState;

static void *
_ccnxNameReaderIterator_Init(CCNxNameReader *group)
{
    _ccnxNameReaderIteratorState *state = parcMemory_Allocate(sizeof(_ccnxNameReaderIteratorState));
    state->atEnd = false;
    state->fd = group->fd;
    state->composer = NULL;
    state->leftover = NULL;
    return state;
}

static bool
_ccnxNameReaderIterator_HasNext(CCNxNameReader *group, void *voidstate)
{
    _ccnxNameReaderIteratorState *state = (_ccnxNameReaderIteratorState *) voidstate;
    return !(state->atEnd && state->leftover == NULL);
}

#include <stdio.h>

static void *
_ccnxNameReaderIterator_Next(CCNxNameReader *group, void *state)
{
    _ccnxNameReaderIteratorState *thestate = (_ccnxNameReaderIteratorState *) state;
    thestate->numLines++;
    
    if (thestate->composer != NULL) {   
        parcBufferComposer_Release(&thestate->composer);
    }
    thestate->composer = parcBufferComposer_Create();

    if (thestate->leftover != NULL) {
        int newLineIndex = parcBuffer_FindUint8(thestate->leftover, '\n');
        if (newLineIndex < 0) {
            parcBufferComposer_PutBuffer(thestate->composer, thestate->leftover);
            parcBuffer_Release(&thestate->leftover);
        } else {
            for (size_t i = 0; i < newLineIndex; i++) {
                parcBufferComposer_PutChar(thestate->composer, parcBuffer_GetUint8(thestate->leftover));
            }
            parcBuffer_GetUint8(thestate->leftover);
            return thestate;
        }
    } else if (!thestate->atEnd) {

    bool keepReading = true;
    do {
        keepReading = false;
        PARCBuffer *buffer = parcBuffer_Allocate(64);
        bool filled = _readToBuffer(thestate->fd, buffer);
        parcBuffer_Flip(buffer);

        if (!filled) {
            thestate->atEnd = true;
        }

        int newlineIndex = parcBuffer_FindUint8(buffer, '\n');
        if (newlineIndex < 0) {
            parcBufferComposer_PutBuffer(thestate->composer, buffer);
            keepReading = true;
        } else {
            for (size_t i = 0; i < newlineIndex; i++) {
                parcBufferComposer_PutChar(thestate->composer, parcBuffer_GetUint8(buffer));
            }
            parcBuffer_GetUint8(buffer); // swallow the newline

            PARCBufferComposer *composer = parcBufferComposer_Create();
            parcBufferComposer_PutBuffer(composer, buffer);
            thestate->leftover = parcBufferComposer_ProduceBuffer(composer);
            parcBufferComposer_Release(&composer);
        }

        parcBuffer_Release(&buffer);
    } while (keepReading && !thestate->atEnd);
    }
    
    /*
    if (thestate->pointerNumber == parcLinkedList_Size(group->pointers)) {
        thestate->atEnd = true;
    }
    */

    return thestate;
}

static void
_ccnxNameReaderIterator_RemoveAt(CCNxNameReader *group, void **state)
{
    // pass
}

static void *
_ccnxNameReaderIterator_GetElement(CCNxNameReader *group, void *state)
{
    _ccnxNameReaderIteratorState *thestate = (_ccnxNameReaderIteratorState *) state;
    PARCBuffer *nameBuffer = parcBuffer_Flip(parcBufferComposer_CreateBuffer(thestate->composer));
    CCNxName *name = ccnxName_CreateFromBuffer(nameBuffer);
    parcBuffer_Release(&nameBuffer);
    return name;
}

static void
_ccnxNameReaderIterator_Finish(CCNxNameReader *group, void *state)
{
    _ccnxNameReaderIteratorState *thestate = (_ccnxNameReaderIteratorState *) state;
    if (thestate->composer != NULL) {
        parcBufferComposer_Release(&thestate->composer);
    }
    if (thestate->leftover != NULL) {
        parcBuffer_Release(&thestate->leftover);
    }
    parcMemory_Deallocate(&thestate);
}

static void
_ccnxNameReaderIterator_AssertValid(const void *state)
{
    // pass
}

PARCIterator *
ccnxNameReader_Iterator(const CCNxNameReader *reader)
{
    PARCIterator *iterator = parcIterator_Create((void *) reader,
                                                 (void *(*)(PARCObject *)) _ccnxNameReaderIterator_Init,
                                                 (bool  (*)(PARCObject *, void *)) _ccnxNameReaderIterator_HasNext,
                                                 (void *(*)(PARCObject *, void *)) _ccnxNameReaderIterator_Next,
                                                 (void  (*)(PARCObject *, void **)) _ccnxNameReaderIterator_RemoveAt,
                                                 (void *(*)(PARCObject *, void *)) _ccnxNameReaderIterator_GetElement,
                                                 (void  (*)(PARCObject *, void *)) _ccnxNameReaderIterator_Finish,
                                                 (void  (*)(const void *)) _ccnxNameReaderIterator_AssertValid);

    return iterator;
}

/*
PARCBufferComposer *
XXX(FILE *fp)
{
    PARCBufferComposer *composer = parcBufferComposer_Create();
    char curr = fgetc(fp);
    while ((isalnum(curr) || curr == ':' || curr == '/' || curr == '.' ||
            curr == '_' || curr == '(' || curr == ')' || curr == '[' ||
            curr == ']' || curr == '-' || curr == '%' || curr == '+' ||
            curr == '=' || curr == ';' || curr == '$' || curr == '\'') && curr != EOF) {
        parcBufferComposer_PutChar(composer, curr);
        curr = fgetc(fp);
    }
    return composer;
}
*/
