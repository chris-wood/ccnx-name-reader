#ifndef ccnx_name_reader_h_
#define ccnx_name_reader_h_

struct ccnx_name_reader;
typedef struct ccnx_name_reader CCNxNameReader;

/**
 * Create a name reader from the specified file. 
 */
CCNxNameReader *ccnxNameReader_Create(char *fileName);

/**
 * Retrieve an iterator to walk over the names in the specified reader.
 */
PARCIterator *ccnxNameReader_Iterator(const CCNxNameReader *reader);

#endif // ccnx_name_reader_h_
