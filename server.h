/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
#include <arpa/inet.h>
/* For fcntl */
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <event2/listener.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <vector>
#include <sstream>

#define SERVER_PATH "/home/alexey/testHttpServer/Static"
using namespace std;

enum EResponseType
{
    eOK = 200,
    eBadRequest = 405,
    eNotFound = 404,
    eForbiden = 403
};

enum ERequestType
{
    eGET,
    ePOST,
    eHEAD
};

enum EContentType
{
    eHTML,
    eJPG,
    eJPEG,
    ePNG,
    eGIF,
    eSWF,
    eCSS,
    eJS,

    eUnknown
};

struct HttpHeader
{
    EResponseType eResponseType;
    EContentType eContentType;
    unsigned int nContentLength;
};


string          urlDecode(string &SRC);
bool            isPathCorrect(string sPath);
ERequestType    getRequestMethod(string sRequest);
string          getDecodedPath(string sRequest);
EContentType    getContentType(string sPath);
void            writeHeader(HttpHeader* pHttpHeader, evbuffer* pOutput);
void            sendDocument(struct bufferevent *pBufferEvent);
string          convertInt(int number);

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);
