#include "server.h"
#include <qdebug.h>
#include <QString>


bool char_isspace(char c) {
    return std::isspace(static_cast<unsigned char>(c));
}

ERequestType getRequestMethod(string sRequest)
{
    size_t endMethod = sRequest.find(" ");
    std::string sMethod = sRequest.substr(0, endMethod);

    ERequestType requestType;
    if(sMethod == "GET")
        requestType = eGET;
    else if (sMethod == "POST")
        requestType = ePOST;
    else if (sMethod == "HEAD")
        requestType = eHEAD;

    return requestType;
}

string getDecodedPath(string sRequest)
{
    //qDebug()<<QString::fromStdString(sRequest) << "sRequest";
    size_t startURI = sRequest.find(" ") + 1;
    size_t endURI = sRequest.find("HTTP") - 5;

    std::string sPath = sRequest.substr(startURI, endURI);
    size_t paramString = sPath.find("?");
    sPath = sPath.substr(0, paramString);

    string res = urlDecode(sPath);

    //qDebug()<<QString::fromStdString(res) << "res";
    return res;
}

EContentType getContentType(string sPath)
{
    unsigned found = sPath.find_last_of("/\\");
    string sFile = sPath.substr(found+1);

    unsigned extensionPos = sFile.find_last_of(".") + 1;
    string sExtension = sFile.substr(extensionPos);

    EContentType eContentType = eUnknown;
    if(sExtension == "html")
        eContentType = eHTML;
    else if(sExtension == "png")
        eContentType = ePNG;
    else if(sExtension == "jpg")
        eContentType = eJPG;
    else if(sExtension == "jpeg")
        eContentType = eJPEG;
    else if(sExtension == "css")
        eContentType = eCSS;
    else if(sExtension == "js")
        eContentType = eJS;
    else if(sExtension == "gif")
        eContentType = eGIF;
    else if(sExtension == "swf")
        eContentType = eSWF;

    return eContentType;
}

bool            isPathDirectory(string sPath)
{
    return true;
}

void writeHeader(HttpHeader *pHttpHeader, evbuffer *pOutput)
{
    string sResponse;
    if(pHttpHeader->eResponseType == eOK)
    {
        sResponse.append("HTTP/1.1 200 OK\r\n");
        //Parse time
        char timeStr[22];
        time_t now = time(NULL);
        strftime(timeStr, 22, "%Y-%m-%d %H:%M:%S", localtime(&now));
        string sDate = "Date: " + string(timeStr) + "\r\n";
        sResponse.append(sDate);

        sResponse.append("Server: badServer\r\n");

        //Parse content type
        string sContent;

        if(pHttpHeader->eContentType == eJPG){
            sContent = "image/jpeg";
        }
        else if(pHttpHeader->eContentType == ePNG){
            sContent = "image/png";
        }
        else if(pHttpHeader->eContentType == eJPEG){
            sContent = "image/jpeg";
        }
        else if(pHttpHeader->eContentType == eCSS){
            sContent = "text/css";
        }
        else if(pHttpHeader->eContentType == eJS){
            sContent = "application/x-javascript";
        }
        else if(pHttpHeader->eContentType == eGIF){
            sContent = "image/gif";
        }
        else if(pHttpHeader->eContentType == eSWF){
            sContent = "application/x-shockwave-flash";
        }
        else{//(pHttpHeader->eContentType == eHTML){
            sContent = "text/html";
        }
        sResponse.append("Content-Type: " + sContent + "\r\n");
        sResponse.append("Content-Length: " + convertInt(pHttpHeader->nContentLength) + "\r\n");
        sResponse.append("Connection: close\r\n\r\n");


    }
    else if(pHttpHeader->eResponseType == eNotFound)
    {
        sResponse.append("HTTP/1.1 404 Not Found\r\n\r\n");
        sResponse.append("404 Not Found");
    }
    else if(pHttpHeader->eResponseType == eBadRequest)
    {
        sResponse.append("HTTP/1.1 405 Bad Request\r\n\r\n");
        sResponse.append("405 Bad Request");
    }
    else if(pHttpHeader->eResponseType == eForbiden)
    {
        sResponse.append("HTTP/1.1 403 Forbidden\r\n\r\n");
        sResponse.append("403 Forbidden");
    }

    evbuffer_add_printf(pOutput, sResponse.c_str());
}


void sendDocument(struct bufferevent *pBufferEvent)
{
    struct evbuffer* pInput = bufferevent_get_input(pBufferEvent);
    struct evbuffer* pOutput = bufferevent_get_output(pBufferEvent);

    string sRequest = string((char*)evbuffer_pullup(pInput, -1));
    /*printf("Received %zu bytes\n", evbuffer_get_length(pInput));
    printf("----- data ----\n");
    printf("%.*s\n", (int)evbuffer_get_length(pInput), sRequest.c_str());*/

    ERequestType eRequestType = getRequestMethod(sRequest);
    string sPath = getDecodedPath(sRequest);
    bool bIsPathOk = isPathCorrect(sPath);

    HttpHeader* pHttpHeader = new HttpHeader();

    //Load home page
    char c = *sPath.rbegin();
    if(c == '/'){
        sPath = sPath.append("index.html");
    }

    //Open file on disk
    int fd = -1;
    struct stat st;
    string sWholePath = SERVER_PATH + sPath;
    if (eRequestType ==  eHEAD)
    {
        sWholePath.erase(remove_if(sWholePath.begin(), sWholePath.end(), char_isspace),sWholePath.end());
        fd = open(sWholePath.c_str(), O_RDONLY);
        //qDebug()<<QString::fromStdString(sWholePath.c_str()) << "HEAD";
        if (fstat(fd, &st)<0)
        {
            perror("fstat");

        }
        else
        {
            pHttpHeader->eResponseType = eOK;
            pHttpHeader->nContentLength = st.st_size;
            pHttpHeader->eContentType = getContentType(sPath);
        }
    }
    //else
        //qDebug()<<QString::fromStdString(sWholePath.c_str());
    else if(eRequestType != eGET)//(eRequestType != eGET && eRequestType !=  eHEAD)
    {
        pHttpHeader->eResponseType = eBadRequest;
    }
    else if(!bIsPathOk)
    {
        pHttpHeader->eResponseType = eForbiden;
    }
    else  if((fd = open(sWholePath.c_str(), O_RDONLY)) < 0)
    {
        if(c != '/')
            pHttpHeader->eResponseType = eNotFound;
        else
            pHttpHeader->eResponseType = eForbiden;
    }
    else
    {
        if (fstat(fd, &st)<0)
        {
            /* Make sure the length still matches, now that we
             * opened the file :/ */
            perror("fstat");
        }
        pHttpHeader->eResponseType = eOK;
        pHttpHeader->nContentLength = st.st_size;
        pHttpHeader->eContentType = getContentType(sPath);
    }

    //Write http header
    writeHeader(pHttpHeader, pOutput);

    //Write static file to output

    //if(fd != -1)
    if(eRequestType == eGET)
        evbuffer_add_file(pOutput, fd, 0, st.st_size);
    //else if(eRequestType == eHEAD)
        //evbuffer_add_printf(pOutput, "\r\n\r\n");//evbuffer_add(pOutput, CRLF, 2);

    delete pHttpHeader;
}

string convertInt(int number)
{
    if (number == 0)
        return "0";
    string temp="";
    string returnvalue="";
    while (number>0)
    {
        temp+=number%10+48;
        number/=10;
    }
    for (int i=0;i<temp.length();i++)
        returnvalue+=temp[temp.length()-i-1];
    return returnvalue;
}


string urlDecode(string &SRC) {
    string ret;
    char ch;
    int i, ii;
    for (i=0; i<SRC.length(); i++) {
        if (int(SRC[i])==37) {
            sscanf(SRC.substr(i+1,2).c_str(), "%x", &ii);
            ch=static_cast<char>(ii);
            ret+=ch;
            i=i+2;
        } else {
            ret+=SRC[i];
        }
    }
    return (ret);
}

bool isPathCorrect(string sPath)
{
    std::vector<std::string> dirVector = split(sPath, '/');
    int upCount = 0;
    int downCount = 0;
    for(std::vector<std::string>::iterator it = dirVector.begin(); it != dirVector.end(); ++it)
    {
        string sDir = it.operator *();
        if(sDir == "..")
            ++downCount;
        else if (!sDir.empty())
            ++upCount;
    }

    return upCount >= downCount ? true : false;
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}
